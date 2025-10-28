#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <WebSocketsServer.h>
#include <WiFi.h>

#include "pass.h"  // provides getSsid() and getPass()

// --------------------------- Field ------------------------------------------
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
    void setReadOnly(bool ro) { readOnly = ro; }

    void toJson(JsonVariant obj) const {
        obj["id"] = id;
        obj["name"] = name;
        obj["type"] = type;
        obj["value"] = value;
        obj["description"] = description;
        obj["readOnly"] = readOnly;
    }

    void fromJson(JsonVariant obj) {
        if (obj.containsKey("id")) id = obj["id"].as<String>();
        if (obj.containsKey("name")) name = obj["name"].as<String>();
        if (obj.containsKey("type")) type = obj["type"].as<String>();
        if (obj.containsKey("value")) value = obj["value"].as<String>();
        if (obj.containsKey("description")) description = obj["description"].as<String>();
        if (obj.containsKey("readOnly")) readOnly = obj["readOnly"].as<bool>();
    }
};

// --------------------------- Model ------------------------------------------
class Model {
    std::vector<Field> fields_;

   public:
    std::vector<Field>& fields() { return fields_; }

    Field* getById(const String& id) {
        for (auto& f : fields_)
            if (f.getId() == id) return &f;
        return nullptr;
    }

    Field* getByName(const String& name) {
        for (auto& f : fields_)
            if (f.getName().equalsIgnoreCase(name)) return &f;
        return nullptr;
    }

    void addField(const Field& f) { fields_.push_back(f); }

    void removeById(const String& id) {
        fields_.erase(std::remove_if(fields_.begin(), fields_.end(),
                                     [&](const Field& f) { return f.getId() == id; }),
                      fields_.end());
    }

    void reorder(const String& id, bool up) {
        for (size_t i = 0; i < fields_.size(); ++i) {
            if (fields_[i].getId() == id) {
                if (up && i > 0)
                    std::swap(fields_[i], fields_[i - 1]);
                else if (!up && i + 1 < fields_.size())
                    std::swap(fields_[i], fields_[i + 1]);
                return;
            }
        }
    }

    bool load() {
        if (!SPIFFS.exists("/model.json")) {
            Serial.println("[SPIFFS] model.json missing");
            return false;
        }
        File f = SPIFFS.open("/model.json", "r");
        if (!f) return false;
        String j = f.readString();
        f.close();
        if (j.isEmpty()) return false;

        JsonDocument doc;
        if (deserializeJson(doc, j)) return false;

        fields_.clear();
        for (JsonObject obj : doc["fields"].as<JsonArray>()) {
            Field fld;
            fld.fromJson(obj);
            fields_.push_back(fld);
        }
        if (fields_.empty()) return false;

        Serial.printf("[SPIFFS] Loaded model.json successfully with %u fields\n", (unsigned)fields_.size());
        return true;
    }

    void save() {
        JsonDocument doc;
        JsonArray arr = doc.createNestedArray("fields");
        for (auto& f : fields_) {
            JsonObject o = arr.add<JsonObject>();
            f.toJson(o);
        }
        File f = SPIFFS.open("/model.json", "w");
        if (!f) {
            Serial.println("[SPIFFS] Failed to open model.json for writing");
            return;
        }
        serializeJson(doc, f);
        f.close();
        Serial.println("[SPIFFS] model.json saved.");
    }
};

// --------------------------- Globals ----------------------------------------
Model gModel;
AsyncWebServer server(80);
WebSocketsServer ws(81);

// --------------------------- Utility functions ------------------------------
bool isIntegerString(const String& s) {
    if (s.isEmpty()) return false;
    size_t i = (s[0] == '+' || s[0] == '-') ? 1 : 0;
    for (; i < s.length(); ++i)
        if (!isDigit(s[i])) return false;
    return true;
}
bool isFloatString(const String& s) {
    if (s.isEmpty()) return false;
    bool seenDot = false;
    size_t i = (s[0] == '+' || s[0] == '-') ? 1 : 0;
    for (; i < s.length(); ++i) {
        if (s[i] == '.') {
            if (seenDot) return false;
            seenDot = true;
        } else if (!isDigit(s[i]))
            return false;
    }
    return true;
}
bool validateFieldValue(const Field& f, const String& val) {
    String t = f.getType();
    if (t == "int") return isIntegerString(val);
    if (t == "float") return isFloatString(val);
    if (t == "bool") return (val == "0" || val == "1" || val == "true" || val == "false");
    return true;
}

void broadcastModel() {
    JsonDocument doc;
    JsonArray arr = doc.createNestedArray("fields");
    for (auto& f : gModel.fields()) {
        JsonObject o = arr.add<JsonObject>();
        f.toJson(o);
    }
    String s;
    serializeJson(doc, s);
    ws.broadcastTXT(s);
}

// --------------------------- WebSocket event -------------------------------
void onWsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    if (type != WStype_TEXT) return;
    String msg = String((char*)payload).substring(0, length);
    JsonDocument doc;
    if (deserializeJson(doc, msg)) return;
    String action = doc["action"].as<String>();

    if (action == "update") {
        String id = doc["id"].as<String>();
        String val = doc["value"].as<String>();
        Field* f = gModel.getById(id);
        if (f && !f->isReadOnly() && validateFieldValue(*f, val)) {
            f->setValue(val);
            gModel.save();
            broadcastModel();
            Serial.printf("[WEB] Updated %s = %s\n", f->getName().c_str(), val.c_str());
        }
    } else if (action == "add") {
        Field f;
        f.fromJson(doc);
        if (f.getId().isEmpty()) f = Field(String(gModel.fields().size() + 1), f.getName(), f.getType(), f.getValue(), f.getDescription(), f.isReadOnly());
        gModel.addField(f);
        gModel.save();
        broadcastModel();
        Serial.printf("[WEB] Added %s (id=%s)\n", f.getName().c_str(), f.getId().c_str());
    } else if (action == "delete") {
        String id = doc["id"].as<String>();
        gModel.removeById(id);
        gModel.save();
        broadcastModel();
    } else if (action == "moveUp" || action == "moveDown") {
        String id = doc["id"].as<String>();
        bool up = (action == "moveUp");
        gModel.reorder(id, up);
        gModel.save();
        broadcastModel();
    }
}

// --------------------------- Factory defaults -------------------------------
void factoryDefaultModel() {
    Serial.println("[MODEL] Creating factory default model...");
    gModel.fields().clear();
    gModel.addField(Field("1", "Temperature", "float", "22.5", "Room temperature", false));
    gModel.addField(Field("2", "Enabled", "bool", "1", "System enabled", false));
    gModel.addField(Field("3", "DeviceID", "string", "ESP32-001", "Device identifier", true));
    gModel.addField(Field("4", "Count", "int", "42", "Event counter", false));
    gModel.save();
}

// --------------------------- HTML pages -------------------------------------
String generateIndex();
String generateMetadata();
String generateDebug();

String generateIndex() {
    String html = "<html><head><meta charset='utf-8'><title>Index</title></head><body>";
    html += "<div><a href='/'>Index</a> | <a href='/metadata'>Metadata</a> | <a href='/debug'>Debug</a></div>";
    html += "<h1>Index</h1><table border='1'><tr><th>Name</th><th>Type</th><th>Value</th></tr>";

    for (auto& f : gModel.fields()) {
        String disabled = f.isReadOnly() ? "disabled" : "";
        html += "<tr><td>" + f.getName() + "</td><td>" + f.getType() + "</td><td>";
        if (f.getType() == "bool") {
            html += "<select data-id='" + f.getId() + "' onchange='updateField(this)' " + disabled + ">";
            html += "<option value='1'" + String(f.getValue() == "1" ? " selected" : "") + ">true</option>";
            html += "<option value='0'" + String(f.getValue() == "0" ? " selected" : "") + ">false</option>";
            html += "</select>";
        } else {
            html += "<input type='text' data-id='" + f.getId() + "' value='" + f.getValue() + "' onchange='updateField(this)' " + disabled + ">";
        }
        html += "</td></tr>";
    }
    html += "</table>";

    html += R"rawliteral(
<script>
var ws = new WebSocket('ws://'+location.hostname+':81/');
ws.onmessage = function(evt){
  var data = JSON.parse(evt.data);
  if(!data.fields) return;
  data.fields.forEach(f=>{
    var el=document.querySelector("[data-id='"+f.id+"']");
    if(el){ el.value=f.value; el.disabled=f.readOnly; }
  });
};
function updateField(el){
  ws.send(JSON.stringify({action:'update', id:el.dataset.id, value:el.value}));
}
</script></body></html>
)rawliteral";
    return html;
}

String generateMetadata() {
    String html = "<html><head><meta charset='utf-8'><title>Metadata</title></head><body>";
    html += "<div><a href='/'>Index</a> | <a href='/metadata'>Metadata</a> | <a href='/debug'>Debug</a></div>";
    html += "<h1>Metadata</h1><table border='1'><tr><th>Name</th><th>Type</th><th>Description</th><th>ReadOnly</th><th>Order</th><th>Delete</th></tr>";
    for (auto& f : gModel.fields()) {
        html += "<tr><td>" + f.getName() + "</td><td>" + f.getType() + "</td><td>" + f.getDescription() + "</td>";
        html += "<td>" + String(f.isReadOnly() ? "Yes" : "No") + "</td>";
        html += "<td><button onclick=\"moveField('" + f.getId() + "','up')\">▲</button><button onclick=\"moveField('" + f.getId() + "','down')\">▼</button></td>";
        html += "<td><button onclick=\"deleteField('" + f.getId() + "')\">Delete</button></td></tr>";
    }
    html += "</table><h2>Add Field</h2>";
    html += "ID:<input id='fid'> Name:<input id='fname'> Type:<select id='ftype'><option>string</option><option>int</option><option>float</option><option>bool</option></select> ";
    html += "Value:<input id='fvalue'> Desc:<input id='fdesc'> ReadOnly:<input type='checkbox' id='freadonly'> <button onclick='addField()'>Add</button>";
    html += R"rawliteral(
<script>
var ws=new WebSocket('ws://'+location.hostname+':81/');
ws.onmessage=function(e){
 var data=JSON.parse(e.data); if(!data.fields)return;
 var rows='';
 data.fields.forEach(f=>{
   rows+='<tr><td>'+f.name+'</td><td>'+f.type+'</td><td>'+f.description+'</td><td>'+(f.readOnly?'Yes':'No')+'</td>';
   rows+='<td><button onclick="moveField(\''+f.id+'\',\'up\')">▲</button><button onclick="moveField(\''+f.id+'\',\'down\')">▼</button></td>';
   rows+='<td><button onclick="deleteField(\''+f.id+'\')">Delete</button></td></tr>';
 });
 document.querySelector('table').innerHTML='<tr><th>Name</th><th>Type</th><th>Description</th><th>ReadOnly</th><th>Order</th><th>Delete</th></tr>'+rows;
};
function addField(){
 ws.send(JSON.stringify({action:'add',id:fid.value,name:fname.value,type:ftype.value,value:fvalue.value,description:fdesc.value,readOnly:freadonly.checked}));
 fid.value=fname.value=fvalue.value=fdesc.value='';
 freadonly.checked=false;
}
function deleteField(id){ws.send(JSON.stringify({action:'delete',id:id}));}
function moveField(id,dir){ws.send(JSON.stringify({action:dir==='up'?'moveUp':'moveDown',id:id}));}
</script></body></html>
)rawliteral";
    return html;
}

String generateDebug() {
    String html = "<html><body><a href='/'>Index</a> | <a href='/metadata'>Metadata</a> | <a href='/debug'>Debug</a><h1>Debug</h1><pre>";
    File f = SPIFFS.open("/model.json", "r");
    if (f) {
        while (f.available()) html += (char)f.read();
        f.close();
    }
    html += "</pre><button onclick='fetch(\"/reboot\")'>Reboot</button></body></html>";
    return html;
}

// --------------------------- Setup / Loop -----------------------------------
void setup() {
    Serial.begin(115200);
    Serial.println("\nBooting...");

    WiFi.mode(WIFI_STA);
    WiFi.begin(getSsid(), getPass());
    Serial.print("[WiFi] Connecting");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.printf("\n[WiFi] Connected: %s\n", WiFi.localIP().toString().c_str());

    if (MDNS.begin("ad")) Serial.println("[mDNS] Registered as ad.local");
    if (SPIFFS.begin())
        Serial.println("[SPIFFS] Mounted successfully.");
    else
        Serial.println("[SPIFFS] Mount failed.");

    if (!gModel.load()) factoryDefaultModel();

    server.on("/", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/html", generateIndex()); });
    server.on("/metadata", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/html", generateMetadata()); });
    server.on("/debug", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/html", generateDebug()); });
    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200,"text/plain","Rebooting..."); delay(500); ESP.restart(); });
    server.begin();

    ws.begin();
    ws.onEvent(onWsEvent);

    Serial.printf("Root URL: http://%s/\n", WiFi.localIP().toString().c_str());
    broadcastModel();
}

void loop() {
    ws.loop();
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        if (cmd == "?") {
            for (auto& f : gModel.fields()) Serial.printf("%s (%s) = %s | ReadOnly=%s\n",
                                                          f.getName().c_str(), f.getType().c_str(), f.getValue().c_str(), f.isReadOnly() ? "true" : "false");
        } else if (cmd == "j") {
            File f = SPIFFS.open("/model.json", "r");
            if (f) {
                while (f.available()) Serial.write(f.read());
                f.close();
                Serial.println();
            }
        } else if (cmd.indexOf('=') > 0) {
            String name = cmd.substring(0, cmd.indexOf('=')), val = cmd.substring(cmd.indexOf('=') + 1);
            Field* f = gModel.getByName(name);
            if (f && validateFieldValue(*f, val)) {
                f->setValue(val);
                gModel.save();
                broadcastModel();
                Serial.printf("[SERIAL] Updated %s = %s\n", f->getName().c_str(), val.c_str());
            }
        }
    }
}
