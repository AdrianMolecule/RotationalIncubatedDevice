#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WebSocketsServer.h>
#include <WiFi.h>

#include "pass.h"

// -----------------------------------------------------------------------------
// Field class
// -----------------------------------------------------------------------------
class Field {
    String id, name, type, value, description;

   public:
    Field() {}
    Field(String i, String n, String t, String v, String d)
        : id(i), name(n), type(t), value(v), description(d) {}

    String getId() const { return id; }
    String getName() const { return name; }
    String getType() const { return type; }
    String getValue() const { return value; }
    String getDescription() const { return description; }

    void setValue(const String& v) { value = v; }

    void toJson(JsonVariant obj) const {
        obj["id"] = id;
        obj["name"] = name;
        obj["type"] = type;
        obj["value"] = value;
        obj["description"] = description;
    }

    void fromJson(JsonVariant obj) {
        id = obj["id"].as<String>();
        name = obj["name"].as<String>();
        type = obj["type"].as<String>();
        value = obj["value"].as<String>();
        description = obj["description"].as<String>();
    }
};

// -----------------------------------------------------------------------------
// Model class
// -----------------------------------------------------------------------------
class Model {
    std::vector<Field> fields_;

   public:
    std::vector<Field>& fields() { return fields_; }

    Field* getById(const String& id) {
        for (auto& f : fields_)
            if (f.getId() == id)
                return &f;
        return nullptr;
    }

    Field* getByName(const String& name) {
        for (auto& f : fields_)
            if (f.getName().equalsIgnoreCase(name))
                return &f;
        return nullptr;
    }

    void addField(const Field& f) { fields_.push_back(f); }

    void removeById(const String& id) {
        fields_.erase(std::remove_if(fields_.begin(), fields_.end(),
                                     [&](const Field& f) { return f.getId() == id; }),
                      fields_.end());
    }

    void save() {
        JsonDocument doc;
        JsonArray arr = doc["fields"].to<JsonArray>();
        for (auto& f : fields_) {
            JsonObject o = arr.add<JsonObject>();
            f.toJson(o);
        }
        File file = SPIFFS.open("/model.json", "w");
        if (file) {
            serializeJson(doc, file);
            file.close();
            Serial.println("[SPIFFS] model.json saved.");
        } else {
            Serial.println("[SPIFFS] Failed to write model.json");
        }
    }

    bool load() {
        if (!SPIFFS.exists("/model.json")) {
            Serial.println("[SPIFFS] model.json missing");
            return false;
        }
        File file = SPIFFS.open("/model.json", "r");
        if (!file) {
            Serial.println("[SPIFFS] model.json open failed");
            return false;
        }
        JsonDocument doc;
        auto err = deserializeJson(doc, file);
        file.close();
        if (err) {
            Serial.printf("[SPIFFS] Deserialize failed: %s\n", err.c_str());
            return false;
        }
        fields_.clear();
        for (JsonObject o : doc["fields"].as<JsonArray>()) {
            Field f;
            f.fromJson(o);
            fields_.push_back(f);
        }
        if (fields_.empty()) {
            Serial.println("[SPIFFS] No fields in model.json");
            return false;
        }
        Serial.printf("[SPIFFS] Loaded %u fields.\n", (unsigned)fields_.size());
        return true;
    }
};

// -----------------------------------------------------------------------------
// Globals
// -----------------------------------------------------------------------------
Model gModel;
AsyncWebServer server(80);
WebSocketsServer ws(81);

// -----------------------------------------------------------------------------
// Factory default model
// -----------------------------------------------------------------------------
void factoryDefaultModel() {
    Serial.println("[MODEL] Initializing factory defaults...");
    gModel.fields().clear();
    gModel.addField(Field("F1", "Temperature", "float", "23.5", "Room temperature"));
    gModel.addField(Field("F2", "Switch", "bool", "1", "Light switch"));
    gModel.addField(Field("F3", "Status", "string", "OK", "Device status"));
    gModel.save();
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
void broadcastModel() {
    JsonDocument doc;
    JsonArray arr = doc["fields"].to<JsonArray>();
    for (auto& f : gModel.fields()) {
        JsonObject o = arr.add<JsonObject>();
        f.toJson(o);
    }
    String payload;
    serializeJson(doc, payload);
    ws.broadcastTXT(payload);
}

void listFields() {
    Serial.println(F("\n[MODEL] Fields:"));
    for (auto& f : gModel.fields()) {
        Serial.printf("  %s (%s): %s | Value: %s\n",
                      f.getName().c_str(),
                      f.getType().c_str(),
                      f.getDescription().c_str(),
                      f.getValue().c_str());
    }
    Serial.println(F("Use <FieldName>=<Value> to modify a value.\n"));
}
void dumpJson() {
    File file = SPIFFS.open("/model.json", "r");
    if (!file) {
        Serial.println("[SPIFFS] Cannot open model.json");
        return;
    }
    Serial.println(F("\n[JSON] Dump:"));
    while (file.available())
        Serial.write(file.read());
    Serial.println();
    file.close();
}

// -----------------------------------------------------------------------------
// Validation
// -----------------------------------------------------------------------------
bool validateFieldValue(const Field& f, const String& val) {
    if (f.getType() == "int") {
        for (char c : val)
            if (!isdigit(c) && c != '-' && c != '+') return false;
    } else if (f.getType() == "float") {
        bool dot = false;
        for (char c : val) {
            if (c == '.') {
                if (dot) return false;
                dot = true;
            } else if (!isdigit(c) && c != '-' && c != '+')
                return false;
        }
    } else if (f.getType() == "bool") {
        if (!(val == "0" || val == "1")) return false;
    }
    return true;  // string always valid
}

// -----------------------------------------------------------------------------
// Page generators
// -----------------------------------------------------------------------------
String generateIndex() {
    String html = "<html><head><title>Index</title></head><body>";
    html +=
        "<div><a href='/'>Index</a> | <a href='/metadata'>Metadata</a> | "
        "<a href='/debug'>Debug</a></div>";
    html += "<h1>Fields</h1><table border='1'><tr><th>Name</th><th>Type</th><th>Value</th></tr>";
    for (auto& f : gModel.fields()) {
        html += "<tr><td>" + f.getName() + "</td><td>" + f.getType() + "</td><td>";
        if (f.getType() == "bool") {
            html += "<select data-id=\"" + f.getId() + "\" onchange=\"updateField(this)\">";
            html += "<option value='1'" + String(f.getValue() == "1" ? " selected" : "") + ">true</option>";
            html += "<option value='0'" + String(f.getValue() == "0" ? " selected" : "") + ">false</option>";
            html += "</select>";
        } else {
            html += "<input type='" + String((f.getType() == "int" || f.getType() == "float") ? "number" : "text") +
                    "' data-id=\"" + f.getId() + "\" value=\"" + f.getValue() +
                    "\" onchange=\"updateField(this)\">";
        }
        html += "</td></tr>";
    }
    html += R"rawliteral(
</table>
<script>
var ws=new WebSocket('ws://'+location.hostname+':81/');
function updateField(el){
  ws.send(JSON.stringify({action:'update',id:el.dataset.id,value:el.value}));
}
</script>
</body></html>
)rawliteral";
    return html;
}

String generateMetadata() {
    String html = "<html><head><title>Metadata</title></head><body>";
    html +=
        "<div><a href='/'>Index</a> | <a href='/metadata'>Metadata</a> | "
        "<a href='/debug'>Debug</a></div>";
    html +=
        "<h1>Metadata</h1><table border='1'><tr><th>ID</th><th>Name</th><th>Type</th>"
        "<th>Value</th><th>Description</th><th>Action</th></tr>";
    for (auto& f : gModel.fields()) {
        html += "<tr><td>" + f.getId() + "</td><td>" + f.getName() + "</td><td>" + f.getType() +
                "</td><td>" + f.getValue() + "</td><td>" + f.getDescription() +
                "</td><td><button onclick=\"delField('" + f.getId() + "')\">Delete</button></td></tr>";
    }
    html += R"rawliteral(
</table>
<h2>Add Field</h2>
ID:<input id="fid"> Name:<input id="fname"> Type:<select id="ftype">
<option value="string">string</option><option value="int">int</option>
<option value="float">float</option><option value="bool">bool</option></select>
Value:<input id="fvalue"> Desc:<input id="fdesc">
<button onclick="addField()">Add</button>
<script>
var ws=new WebSocket('ws://'+location.hostname+':81/');
function addField(){
  ws.send(JSON.stringify({
    action:'add',
    id:fid.value,name:fname.value,type:ftype.value,
    value:fvalue.value,description:fdesc.value
  }));
  location.reload();
}
function delField(id){
  ws.send(JSON.stringify({action:'delete',id:id}));
  location.reload();
}
</script>
</body></html>
)rawliteral";
    return html;
}

String generateDebug() {
    String html = "<html><head><title>Debug</title></head><body>";
    html +=
        "<div><a href='/'>Index</a> | <a href='/metadata'>Metadata</a> | "
        "<a href='/debug'>Debug</a></div>";
    html += "<h1>Debug</h1>";
    html += "<p>WiFi: " + String(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected") +
            "</p><p>IP: " + WiFi.localIP().toString() + "</p>";
    html += "<h2>SPIFFS files</h2><ul>";
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while (file) {
        html += "<li>" + String(file.name()) + " (" + file.size() + " bytes)</li>";
        file = root.openNextFile();
    }
    html += "</ul><h2>model.json contents</h2><pre>";
    File f = SPIFFS.open("/model.json", "r");
    if (f) {
        while (f.available()) html += (char)f.read();
        f.close();
    }
    html += R"rawliteral(</pre>
<button onclick="fetch('/reboot')">Reboot</button>
<script>setTimeout(()=>location.reload(),5000);</script>
</body></html>)rawliteral";
    return html;
}

// -----------------------------------------------------------------------------
// WebSocket handler
// -----------------------------------------------------------------------------
void onWsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t len) {
    if (type != WStype_TEXT) return;
    JsonDocument doc;
    if (deserializeJson(doc, payload, len)) return;
    String action = doc["action"].as<String>();
    if (action == "update") {
        String id = doc["id"], val = doc["value"];
        Field* f = gModel.getById(id);
        if (f) {
            if (validateFieldValue(*f, val)) {
                f->setValue(val);
                gModel.save();
                broadcastModel();
                Serial.printf("[WEB] Updated %s = %s\n", f->getName().c_str(), val.c_str());
            } else {
                Serial.printf("[VALIDATION] Invalid value for %s: %s\n", f->getName().c_str(), val.c_str());
            }
        }
    } else if (action == "add") {
        Field f(doc["id"], doc["name"], doc["type"], doc["value"], doc["description"]);
        gModel.addField(f);
        gModel.save();
        broadcastModel();
        Serial.printf("[WEB] Added %s\n", f.getName().c_str());
    } else if (action == "delete") {
        String id = doc["id"];
        Field* f = gModel.getById(id);
        if (f) Serial.printf("[WEB] Deleted %s\n", f->getName().c_str());
        gModel.removeById(id);
        gModel.save();
        broadcastModel();
    }
}

// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    Serial.println("\nBooting...");

    WiFi.mode(WIFI_STA);
    WiFi.begin(getSsid(), getPass());
    Serial.print("[WiFi] Connecting");
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
        delay(500);
        Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED)
        Serial.printf("\n[WiFi] Connected: %s\n", WiFi.localIP().toString().c_str());
    else
        Serial.println("\n[WiFi] Failed to connect");

    if (MDNS.begin("ad"))
        Serial.println("[mDNS] Registered as ad.local");
    else
        Serial.println("[mDNS] mDNS failed");

    if (SPIFFS.begin())
        Serial.println("[SPIFFS] Mounted");
    else
        Serial.println("[SPIFFS] Mount failed!");

    if (!gModel.load())
        factoryDefaultModel();

    server.on("/", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/html", generateIndex()); });
    server.on("/metadata", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/html", generateMetadata()); });
    server.on("/debug", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/html", generateDebug()); });
    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest* r) {
        r->send(200, "text/plain", "Rebooting...");
        Serial.println("[SYS] Rebooting...");
        delay(500);
        ESP.restart();
    });

    server.begin();
    ws.begin();
    ws.onEvent(onWsEvent);

    Serial.printf("Root URL: http://%s/\n", WiFi.localIP().toString().c_str());
    Serial.println("Serial commands: ? = list, j = dump json, <FieldName>=<Value> = update");
}

// -----------------------------------------------------------------------------
// Loop
// -----------------------------------------------------------------------------
void loop() {
    ws.loop();
    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line == "?")
            listFields();
        else if (line == "j")
            dumpJson();
        else if (line.indexOf('=') > 0) {
            String name = line.substring(0, line.indexOf('='));
            String val = line.substring(line.indexOf('=') + 1);
            Field* f = gModel.getByName(name);
            if (f) {
                if (validateFieldValue(*f, val)) {
                    f->setValue(val);
                    gModel.save();
                    broadcastModel();
                    Serial.printf("[SERIAL] Updated %s = %s\n", f->getName().c_str(), val.c_str());
                } else {
                    Serial.printf("[VALIDATION] Invalid value for %s: %s\n", f->getName().c_str(), val.c_str());
                }
            } else
                Serial.printf("[SERIAL] Field not found: %s\n", name.c_str());
        }
    }
}
