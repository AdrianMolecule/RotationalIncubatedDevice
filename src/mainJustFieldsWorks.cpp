#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>

#include <vector>

#include "Field.h"
#include "JsonWrapper.h"
#include "Helper.h"
#include "pass.h"

AsyncWebServer server(80);
AsyncWebSocket webSocket("/ws");

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
    void add(const Field& f) { fields.push_back(f); }
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
            Field field;
            field.fromJson(fld);
            fields.push_back(field);
        }
        return !fields.empty();
    }
    bool saveToFile() { return JsonWrapper::saveModelToFile(fields); }
    String toJson() { return JsonWrapper::fieldsToJsonString(fields); }
    void listSerial() {
        for (auto& f : fields) Serial.printf("%s (%s) = %s\n", f.getName().c_str(), f.getType().c_str(), f.getValue().c_str());
    }
};

Model model;

String generateMenu() { return "<p><a href='/'>Index</a> | <a href='/info'>Info</a> | <a href='/metadata'>Metadata</a> | <a href='/debug'>Debug</a> | <a href='/reboot'>Reboot</a></p>"; }

String generateIndexPage(bool brief) {
    String html = generateMenu();
    if (brief) {
        html += "<h1>Index Page</h1>";
    } else {
        html += "<h1>Info Page</h1>";
    }
    html += "<table border=1><tr><th>Name</th><th>Type</th><th>Value</th><th>Description</th></tr>";
    for (auto& f : model.fields) {
        if (!brief && f.getReadOnly()) continue;
        html += "<tr><td>" + f.getName() + "</td><td>" + f.getType() + "</td>";
        if (f.getReadOnly())
            html += "<td><input value='" + f.getValue() + "' disabled></td>";
        else
            html += "<td><input data-id='" + f.getId() + "' value='" + f.getValue() + "' onchange='onChange(this)'></td>";
        html += "<td>" + f.getDescription() + "</td></tr>";
    }
    html += R"(<script>
var ws = new WebSocket('ws://' + location.hostname + '/ws');
ws.onmessage = function(evt) {
    try {
        var data = JSON.parse(evt.data);
        if (!Array.isArray(data)) return;
        data.forEach(f => {
            var el = document.querySelector("input[data-id='" + f.id + "']");
            if (el) 
                el.value = f.value;
        });
    } catch(e) {
        console.error("WS update error:", e);
    }
};
function onChange(el){
    var val = el.value;
    var id = el.getAttribute('data-id');
    ws.send(JSON.stringify({action:'update',id:id,value:val}));
}
</script>)";
    return html;
}
String generateMetadataPage() {
    String html;
    html += "<h1>Metadata</h1>";
    html += "<p><a href='/'>Index</a> | <a href='/info'>Info</a> | <a href='/debug'>Debug</a></p>";
    html += "<table border=1><thead><tr><th>Id</th><th>Name</th><th>Type</th><th>Value</th><th>Description</th><th>ReadOnly</th><th>Reorder</th><th>Delete</th></tr></thead><tbody id='meta-body'>";
    for (auto& f : model.fields) {
        html += "<tr>";
        html += "<td>" + f.getId() + "</td>";
        html += "<td>" + f.getName() + "</td>";
        html += "<td>" + f.getType() + "</td>";
        html += "<td>" + f.getValue() + "</td>";
        html += "<td>" + f.getDescription() + "</td>";
        html += "<td>" + String(f.getReadOnly()) + "</td>";
        html += "<td><button onclick='reorder(\"" + f.getId() + "\",true)'>&#9650;</button><button onclick='reorder(\"" + f.getId() + "\",false)'>&#9660;</button></td>";
        html += "<td><button onclick='delField(\"" + f.getId() + "\")'>Delete</button></td>";
        html += "</tr>";
    }
    html += "</tbody></table>";
    html += "<h3>Add New Field</h3>";
    html += "ID: <input id='fid'><br>";
    html += "Name: <input id='fname'><br>";
    html += "Type: <input id='ftype'><br>";
    html += "Value: <input id='fvalue'><br>";
    html += "Description: <input id='fdesc'><br>";
    html += "ReadOnly: <input id='freadonly' type='checkbox'><br>";
    html += "<button onclick='addField()'>Add Field</button>";
    html += R"rawliteral(
<script>
var ws = new WebSocket('ws://' + location.hostname + '/ws');
ws.onmessage = function(evt){
    try{
        var data = JSON.parse(evt.data);
        if(!Array.isArray(data)) return;
        var tbody = document.querySelector('#meta-body');
        if(!tbody) return;
        tbody.innerHTML = "";
        data.forEach(f=>{
            var row = document.createElement("tr");
            row.innerHTML =
                "<td>"+f.id+"</td>"+
                "<td>"+f.name+"</td>"+
                "<td>"+f.type+"</td>"+
                "<td>"+f.value+"</td>"+
                "<td>"+f.description+"</td>"+
                "<td>"+f.readOnly+"</td>"+
                "<td><button onclick='reorder(\""+f.id+"\",true)'>&#9650;</button>"+
                "<button onclick='reorder(\""+f.id+"\",false)'>&#9660;</button></td>"+
                "<td><button onclick='delField(\""+f.id+"\")'>Delete</button></td>";
            tbody.appendChild(row);
        });
    }catch(e){console.error(e);}
};
function delField(id){ws.send(JSON.stringify({action:'delete',id:id}));}
function reorder(id,up){ws.send(JSON.stringify({action:up?'moveUp':'moveDown',id:id}));}
function addField(){
    var fid=document.getElementById('fid').value.trim();
    if(!fid){
        var maxId=0;
        document.querySelectorAll('#meta-body tr td:first-child').forEach(td=>{
            var n=parseInt(td.innerText);
            if(!isNaN(n)&&n>maxId) maxId=n;
        });
        fid=(maxId+1).toString();
        document.getElementById('fid').value=fid;
    }
    var msg={action:'add',field:{
        id:fid,
        name:document.getElementById('fname').value,
        type:document.getElementById('ftype').value,
        value:document.getElementById('fvalue').value,
        description:document.getElementById('fdesc').value,
        readOnly:document.getElementById('freadonly').checked
    }};
    ws.send(JSON.stringify(msg));
    // clear inputs after sending
    document.getElementById('fid').value="";
    document.getElementById('fname').value="";
    document.getElementById('ftype').value="";
    document.getElementById('fvalue').value="";
    document.getElementById('fdesc').value="";
    document.getElementById('freadonly').checked=false;
}
</script>
)rawliteral";

    return html;
}

String generateDebugPage() { return generateMenu() + "<h1>Debug</h1><pre>" + model.toJson() + "</pre>"; }

void handleWebSocketMessage(String msg) {  // from the UI
    JsonDocument doc;
    if (deserializeJson(doc, msg)) return;
    String action = doc["action"] | "";
    if (action == "update") {
        String id = doc["id"] | "";
        String val = doc["value"] | "";
        Field* f = model.getById(id);
        if (f && !f->getReadOnly()) {
            f->setValue(val);
            model.saveToFile();
            webSocket.textAll(model.toJson());
        }
    } else if (action == "delete") {
        String id = doc["id"] | "";
        if (model.remove(id)) {
            model.saveToFile();
            webSocket.textAll(model.toJson());
        }
    } else if (action == "moveUp" || action == "moveDown") {
        String id = doc["id"] | "";
        model.reorder(id, action == "moveUp");
        model.saveToFile();
        webSocket.textAll(model.toJson());
    } else if (action == "add") {
        JsonObject fld = doc["field"].as<JsonObject>();
        Field f;
        f.fromJson(fld);
        model.add(f);
        model.saveToFile();
        webSocket.textAll(model.toJson());
    }
}

class BackEnd {
   public:
    static unsigned long durationSinceRebootInSeconds;
    static void setupBackend() {
        durationSinceRebootInSeconds = 0;
        Serial.println("[BACKEND] Backend initialized.");
    }
    static void loopBackend() {
        while (true) {
            durationSinceRebootInSeconds = millis() / 1000;
            static unsigned long lastModelUpdateInSeconds = 0;
            if (durationSinceRebootInSeconds - lastModelUpdateInSeconds >= 5) {
                lastModelUpdateInSeconds = durationSinceRebootInSeconds;
                Field* f = model.getByName("duration");
                if (!f) {
                    Field nf("10", "duration", "string", String(durationSinceRebootInSeconds), "time since start", false);
                    model.add(nf);
                } else
                    f->setValue(String(durationSinceRebootInSeconds));
                model.saveToFile();
                webSocket.textAll(model.toJson());
            }
            Field* f = model.getByName("delay");
            String delayAsString;
            if (!f) {
                Field nf("11", "delay", "string", "7", "blocking delay in backend", false);
                model.add(nf);
                model.saveToFile();
                webSocket.textAll(model.toJson());
                delayAsString = nf.getValue();
            } else {
                delayAsString = f->getValue();
            }
            int delayAsInt = delayAsString.toInt() * 1000;
            Serial.println("[BACKEND] Blocking delay " + delayAsString + "s");
            webSocket.textAll(model.toJson());
            delay(delayAsInt);
        }
    }
};
unsigned long BackEnd::durationSinceRebootInSeconds = 0;
// NTP Server settings
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -18000;    // Set your GMT offset in seconds (e.g., EST is -5 hours * 3600 seconds/hour = -18000)
const int daylightOffset_sec = 3600;  // Set your daylight saving offset in seconds (e.g., 1 hour = 3600 seconds)

void setup() {
    Serial.begin(115200);
    Serial.println("[SYS] Booting...");
    WiFi.begin(getSsid(), getPass());
    Serial.printf("[WiFi] Connecting to %s ", getSsid());
    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 20) {
        delay(500);
        Serial.print(".");
        timeout++;
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        // Initialize and get the time from NTP server
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

        Serial.println(" at:"+Helper::getTime());
        if (MDNS.begin("bio"))
            Serial.println("[mDNS] Registered as bio.local");
        else
            Serial.println("[mDNS] Failed to start mDNS");
    } else
        Serial.println("[WiFi] Connection failed!");
    if (!SPIFFS.begin(false))
        Serial.println("[FS] Mount failed!");
    else
        Serial.println("[FS] Mounted successfully.");
    if (!model.load()) {
        Serial.println("[MODEL] Loading defaults");
        Field f1{"1", "Temperature", "float", "22.5", "Room temp", false};
        Field f2{"2", "Enabled", "bool", "true", "System enabled", false};
        Field f3{"3", "DeviceID", "string", "ESP32-001", "Identifier", true};
        Field f4{"4", "x", "int", "0", "Event counter", false};
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
    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest* r) {r->send(200,"text/plain","Rebooting...");delay(100);ESP.restart(); });
    webSocket.onEvent([](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {if(type==WS_EVT_DATA){String msg;for(size_t i=0;i<len;i++)msg+=(char)data[i];handleWebSocketMessage(msg);} });
    server.addHandler(&webSocket);
    server.begin();
    BackEnd::setupBackend();
    xTaskCreatePinnedToCore([](void*) { BackEnd::loopBackend(); }, "BackendTask", 4096, nullptr, 1, nullptr, 1);
    Serial.println("[SYS] Setup complete.");
}

// -- -- -- -- -- -- -- -- -- -- --
// Loop
// ----------------------
void loop() {
    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line == "m")
            model.listSerial();
        else if (line == "j")
            Serial.println(model.toJson());
        else if (line.startsWith("upload ")) {
            String jsonStr = line.substring(line.indexOf(' ') + 1);
            jsonStr.trim();
            if (JsonWrapper::jsonToFields(jsonStr, model.fields)) {
                JsonWrapper::saveModelToFile(model.fields);
                Serial.println("New replaced Json:" + String(model.toJson()));
            } else {
                Serial.println("New entered Json does not parse so Model remained unchanged");
            }
            webSocket.textAll(model.toJson());
        } else if (line.startsWith("add ")) {
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
            } else {
                Serial.println("TODO should default the id if not present int the field");
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
                webSocket.textAll(model.toJson());
            }
        } else if (line.startsWith("delete ")) {
            String name = line.substring(7);
            Field* f = model.getByName(name);
            if (f) {
                model.remove(f->getId());
                model.saveToFile();
                Serial.printf("[SERIAL] Deleted field  %s\n", name.c_str());
                webSocket.textAll(model.toJson());
            }
        } else if (line.startsWith("?")) {
            Serial.println("\n--- Available Serial Commands ---");
            Serial.println("? or help          : Display this help message");
            Serial.println("m                  : List current model fields details");
            Serial.println("j                  : Print full model as JSON string");
            Serial.println("upload <json_str>  : Replace entire model with new JSON data");
            Serial.println("add name=... id=...: Add a new field (supply all params)");
            Serial.println("delete <name>      : Delete a field by name");
            Serial.println("<name>=<value>     : Update the value of an existing field");
            Serial.println("-----------------------------------\n");
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
                    webSocket.textAll(model.toJson());
                }
            }
        }
    }
}
