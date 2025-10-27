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
// Field class - represents a single data element
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
// Model class - stores all fields, handles persistence
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
// Global instances
// -----------------------------------------------------------------------------
Model gModel;
AsyncWebServer server(80);
WebSocketsServer ws(81);

// -----------------------------------------------------------------------------
// Utility: broadcast model to all WebSocket clients
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
// Page generators
// -----------------------------------------------------------------------------
String generateIndex() {
    String html = "<html><head><title>Index</title></head><body>";
    html +=
        "<div><a href='/'>Index</a> | <a href='/metadata'>Metadata</a> | "
        "<a href='/debug'>Debug</a></div>";
    html += "<h1>Fields</h1><table border='1'><tr><th>Name</th><th>Type</th><th>Value</th></tr>";
    for (auto& f : gModel.fields()) {
        html += "<tr><td>" + f.getName() + "</td><td>" + f.getType() +
                "</td><td><input data-id=\"" + f.getId() + "\" value=\"" + f.getValue() +
                "\" onchange=\"updateField(this)\"></td></tr>";
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
ID:<input id="fid"> Name:<input id="fname"> Type:<input id="ftype"> 
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
// WebSocket event handler
// -----------------------------------------------------------------------------
void onWsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t len) {
    if (type != WStype_TEXT)
        return;
    JsonDocument doc;
    if (deserializeJson(doc, payload, len))
        return;
    String action = doc["action"].as<String>();
    if (action == "update") {
        String id = doc["id"], val = doc["value"];
        Field* f = gModel.getById(id);
        if (f) {
            f->setValue(val);
            gModel.save();
            broadcastModel();
            Serial.printf("[UPDATE] %s=%s\n", id.c_str(), val.c_str());
        }
    } else if (action == "add") {
        Field f(doc["id"], doc["name"], doc["type"], doc["value"], doc["description"]);
        gModel.addField(f);
        gModel.save();
        broadcastModel();
        Serial.printf("[ADD] %s added\n", f.getId().c_str());
    } else if (action == "delete") {
        String id = doc["id"];
        gModel.removeById(id);
        gModel.save();
        broadcastModel();
        Serial.printf("[DELETE] %s removed\n", id.c_str());
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
        Serial.println("[SYS] Rebooting on user request...");
        delay(500);
        ESP.restart();
    });

    server.begin();

    ws.begin();
    ws.onEvent(onWsEvent);

    Serial.printf("Root URL: http://%s/\n", WiFi.localIP().toString().c_str());
}

// -----------------------------------------------------------------------------
// Loop
// -----------------------------------------------------------------------------
void loop() { ws.loop(); }
