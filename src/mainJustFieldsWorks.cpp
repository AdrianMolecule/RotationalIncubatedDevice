// src/main.cpp
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
        if (!f) {
            Serial.println("[SPIFFS] model.json open failed");
            return false;
        }
        String j = f.readString();
        f.close();
        if (j.isEmpty()) {
            Serial.println("[SPIFFS] model.json empty");
            return false;
        }

        StaticJsonDocument<8192> doc;  // temporary doc size; change if model grows
        auto err = deserializeJson(doc, j);
        if (err) {
            Serial.printf("[SPIFFS] deserializeJson failed: %s\n", err.c_str());
            return false;
        }

        fields_.clear();
        if (!doc.containsKey("fields")) {
            Serial.println("[SPIFFS] model.json missing 'fields'");
            return false;
        }
        for (JsonObject obj : doc["fields"].as<JsonArray>()) {
            Field fld;
            fld.fromJson(obj);
            fields_.push_back(fld);
        }
        if (fields_.empty()) {
            Serial.println("[SPIFFS] No fields found in model.json");
            return false;
        }
        Serial.printf("[SPIFFS] Loaded model.json successfully with %u fields\n", (unsigned)fields_.size());
        return true;
    }

    void save() {
        StaticJsonDocument<8192> doc;
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
    if (t == "bool") return (val == "0" || val == "1" || val.equalsIgnoreCase("true") || val.equalsIgnoreCase("false"));
    return true;
}

void broadcastModel() {
    StaticJsonDocument<8192> doc;
    JsonArray arr = doc.createNestedArray("fields");
    for (auto& f : gModel.fields()) {
        JsonObject o = arr.add<JsonObject>();
        f.toJson(o);
    }
    String s;
    serializeJson(doc, s);
    ws.broadcastTXT(s);
}

// --------------------------- Serial add-field parser ------------------------
/*
 Expected format:
  F id=<id> name=<Name> type=<string|int|float|bool> value=<value> description=<desc> readOnly=<0|1|true|false>

 Attributes can appear in any order. Values must not contain spaces (use _ or similar).
*/
void addFieldFromSerial(const String& line) {
    // line begins with 'F ' or 'f '
    String rest = line;
    if (rest.startsWith("F "))
        rest = rest.substring(2);
    else if (rest.startsWith("f "))
        rest = rest.substring(2);

    // initialize defaults
    String id = "";
    String name = "";
    String type = "string";
    String value = "";
    String description = "";
    bool readOnly = false;

    // split by spaces into tokens
    int start = 0;
    while (start < rest.length()) {
        // find next space
        int sp = rest.indexOf(' ', start);
        String token;
        if (sp == -1) {
            token = rest.substring(start);
            start = rest.length();
        } else {
            token = rest.substring(start, sp);
            start = sp + 1;
        }
        token.trim();
        if (token.length() == 0) continue;
        int eq = token.indexOf('=');
        if (eq <= 0) continue;
        String key = token.substring(0, eq);
        String val = token.substring(eq + 1);
        // assign
        if (key.equalsIgnoreCase("id"))
            id = val;
        else if (key.equalsIgnoreCase("name"))
            name = val;
        else if (key.equalsIgnoreCase("type"))
            type = val;
        else if (key.equalsIgnoreCase("value"))
            value = val;
        else if (key.equalsIgnoreCase("description"))
            description = val;
        else if (key.equalsIgnoreCase("readonly") || key.equalsIgnoreCase("readOnly")) {
            if (val == "1" || val.equalsIgnoreCase("true"))
                readOnly = true;
            else
                readOnly = false;
        }
    }

    // minimal validation: name and type at least
    if (name.length() == 0) {
        Serial.println("[SERIAL] Add field failed: name required (name=<Name>)");
        return;
    }
    // validate type
    String t = type;
    t.toLowerCase();
    if (!(t == "string" || t == "int" || t == "float" || t == "bool")) {
        Serial.println("[SERIAL] Add field failed: invalid type (use string,int,float,bool)");
        return;
    }
    // if id empty generate one
    if (id.length() == 0) {
        int next = (int)gModel.fields().size() + 1;
        id = String(next);
        // ensure uniqueness (increment until unique)
        while (gModel.getById(id) != nullptr) {
            next++;
            id = String(next);
        }
    } else {
        // ensure id uniqueness
        if (gModel.getById(id) != nullptr) {
            Serial.printf("[SERIAL] Add field failed: id '%s' already exists\n", id.c_str());
            return;
        }
    }

    // create field and basic validation of value
    Field f(id, name, t, value, description, readOnly);
    if (!value.isEmpty()) {
        if (!validateFieldValue(f, value)) {
            Serial.printf("[SERIAL] Add field failed: invalid value '%s' for type %s\n", value.c_str(), t.c_str());
            return;
        }
    }
    gModel.addField(f);
    gModel.save();
    broadcastModel();
    Serial.printf("[SERIAL] Added field id=%s name=%s type=%s readOnly=%s\n",
                  id.c_str(), name.c_str(), t.c_str(), readOnly ? "true" : "false");
}

// --------------------------- WebSocket handler -------------------------------
void onWsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    if (type != WStype_TEXT) return;
    String msg = String((char*)payload).substring(0, length);
    StaticJsonDocument<1024> doc;
    if (deserializeJson(doc, msg)) return;
    String action = doc["action"].as<String>();

    if (action == "update") {
        String id = doc["id"].as<String>();
        String val = doc["value"].as<String>();
        Field* f = gModel.getById(id);
        if (f) {
            if (f->isReadOnly()) {
                Serial.printf("[WEB] Ignored update to readOnly field %s\n", f->getName().c_str());
                return;
            }
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
        Field f;
        f.fromJson(doc.as<JsonObject>());
        if (f.getId().isEmpty()) {
            int next = (int)gModel.fields().size() + 1;
            String newid = String(next);
            while (gModel.getById(newid) != nullptr) {
                next++;
                newid = String(next);
            }
            // rebuild with id
            f = Field(newid, f.getName(), f.getType(), f.getValue(), f.getDescription(), f.isReadOnly());
        } else {
            if (gModel.getById(f.getId()) != nullptr) {
                Serial.printf("[WEB] Add failed: id %s exists\n", f.getId().c_str());
                return;
            }
        }
        // validate value if present
        if (!f.getValue().isEmpty() && !validateFieldValue(f, f.getValue())) {
            Serial.printf("[WEB] Add failed: invalid value %s for type %s\n", f.getValue().c_str(), f.getType().c_str());
            return;
        }
        gModel.addField(f);
        gModel.save();
        broadcastModel();
        Serial.printf("[WEB] Added %s (id=%s)\n", f.getName().c_str(), f.getId().c_str());
    } else if (action == "delete") {
        String id = doc["id"].as<String>();
        Field* f = gModel.getById(id);
        if (f) Serial.printf("[WEB] Deleted %s (id=%s)\n", f->getName().c_str(), id.c_str());
        gModel.removeById(id);
        gModel.save();
        broadcastModel();
    } else if (action == "moveUp" || action == "moveDown") {
        String id = doc["id"].as<String>();
        bool up = (action == "moveUp");
        gModel.reorder(id, up);
        gModel.save();
        broadcastModel();
        Serial.printf("[WEB] Reordered id=%s %s\n", id.c_str(), up ? "up" : "down");
    }
}

// --------------------------- Factory default model --------------------------
void factoryDefaultModel() {
    Serial.println("[MODEL] Creating factory default model...");
    gModel.fields().clear();
    gModel.addField(Field("1", "Temperature", "float", "22.5", "Room temperature", false));
    gModel.addField(Field("2", "Enabled", "bool", "1", "System enabled", false));
    gModel.addField(Field("3", "DeviceID", "string", "ESP32-001", "Device identifier", true));
    gModel.addField(Field("4", "Count", "int", "42", "Event counter", false));
    gModel.save();
}

// --------------------------- HTML generators --------------------------------
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
            html += "<input type='" + String((f.getType() == "int" || f.getType() == "float") ? "number" : "text") +
                    "' data-id='" + f.getId() + "' value='" + f.getValue() + "' onchange='updateField(this)' " + disabled + ">";
        }
        html += "</td></tr>";
    }
    html += "</table>";

    html += R"rawliteral(
<script>
var ws = new WebSocket('ws://'+location.hostname+':81/');
ws.onmessage = function(evt){
  try {
    var data = JSON.parse(evt.data);
    if(!data.fields) return;
    data.fields.forEach(function(f){
      var el = document.querySelector("[data-id='"+f.id+"']");
      if(el) {
        el.value = f.value;
        el.disabled = !!f.readOnly;
      }
    });
  } catch(e) { console.log("ws parse err", e); }
};
function updateField(el){
  ws.send(JSON.stringify({action:'update', id: el.dataset.id, value: el.value}));
}
</script>
</body></html>
)rawliteral";
    return html;
}

String generateMetadata() {
    String html = "<html><head><meta charset='utf-8'><title>Metadata</title></head><body>";
    html += "<div><a href='/'>Index</a> | <a href='/metadata'>Metadata</a> | <a href='/debug'>Debug</a></div>";
    html += "<h1>Metadata</h1>";

    html += "<table border='1'><thead><tr><th>Name</th><th>Type</th><th>Description</th><th>ReadOnly</th><th>Order</th><th>Delete</th></tr></thead><tbody>";
    for (auto& f : gModel.fields()) {
        html += "<tr><td>" + f.getName() + "</td><td>" + f.getType() + "</td><td>" + f.getDescription() + "</td>";
        html += "<td>" + String(f.isReadOnly() ? "Yes" : "No") + "</td>";
        html += "<td><button onclick=\"moveField('" + f.getId() + "','up')\">&#x25B2;</button><button onclick=\"moveField('" + f.getId() + "','down')\">&#x25BC;</button></td>";
        html += "<td><button onclick=\"deleteField('" + f.getId() + "')\">Delete</button></td></tr>";
    }
    html += "</tbody></table>";

    html += "<h2>Add Field</h2>";
    html += "ID: <input id='fid' placeholder='optional'> ";
    html += "Name: <input id='fname' placeholder='required'> ";
    html += "Type: <select id='ftype'><option value='string'>string</option><option value='int'>int</option><option value='float'>float</option><option value='bool'>bool</option></select> ";
    html += "Value: <input id='fvalue' placeholder=''> ";
    html += "Desc: <input id='fdesc' placeholder=''> ";
    html += "ReadOnly: <input type='checkbox' id='freadonly'> ";
    html += "<button onclick='addField()'>Add Field</button>";

    html += R"rawliteral(
<script>
var ws = new WebSocket('ws://'+location.hostname+':81/');
ws.onmessage = function(evt){
  try {
    var data = JSON.parse(evt.data);
    if(!data.fields) return;
    var rows = '';
    data.fields.forEach(function(f){
      rows += '<tr><td>'+f.name+'</td><td>'+f.type+'</td><td>'+f.description+'</td><td>'+(f.readOnly?'Yes':'No')+'</td>';
      rows += '<td><button onclick="moveField(\''+f.id+'\',\'up\')">&#x25B2;</button><button onclick="moveField(\''+f.id+'\',\'down\')">&#x25BC;</button></td>';
      rows += '<td><button onclick="deleteField(\''+f.id+'\')">Delete</button></td></tr>';
    });
    document.querySelector('table').innerHTML = '<tr><th>Name</th><th>Type</th><th>Description</th><th>ReadOnly</th><th>Order</th><th>Delete</th></tr>' + rows;
  } catch(e) { console.log("ws parse err", e); }
};
function addField(){
  var obj = {
    action: 'add',
    id: document.getElementById('fid').value,
    name: document.getElementById('fname').value,
    type: document.getElementById('ftype').value,
    value: document.getElementById('fvalue').value,
    description: document.getElementById('fdesc').value,
    readOnly: document.getElementById('freadonly').checked
  };
  ws.send(JSON.stringify(obj));
  document.getElementById('fid').value=''; document.getElementById('fname').value=''; document.getElementById('fvalue').value='';
  document.getElementById('fdesc').value=''; document.getElementById('freadonly').checked=false;
}
function deleteField(id){ ws.send(JSON.stringify({action:'delete', id:id})); }
function moveField(id,dir){ ws.send(JSON.stringify({action: dir==='up'?'moveUp':'moveDown', id:id})); }
</script>
</body></html>
)rawliteral";
    return html;
}

String generateDebug() {
    String html = "<html><head><meta charset='utf-8'><title>Debug</title></head><body>";
    html += "<div><a href='/'>Index</a> | <a href='/metadata'>Metadata</a> | <a href='/debug'>Debug</a></div>";
    html += "<h1>Debug</h1><pre>";
    File f = SPIFFS.open("/model.json", "r");
    if (f) {
        while (f.available()) html += (char)f.read();
        f.close();
    } else
        html += "[SPIFFS] model.json missing";
    html += "</pre><br><button onclick='fetch(\"/reboot\").then(()=>alert(\"Rebooting\"))'>Reboot ESP32</button>";
    html += "</body></html>";
    return html;
}

// --------------------------- Setup / Loop -----------------------------------
void setup() {
    Serial.begin(115200);
    Serial.println("\nBooting...");

    WiFi.mode(WIFI_STA);
    WiFi.begin(getSsid(), getPass());
    Serial.print("[WiFi] Connecting");
    unsigned long start = millis();
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
        Serial.println("[SPIFFS] SPIFFS mount failed!");

    if (!gModel.load()) factoryDefaultModel();

    server.on("/", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/html", generateIndex()); });
    server.on("/metadata", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/html", generateMetadata()); });
    server.on("/debug", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/html", generateDebug()); });
    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/plain", "Rebooting..."); delay(500); ESP.restart(); });

    server.begin();

    ws.begin();
    ws.onEvent(onWsEvent);

    Serial.printf("Root URL: http://%s/\n", WiFi.localIP().toString().c_str());
    Serial.println("Serial commands: ? = list, j = dump json, <FieldName>=<Value> = update, F ... = add field");
    broadcastModel();
}

void loop() {
    ws.loop();

    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line == "?") {
            for (auto& f : gModel.fields()) Serial.printf("%s (%s) = %s | ReadOnly=%s\n",
                                                          f.getName().c_str(), f.getType().c_str(), f.getValue().c_str(), f.isReadOnly() ? "true" : "false");
        } else if (line == "j") {
            File f = SPIFFS.open("/model.json", "r");
            if (f) {
                while (f.available()) Serial.write(f.read());
                f.close();
                Serial.println();
            }
        } else if (line.startsWith("F ") || line.startsWith("f ")) {
            addFieldFromSerial(line);
        } else if (line.indexOf('=') > 0) {
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
