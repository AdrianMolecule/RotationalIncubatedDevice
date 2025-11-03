#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WebSocketsServer.h>
#include <WiFi.h>

#include <vector>

#include "JsonWrapper.h"
#include "pass.h"

//Adrian next I'll remove the unnecessary socket server

// ----------------------
// Field and Model Classes
// ----------------------

class Model {
   public:
    std::vector<Field> fields;

    Field* getById(const String& id) {
        for (auto& f : fields)
            if (f.getId() == id) return &f;
        return nullptr;
    }

    Field* getByName(const String& name) {
        for (auto& f : fields)
            if (f.getName() == name) return &f;
        return nullptr;
    }

    void add(Field f) { fields.push_back(f); }

    bool remove(const String& id) {
        for (size_t i = 0; i < fields.size(); i++) {
            if (fields[i].getId() == id) {
                fields.erase(fields.begin() + i);
                return true;
            }
        }
        return false;
    }

    void reorder(const String& id, bool up) {
        for (size_t i = 0; i < fields.size(); i++) {
            if (fields[i].getId() == id) {
                if (up && i > 0)
                    std::swap(fields[i], fields[i - 1]);
                else if (!up && i < fields.size() - 1)
                    std::swap(fields[i], fields[i + 1]);
                break;
            }
        }
    }

    bool load() {
        if (!SPIFFS.exists("/model.json")) return false;
        File f = SPIFFS.open("/model.json", "r");
        if (!f) return false;
        size_t size = f.size();
        if (size == 0) {
            f.close();
            return false;
        }
        std::unique_ptr<char[]> buf(new char[size + 1]);
        f.readBytes(buf.get(), size);
        buf[size] = 0;
        f.close();
        JsonDocument doc;
        if (deserializeJson(doc, buf.get())) return false;
        fields.clear();
        for (JsonObject fld : doc["fields"].as<JsonArray>()) {
            Field f;
            f.fromJson(fld);
            fields.push_back(f);
        }
        return !fields.empty();
    }

    bool saveToFile() {
        File file = SPIFFS.open("/model.json", "w");
        if (!file) {
            Serial.println("error in model saveToFile file");
            return false;
        }
        return JsonWrapper::saveModelToFile("/model.json", fields);
        return true;
    }

    String fieldsToJsonString() {
        return JsonWrapper::fieldsToJsonString(fields);
    }

    void listSerial() {
        for (auto& f : fields) Serial.printf("%s (%s) = %s\n", f.getName().c_str(), f.getType().c_str(), f.getValue().c_str());
    }
};

Model model;

// ----------------------
// Servers
// ----------------------
AsyncWebServer server(80);
WebSocketsServer webSocket(81);

// ----------------------
// Web Pages
// ----------------------
String generateMenu() {
    return "<p><a href='/'>Index</a> | <a href='/info'>Info</a>| <a href='/metadata'>Metadata</a> | <a href='/debug'>Debug</a> | <a href='/reboot'>Reboot</a></p>";
}

String generateIndexPage(bool brief) {
    String html = generateMenu();
    html += "<h1>Index Page</h1><table border=1><tr><th>Name</th><th>Type</th><th>Value</th><th>Description</th></tr>";
    for (auto& f : model.fields) {
        if (brief && f.getReadOnly()) {
            continue;
        }
        html += "<tr><td>" + f.getName() + "</td><td>" + f.getType() + "</td>";
        if (f.getReadOnly())
            html += "<td><input value='" + f.getValue() + "' disabled></td>";
        else
            html += "<td><input data-id='" + f.getId() + "' value='" + f.getValue() + "' onchange='onChange(this)'></td>";
        html += "<td>" + f.getDescription() + "</td></tr>";
    }
    html += R"(
    <script>
    function onChange(el){
        var val = el.value;
        var id = el.getAttribute('data-id');
        ws.send(JSON.stringify({action:'update',id:id,value:val}));
    }
    var ws = new WebSocket('ws://'+location.hostname+':81/');
    ws.onmessage=function(evt){ location.reload(); }
    </script>
    )";
    return html;
}
String generateMetadataPage() {
    String html = generateMenu();
    html += "<h1>Metadata</h1><table border=1><tr><th>Name</th><th>Type</th><th>Value</th><th>Description</th><th>ReadOnly</th><th>Reorder</th><th>Delete</th></tr>";
    for (auto& f : model.fields) {
        html += "<tr>";
        html += "<td>" + f.getName()+ "</td><td>" + f.getType() + "</td><td>" + f.getValue() + "</td><td>" + f.getDescription() + "</td>";
        html += "<td>" + String(f.getReadOnly()) + "</td>";
        html += "<td><button onclick='reorder(\"" + f.getId() + "\",true)'>&#9650;</button> <button onclick='reorder(\"" + f.getId() + "\",false)'>&#9660;</button></td>";
        html += "<td><button onclick='delField(\"" + f.getId() + "\")'>Delete</button></td>";
        html += "</tr>";
    }

    html += "</table><h3>Add New Field</h3>";
    html += "ID: <input id='fid'><br>Name: <input id='fname'><br>Type: <input id='ftype'><br>Value: <input id='fvalue'><br>Description: <input id='fdesc'><br>ReadOnly: <input id='freadonly' type='checkbox'><br>";
    html += "<button onclick='addField()'>Add Field</button>";
    html += R"(
    <script>
    var ws = new WebSocket('ws://'+location.hostname+':81/');
    function delField(id){ ws.send(JSON.stringify({action:'delete',id:id})); }
    function reorder(id,up){ ws.send(JSON.stringify({action:up?'moveUp':'moveDown',id:id})); }
    function addField(){
        var msg={action:'add',field:{id:document.getElementById('fid').value,
                                    name:document.getElementById('fname').value,
                                    type:document.getElementById('ftype').value,
                                    value:document.getElementById('fvalue').value,
                                    description:document.getElementById('fdesc').value,
                                    readOnly:document.getElementById('freadonly').checked}};
        ws.send(JSON.stringify(msg));
    }
    ws.onmessage=function(evt){ location.reload(); }
    </script>
    )";
    return html;
}

String generateDebugPage() {
    String html = generateMenu();
    html += "<h1>Debug</h1><pre>" + model.fieldsToJsonString() + "</pre>";
    return html;
}

// ----------------------
// BackEnd Class
// ----------------------
class BackEnd {
   public:
    static unsigned long durationSinceRebootInSeconds;

    static void setupBackend() {
        durationSinceRebootInSeconds = 0;
        Serial.println("[BACKEND] Backend initialized.");
    }

    static void loopBackend() {
        while (true) {  // real backend FOREVER loop
            durationSinceRebootInSeconds = millis() / 1000;
            static unsigned long lastModelUpdateInSeconds = 0;
            if (durationSinceRebootInSeconds - lastModelUpdateInSeconds >= 5) {  // we update the duration every 5 seconds
                lastModelUpdateInSeconds = durationSinceRebootInSeconds;
                Field* f = model.getByName("duration");
                if (!f) {
                    Field nf("10", "duration", "string", String(durationSinceRebootInSeconds), "time since start", true);
                    model.add(nf);
                    model.saveToFile();
                    webSocket.broadcastTXT(model.fieldsToJsonString().c_str());
                    Serial.printf("[BACKEND] Created field 'duration' = %s\n", nf.getValue().c_str());
                } else {
                    f->setValue(String(durationSinceRebootInSeconds));
                    model.saveToFile();
                    webSocket.broadcastTXT(model.fieldsToJsonString().c_str());
                    Serial.printf("[BACKEND] Updated field 'duration' = %s\n", f->getValue().c_str());
                }
            }
            // make sure we have a delay in the model. If not prepopulate with n seconds
            Field* f = model.getByName("delay");
            String delayAsString;
            if (!f) {
                Field nf("11", "delay", "string", String(7), "blocking delay in the backEndLoop", false);
                model.add(nf);
                model.saveToFile();
                webSocket.broadcastTXT(model.fieldsToJsonString().c_str());
                Serial.printf("[BACKEND] Added field 'delay' = %s\n", nf.getValue().c_str());
                delayAsString = nf.getValue();
            } else {
                delayAsString = f->getValue();
            }
            int delayAsInt = delayAsString.toInt() * 1000;
            Serial.println("[BACKEND] blocking delay of " + delayAsString + " started.");
            delay(delayAsInt);  // this is to test that we can still update from ui and serial immediatly even if this loop is blocked
            Serial.println("[BACKEND] blocking delay ended.");
        }
    }
};
unsigned long BackEnd::durationSinceRebootInSeconds = 0;

// ----------------------
// Setup
// ----------------------
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Booting...");

    WiFi.begin(getSsid(), getPass());
    Serial.print("Connecting to Wi-Fi");
    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 20) {
        delay(500);
        Serial.print(".");
        timeout++;
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("[WiFi] Connected.");
        Serial.print("[WiFi] IP: ");
        Serial.println(WiFi.localIP());
        if (MDNS.begin("bio"))
            Serial.println("[mDNS] Registered as bio.local");
        else
            Serial.println("[mDNS] Failed to start mDNS");
    } else
        Serial.println("[WiFi] Connection failed!");

    if (!SPIFFS.begin(false))
        Serial.println("[SPIFFS] Mount failed!");
    else
        Serial.println("[SPIFFS] Mounted successfully.");

    if (!model.load()) {
        Serial.println("[MODEL] Loading factory default");
        Field f1{"1", "Temperature", "float", "22.5", "Room temp", false};
        Field f2{"2", "Enabled", "bool", "true", "System enabled", false};
        Field f3{"3", "DeviceID", "string", "ESP32-001", "Identifier", true};
        Field f4{"4", "Count", "int", "42", "Event counter", false};
        model.add(f1);
        model.add(f2);
        model.add(f3);
        model.add(f4);
        model.saveToFile();
    }

    server.on("/", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/html", generateIndexPage(true)); });
    server.on("/info", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/html", generateIndexPage(false)); });
    server.on("/metadata", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/html", generateMetadataPage()); });
    server.on("/debug", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/html", generateDebugPage()); });
    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200,"text/plain","Rebooting..."); delay(100); ESP.restart(); });

    server.begin();

    webSocket.begin();
    webSocket.onEvent([](uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
        if (type == WStype_TEXT) {
            JsonDocument doc;
            deserializeJson(doc, payload);
            String jsonString;
            // Serialize the JsonDocument to the String
            serializeJson(doc, jsonString);
            Serial.println("webSocket.onEvent received an event of type text and the content turned to Jason is:" + jsonString);
            String action = doc["action"] | "";
            if (action == "update") {
                String id = doc["id"] | "";
                String val = doc["value"] | "";
                Field* f = model.getById(id);
                if (f && !f->getReadOnly()) {
                    f->setValue(val);
                    model.saveToFile();
                    Serial.printf("[WEB] Updated %s = %s\n", f->getName().c_str(), f->getValue().c_str());
                    webSocket.broadcastTXT(model.fieldsToJsonString().c_str());
                }
            } else if (action == "delete") {
                String id = doc["id"] | "";
                if (model.remove(id)) {
                    model.saveToFile();
                    webSocket.broadcastTXT(model.fieldsToJsonString().c_str());
                }
            } else if (action == "moveUp") {
                String id = doc["id"] | "";
                model.reorder(id, true);
                model.saveToFile();
                webSocket.broadcastTXT(model.fieldsToJsonString().c_str());
            } else if (action == "moveDown") {
                String id = doc["id"] | "";
                model.reorder(id, false);
                model.saveToFile();
                webSocket.broadcastTXT(model.fieldsToJsonString().c_str());
            } else if (action == "add") {
                JsonObject fld = doc["field"].as<JsonObject>();
                Field f;
                f.fromJson(fld);
                model.add(f);
                model.saveToFile();
                webSocket.broadcastTXT(model.fieldsToJsonString().c_str());
                Serial.printf("[WEB] Added new field %s (%s)\n", f.getName().c_str(), f.getType().c_str());
            }
        }
    });

    Serial.println("[SERIAL] Ready for commands (? , j , FieldName=value , F ... , delete FieldName)");

    // Setup backend
    BackEnd::setupBackend();
    xTaskCreatePinnedToCore(
        [](void*) { BackEnd::loopBackend(); },
        "BackendTask",
        4096,
        nullptr,
        1,
        nullptr,
        1  // <-- pinned to Core 1
    );
}

// ----------------------
// Loop
// ----------------------
void loop() {
    webSocket.loop();
    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line == "?")
            model.listSerial();
        else if (line == "j")
            Serial.println(model.fieldsToJsonString());
        else if (line.startsWith("F ")) {
            Field f;
            f.setReadOnly(false);
            int idx;
            idx = line.indexOf("name=");
            if (idx >= 0) {
                int e = line.indexOf(" ", idx);
                if (e < 0) e = line.length();
                f.setName(line.substring(idx + 5, e));
            }
            idx = line.indexOf("id=");
            if (idx >= 0) {
                int e = line.indexOf(" ", idx);
                if (e < 0) e = line.length();
                f.setId(line.substring(idx + 3, e));
            }
            idx = line.indexOf("type=");
            if (idx >= 0) {
                int e = line.indexOf(" ", idx);
                if (e < 0) e = line.length();
                f.setType(line.substring(idx + 5, e));
            }
            idx = line.indexOf("value=");
            if (idx >= 0) {
                int e = line.indexOf(" ", idx);
                if (e < 0) e = line.length();
                f.setValue(line.substring(idx + 6, e));
            }
            idx = line.indexOf("description=");
            if (idx >= 0) {
                int e = line.indexOf(" ", idx);
                if (e < 0) e = line.length();
                f.setDescription(line.substring(idx + 12, e));
            }
            idx = line.indexOf("readonly=");
            if (idx >= 0) {
                f.setReadOnly((line.substring(idx + 9, idx + 10) == "1"));
            }
            if (f.getName() != "") {
                model.add(f);
                model.saveToFile();
                Serial.printf("[SERIAL] Added field %s\n", f.getName().c_str());
                webSocket.broadcastTXT(model.fieldsToJsonString().c_str());
            }
        } else if (line.startsWith("delete ")) {
            String name = line.substring(7);
            Field* f = model.getByName(name);
            if (f) {
                model.remove(f->getId());
                model.saveToFile();
                Serial.printf("[SERIAL] Deleted field  %s\n", name.c_str());
                webSocket.broadcastTXT(model.fieldsToJsonString().c_str());
            }
        } else {
            int eq = line.indexOf('=');
            if (eq > 0) {
                String name = line.substring(0, eq);
                String val = line.substring(eq + 1);
                Field* f = model.getByName(name);
                if (f && !f->getReadOnly()) {
                    f->setValue(val);
                    model.saveToFile();
                    Serial.printf("[SERIAL] Updated %s = %s\n", f->getName().c_str(), f->getValue().c_str());
                    webSocket.broadcastTXT(model.fieldsToJsonString().c_str());
                }
            }
        }
    }
}
