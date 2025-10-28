#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <WebSocketsServer.h>
#include <WiFi.h>

#include "pass.h"  // assumes getSsid() and getPass() exist

// -----------------------------------------------------------------------------
// Field Class
// -----------------------------------------------------------------------------
class Field {
    String id, name, type, value, description;
    bool readOnly = false;

   public:
    Field() {}
    Field(String i, String n, String t, String v, String d, bool ro = false)
        : id(i), name(n), type(t), value(v), description(d), readOnly(ro) {}

    String getId() const { return id; }
    String getName() const { return name; }
    String getType() const { return type; }
    String getValue() const { return value; }
    String getDescription() const { return description; }
    bool isReadOnly() const { return readOnly; }

    void setValue(const String& v) { value = v; }

    void toJson(JsonVariant obj) const {
        obj["id"] = id;
        obj["name"] = name;
        obj["type"] = type;
        obj["value"] = value;
        obj["description"] = description;
        obj["readOnly"] = readOnly;
    }

    void fromJson(JsonVariant obj) {
        id = obj["id"].as<String>();
        name = obj["name"].as<String>();
        type = obj["type"].as<String>();
        value = obj["value"].as<String>();
        description = obj["description"].as<String>();
        readOnly = obj["readOnly"] | false;
    }
};

// -----------------------------------------------------------------------------
// Model Class
// -----------------------------------------------------------------------------
class Model {
    std::vector<Field> _fields;

   public:
    std::vector<Field>& fields() { return _fields; }

    bool load() {
        if (!SPIFFS.exists("/model.json")) return false;
        File f = SPIFFS.open("/model.json", "r");
        if (!f) return false;
        String json = f.readString();
        f.close();
        if (json.isEmpty()) return false;

        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, json);
        if (err) {
            Serial.println("[SPIFFS] JSON parse failed");
            return false;
        }

        _fields.clear();
        for (JsonObject obj : doc["fields"].as<JsonArray>()) {
            Field fld;
            fld.fromJson(obj);
            _fields.push_back(fld);
        }

        if (_fields.empty()) {
            Serial.println("[SPIFFS] No fields in model.json");
            return false;
        }

        Serial.printf("[SPIFFS] Loaded model.json successfully with %d fields\n", (int)_fields.size());
        return true;
    }

    void save() {
        JsonDocument doc;
        JsonArray arr = doc["fields"].to<JsonArray>();
        for (auto& f : _fields) {
            JsonObject o = arr.add<JsonObject>();
            f.toJson(o);
        }
        File f = SPIFFS.open("/model.json", "w");
        if (!f) {
            Serial.println("[SPIFFS] Failed to save model.json");
            return;
        }
        serializeJson(doc, f);
        f.close();
        Serial.println("[SPIFFS] Model saved");
    }

    Field* getById(const String& id) {
        for (auto& f : _fields)
            if (f.getId() == id) return &f;
        return nullptr;
    }

    Field* getByName(const String& name) {
        for (auto& f : _fields)
            if (f.getName() == name) return &f;
        return nullptr;
    }

    void add(const Field& f) { _fields.push_back(f); }
    void remove(const String& id) {
        _fields.erase(std::remove_if(_fields.begin(), _fields.end(),
                                     [&](Field& f) { return f.getId() == id; }),
                      _fields.end());
    }
};

Model gModel;
AsyncWebServer server(80);
WebSocketsServer ws(81);

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
bool validateFieldValue(const Field& f, const String& val) {
    if (f.getType() == "int") return val.toInt() || val == "0";
    if (f.getType() == "float") return val.toFloat() || val == "0" || val == "0.0";
    if (f.getType() == "bool") return val == "1" || val == "0" || val == "true" || val == "false";
    return true;
}

void listFields() {
    Serial.println(F("\n[MODEL] Fields:"));
    for (auto& f : gModel.fields()) {
        Serial.printf("  %s (%s): %s | Value: %s | ReadOnly: %s\n",
                      f.getName().c_str(), f.getType().c_str(), f.getDescription().c_str(),
                      f.getValue().c_str(), f.isReadOnly() ? "Yes" : "No");
    }
    Serial.println(F("Use <FieldName>=<Value> to modify a value.\n"));
}

void dumpJson() {
    File f = SPIFFS.open("/model.json", "r");
    if (!f) {
        Serial.println("[SPIFFS] model.json missing");
        return;
    }
    while (f.available()) Serial.write(f.read());
    f.close();
    Serial.println();
}

void broadcastModel() {
    JsonDocument doc;
    JsonArray arr = doc["fields"].to<JsonArray>();
    for (auto& f : gModel.fields()) {
        JsonObject o = arr.add<JsonObject>();
        f.toJson(o);
    }
    String json;
    serializeJson(doc, json);
    ws.broadcastTXT(json);
}

// -----------------------------------------------------------------------------
// WebSocket
// -----------------------------------------------------------------------------
void onWsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_TEXT) {
        String msg = String((char*)payload).substring(0, length);
        JsonDocument doc;
        if (deserializeJson(doc, msg)) return;
        String action = doc["action"].as<String>();

        if (action == "update") {
            String id = doc["id"].as<String>(), val = doc["value"].as<String>();
            Field* f = gModel.getById(id);
            if (f && !f->isReadOnly()) {
                if (validateFieldValue(*f, val)) {
                    f->setValue(val);
                    gModel.save();
                    broadcastModel();
                    Serial.printf("[WEB] Updated %s = %s\n", f->getName().c_str(), val.c_str());
                }
            }
        } else if (action == "add") {
            Field f;
            f.fromJson(doc);
            gModel.add(f);
            gModel.save();
            broadcastModel();
            Serial.printf("[WEB] Added field %s\n", f.getName().c_str());
        } else if (action == "delete") {
            String id = doc["id"].as<String>();
            gModel.remove(id);
            gModel.save();
            broadcastModel();
            Serial.printf("[WEB] Deleted field ID=%s\n", id.c_str());
        }
    }
}

// -----------------------------------------------------------------------------
// Factory Default
// -----------------------------------------------------------------------------
void factoryDefaultModel() {
    Serial.println("[MODEL] Creating factory default model...");
    gModel.fields().clear();
    gModel.add(Field("1", "Temperature", "float", "22.5", "Room temperature", false));
    gModel.add(Field("2", "Enabled", "bool", "1", "System enabled", false));
    gModel.add(Field("3", "DeviceID", "string", "ESP32-001", "Identifier", true));
    gModel.add(Field("4", "Count", "int", "42", "Event counter", false));
    gModel.save();
    Serial.println("[MODEL] Factory default model created.");
}

// -----------------------------------------------------------------------------
// HTML Generators
// -----------------------------------------------------------------------------
String generateIndex() {
    String html = "<html><head><title>Index</title></head><body>";
    html +=
        "<div><a href='/'>Index</a> | <a href='/metadata'>Metadata</a> | "
        "<a href='/debug'>Debug</a></div><h1>Index</h1><table border='1'>";
    for (auto& f : gModel.fields()) {
        String disabled = f.isReadOnly() ? "disabled" : "";
        html += "<tr><td>" + f.getName() + "</td><td>" + f.getType() + "</td><td>";
        if (f.getType() == "bool") {
            html += "<select data-id='" + f.getId() + "' onchange='updateField(this)' " + disabled + ">";
            html += "<option value='1'" + String(f.getValue() == "1" ? " selected" : "") + ">true</option>";
            html += "<option value='0'" + String(f.getValue() == "0" ? " selected" : "") + ">false</option>";
            html += "</select>";
        } else {
            html += "<input type='" + String((f.getType() == "int" || f.getType() == "float") ? "number" : "text") +
                    "' data-id='" + f.getId() + "' value='" + f.getValue() + "' onchange='updateField(this)' " + disabled + ">";
        }
        html += "</td></tr>";
    }
    html += "</table>";

    html += R"rawliteral(
<script>
var ws=new WebSocket('ws://'+location.hostname+':81/');
ws.onmessage=function(event){
  var data=JSON.parse(event.data);
  if(!data.fields) return;
  data.fields.forEach(f=>{
    var el=document.querySelector("[data-id='"+f.id+"']");
    if(el){ el.value=f.value; el.disabled=f.readOnly; }
  });
};
function updateField(el){
  ws.send(JSON.stringify({action:'update',id:el.dataset.id,value:el.value}));
}
</script></body></html>
)rawliteral";
    return html;
}

String generateMetadata() {
    String html = "<html><head><title>Metadata</title></head><body>";
    html +=
        "<div><a href='/'>Index</a> | <a href='/metadata'>Metadata</a> | "
        "<a href='/debug'>Debug</a></div>";
    html += "<h1>Metadata</h1><table border='1'><tr><th>Name</th><th>Type</th><th>Description</th><th>ReadOnly</th><th>Delete</th></tr>";

    for (auto& f : gModel.fields()) {
        html += "<tr><td>" + f.getName() + "</td><td>" + f.getType() + "</td><td>" +
                f.getDescription() + "</td><td>" + String(f.isReadOnly() ? "Yes" : "No") +
                "</td><td><button onclick=\"deleteField('" + f.getId() + "')\">Delete</button></td></tr>";
    }

    html += "</table><h2>Add New Field</h2>";
    html += "<input id='fid' placeholder='ID'>";
    html += "<input id='fname' placeholder='Name'>";
    html += "<input id='ftype' placeholder='Type (int,float,bool,string)'>";
    html += "<input id='fvalue' placeholder='Value'>";
    html += "<input id='fdesc' placeholder='Description'>";
    html += "ReadOnly: <input type='checkbox' id='freadonly'>";
    html += "<button onclick='addField()'>Add Field</button>";

    html += R"rawliteral(
<script>
var ws=new WebSocket('ws://'+location.hostname+':81/');
ws.onmessage=function(event){
  var data=JSON.parse(event.data);
  if(!data.fields) return;
  var table="<tr><th>Name</th><th>Type</th><th>Description</th><th>ReadOnly</th><th>Delete</th></tr>";
  data.fields.forEach(f=>{
    table+="<tr><td>"+f.name+"</td><td>"+f.type+"</td><td>"+f.description+"</td><td>"+(f.readOnly?"Yes":"No")+"</td>"+
           "<td><button onclick='deleteField(\""+f.id+"\")'>Delete</button></td></tr>";
  });
  document.querySelector("table").innerHTML=table;
};
function deleteField(id){
  ws.send(JSON.stringify({action:'delete',id:id}));
}
function addField(){
  var f={
    action:'add',
    id:fid.value,name:fname.value,type:ftype.value,
    value:fvalue.value,description:fdesc.value,
    readOnly:freadonly.checked
  };
  ws.send(JSON.stringify(f));
}
</script></body></html>
)rawliteral";
    return html;
}

String generateDebug() {
    String html = "<html><head><title>Debug</title></head><body>";
    html +=
        "<div><a href='/'>Index</a> | <a href='/metadata'>Metadata</a> | "
        "<a href='/debug'>Debug</a></div>";
    html += "<h1>Debug</h1><pre>";
    File file = SPIFFS.open("/model.json", "r");
    if (file) {
        while (file.available()) html += (char)file.read();
        file.close();
    } else
        html += "[SPIFFS] model.json missing";
    html += "</pre><br><button onclick='fetch(\"/reboot\").then(()=>alert(\"Rebooting\"))'>Reboot ESP32</button>";
    html += "</body></html>";
    return html;
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
        Serial.println("[SPIFFS] Mounted successfully.");
    else
        Serial.println("[SPIFFS] Mount failed!");

    if (!gModel.load()) factoryDefaultModel();

    server.on("/", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/html", generateIndex()); });
    server.on("/metadata", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/html", generateMetadata()); });
    server.on("/debug", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/html", generateDebug()); });
    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest* r) {r->send(200,"text/plain","Rebooting...");delay(500);ESP.restart(); });

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
                } else
                    Serial.printf("[VALIDATION] Invalid value for %s: %s\n", f->getName().c_str(), val.c_str());
            } else
                Serial.printf("[SERIAL] Field not found: %s\n", name.c_str());
        }
    }
}
