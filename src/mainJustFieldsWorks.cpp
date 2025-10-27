// main.cpp - full file with embedded HTML and fixed metadata add form

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>

#include "pass.h"

// ---------------- Field ----------------
class Field {
    String name, type, value, description;
    int id;

   public:
    Field() {}
    Field(String n, int i, String t, String v, String d)
        : name(n), id(i), type(t), value(v), description(d) {}
    String getName() const { return name; }
    String getType() const { return type; }
    String getValue() const { return value; }
    String getDescription() const { return description; }
    int getId() const { return id; }
    void setName(const String& n) { name = n; }
    void setType(const String& t) { type = t; }
    void setValue(const String& v) { value = v; }
    void setDescription(const String& d) { description = d; }
};

// ---------------- Model ----------------
class Model {
    std::vector<Field> fields;
    const char* path = "/model.json";

   public:
    Model() { /* don't auto-load here; caller will mount SPIFFS then call loadFromSPIFFS */ }

    std::vector<Field>& getFields() { return fields; }

    void addField(const Field& f) {
        fields.push_back(f);
        saveToSPIFFS();
    }
    void removeFieldById(int id) {
        fields.erase(std::remove_if(fields.begin(), fields.end(),
                                    [id](Field& f) { return f.getId() == id; }),
                     fields.end());
        saveToSPIFFS();
    }

    Field* findFieldById(int id) {
        for (auto& f : fields)
            if (f.getId() == id) return &f;
        return nullptr;
    }

    Field* findFieldByName(const String& name) {
        for (auto& f : fields)
            if (f.getName() == name) return &f;
        return nullptr;
    }

    void saveToSPIFFS() {
        File file = SPIFFS.open(path, FILE_WRITE);
        if (!file) {
            Serial.println("[MODEL] Failed to open file for writing");
            return;
        }
        // Use a JsonArray document
        DynamicJsonDocument doc(4096);
        JsonArray arr = doc.to<JsonArray>();
        for (auto& f : fields) {
            JsonObject o = arr.createNestedObject();
            o["id"] = f.getId();
            o["name"] = f.getName();
            o["type"] = f.getType();
            o["value"] = f.getValue();
            o["description"] = f.getDescription();
        }
        serializeJsonPretty(doc, file);
        file.close();
        Serial.println("[MODEL] Saved to SPIFFS");
    }

    void loadFromSPIFFS() {
        if (!SPIFFS.exists(path)) {
            Serial.println("[MODEL] No existing file, using defaults");
            fields = {
                Field("temperature", 1, "float", "22.5", "Room temperature (°C)"),
                Field("enabled", 2, "bool", "true", "System enable flag"),
                Field("threshold", 3, "int", "10", "Alarm threshold (°C)")};
            saveToSPIFFS();
            return;
        }
        File file = SPIFFS.open(path, FILE_READ);
        if (!file) {
            Serial.println("[MODEL] Failed to open file for reading");
            return;
        }
        DynamicJsonDocument doc(4096);
        DeserializationError err = deserializeJson(doc, file);
        file.close();
        if (err) {
            Serial.println("[MODEL] JSON parse error");
            return;
        }
        fields.clear();
        JsonArray arr = doc.as<JsonArray>();
        for (JsonObject o : arr) {
            fields.push_back(Field(
                o["name"].as<String>(),
                o["id"].as<int>(),
                o["type"].as<String>(),
                o["value"].as<String>(),
                o["description"].as<String>()));
        }
        Serial.println("[MODEL] Loaded from SPIFFS");
    }
};

// ---------------- Embedded HTML ----------------
// index - shows description, menu on top
const char index_html[] PROGMEM = R"HTML(
<!DOCTYPE html><html><head><meta charset="utf-8"><title>Index</title></head>
<body>
<div style="background:#eee;padding:10px;">
  <a href="/">Home</a> | <a href="/metadata.index">Metadata</a> | <a href="/debug.html">Debug</a>
</div>
<h1>Device Fields</h1>
<div id="fields"></div>
<button onclick="save()">Save Changes</button>
<button onclick="reset()">Reset</button>
<script>
let ws = new WebSocket('ws://' + location.host + '/ws');
ws.onmessage = (e) => {
  const data = JSON.parse(e.data);
  let html = '';
  for (let f of data) {
    html += `<div><b>${f.name}</b> - <small>${f.description}</small><br>`;
    if (f.type == 'bool')
      html += `<input type="checkbox" id="v${f.id}" ${f.value=='true'?'checked':''}>`;
    else if (f.type == 'int' || f.type == 'float')
      html += `<input type="number" id="v${f.id}" value="${f.value}">`;
    else
      html += `<input type="text" id="v${f.id}" value="${f.value}">`;
    html += `</div><br>`;
  }
  document.getElementById('fields').innerHTML = html;
};
function save(){
  document.querySelectorAll("input").forEach(el=>{
    let id = parseInt(el.id.substring(1));
    let val = (el.type=='checkbox') ? (el.checked ? 'true' : 'false') : el.value;
    ws.send(JSON.stringify({ edit: { id: id, value: val } }));
  });
}
function reset(){ location.reload(); }
</script>
</body></html>
)HTML";

// metadata - inline add form (name,type,value,description) + edit/delete
const char metadata_html[] PROGMEM = R"HTML(
<!DOCTYPE html><html><head><meta charset="utf-8"><title>Metadata</title></head>
<body>
<div style="background:#eee;padding:10px;">
  <a href="/">Home</a> | <a href="/metadata.index">Metadata</a> | <a href="/debug.html">Debug</a>
</div>
<h1>Metadata</h1>
<h3>Existing Fields</h3>
<table border="1" cellpadding="4">
<thead><tr><th>ID</th><th>Name</th><th>Type</th><th>Value</th><th>Description</th><th>Action</th></tr></thead>
<tbody id="rows"></tbody>
</table>
<h3>Add New Field</h3>
<table>
<tr>
  <td>Name:</td><td><input id="newName" /></td>
  <td>Type:</td><td>
    <select id="newType"><option>string</option><option>int</option><option>float</option><option>bool</option></select>
  </td>
</tr>
<tr>
  <td>Value:</td><td><input id="newValue" /></td>
  <td>Description:</td><td><input id="newDesc" /></td>
  <td><button onclick="addField()">Add Field</button></td>
</tr>
</table>
<script>
let ws = new WebSocket('ws://' + location.host + '/ws');

function render(fields){
  let html = '';
  for (let f of fields) {
    html += `<tr>
      <td>${f.id}</td>
      <td><input id="n${f.id}" value="${f.name}"></td>
      <td>
        <select id="t${f.id}">
          <option ${f.type=='string'?'selected':''}>string</option>
          <option ${f.type=='int'?'selected':''}>int</option>
          <option ${f.type=='float'?'selected':''}>float</option>
          <option ${f.type=='bool'?'selected':''}>bool</option>
        </select>
      </td>
      <td><input id="v${f.id}" value="${f.value}"></td>
      <td><input id="d${f.id}" value="${f.description}"></td>
      <td>
        <button onclick="edit(${f.id})">Save</button>
        <button onclick="del(${f.id})">Delete</button>
      </td>
    </tr>`;
  }
  document.getElementById('rows').innerHTML = html;
}

ws.onmessage = (e) => {
  let arr = JSON.parse(e.data);
  render(arr);
};

function del(id){ ws.send(JSON.stringify({ delete: id })); }

function edit(id){
  ws.send(JSON.stringify({ edit: {
    id: id,
    name: document.getElementById('n'+id).value,
    type: document.getElementById('t'+id).value,
    value: document.getElementById('v'+id).value,
    description: document.getElementById('d'+id).value
  }}));
}

function addField(){
  const name = document.getElementById('newName').value || 'newField';
  const type = document.getElementById('newType').value || 'string';
  const value = document.getElementById('newValue').value || '';
  const description = document.getElementById('newDesc').value || '';
  ws.send(JSON.stringify({ add: { name: name, type: type, value: value, description: description } }));
  document.getElementById('newName').value='';
  document.getElementById('newValue').value='';
  document.getElementById('newDesc').value='';
}
</script>
</body></html>
)HTML";

// debug - includes menu on top
const char debug_html[] PROGMEM = R"HTML(
<!DOCTYPE html><html><head><meta charset="utf-8"><title>Debug</title></head>
<body>
<div style="background:#eee;padding:10px;">
  <a href="/">Home</a> | <a href="/metadata.index">Metadata</a> | <a href="/debug.html">Debug</a>
</div>
<h1>Debug</h1>
<pre id="data"></pre>
<button onclick="reload()">Reload</button>
<button onclick="reboot()">Reboot ESP</button>
<script>
function reload(){
  fetch('/model.json').then(r=>r.json()).then(d=>{
    document.getElementById('data').textContent = JSON.stringify(d, null, 2);
  });
}
function reboot(){ fetch('/reboot'); }
reload();
</script>
</body></html>
)HTML";

// ---------------- Web ----------------
class Web {
    AsyncWebServer server{80};
    AsyncWebSocket ws{"/ws"};
    Model* model;

   public:
    Web(Model* m) : model(m) {}

    void begin() {
        // WiFi
        Serial.println("[WiFi] Connecting...");
        WiFi.mode(WIFI_STA);
        WiFi.begin(getSsid(), getPass());
        unsigned long startAttemptTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
            Serial.print(".");
            delay(500);
        }
        Serial.println();
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("[WiFi] Connected to %s\n", getSsid());
            Serial.printf("[WiFi] IP address: %s\n", WiFi.localIP().toString().c_str());
        } else {
            Serial.println("[WiFi] Failed to connect (continuing offline mode)");
        }

        // SPIFFS mount (after WiFi)
        if (!SPIFFS.begin(false)) {
            Serial.println("[SPIFFS] Mount failed");
        } else {
            Serial.println("[SPIFFS] Mounted successfully");
        }

        // load model now that SPIFFS is mounted
        model->loadFromSPIFFS();

        // mDNS
        if (MDNS.begin("ad")) {
            Serial.println("[mDNS] Started: http://ad.local");
        } else {
            Serial.println("[mDNS] Failed to start");
        }

        // WebSocket handling
        ws.onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client,
                          AwsEventType type, void* arg, uint8_t* data, size_t len) {
            if (type == WS_EVT_CONNECT) {
                client->text(fieldsToJson());
            } else if (type == WS_EVT_DATA) {
                String msg = String((char*)data, len);
                DynamicJsonDocument doc(1024);
                DeserializationError err = deserializeJson(doc, msg);
                if (!err) {
                    if (doc.containsKey("edit")) {
                        JsonObject f = doc["edit"];
                        Field* field = model->findFieldById(f["id"]);
                        if (field) {
                            if (f.containsKey("name")) field->setName(f["name"].as<String>());
                            if (f.containsKey("type")) field->setType(f["type"].as<String>());
                            if (f.containsKey("value")) field->setValue(f["value"].as<String>());
                            if (f.containsKey("description")) field->setDescription(f["description"].as<String>());
                            model->saveToSPIFFS();
                            broadcastFields();
                            Serial.printf("[WEB] Updated %s = %s\n", field->getName().c_str(), field->getValue().c_str());
                        }
                    } else if (doc.containsKey("add")) {
                        JsonObject f = doc["add"];
                        int id = random(1000, 9999);
                        String nm = f["name"].as<String>();
                        String tp = f["type"].as<String>();
                        String val = f["value"].as<String>();
                        String desc = f["description"].as<String>();
                        model->addField(Field(nm, id, tp, val, desc));
                        broadcastFields();
                        Serial.printf("[WEB] Added %s (id=%d)\n", nm.c_str(), id);
                    } else if (doc.containsKey("delete")) {
                        int id = doc["delete"];
                        model->removeFieldById(id);
                        broadcastFields();
                        Serial.printf("[WEB] Deleted id=%d\n", id);
                    }
                }
            }
        });
        server.addHandler(&ws);

        // HTTP routes serve embedded HTML
        server.on("/", HTTP_GET, [this](AsyncWebServerRequest* req) { req->send(200, "text/html", index_html); });
        server.on("/metadata.index", HTTP_GET, [this](AsyncWebServerRequest* req) { req->send(200, "text/html", metadata_html); });
        server.on("/debug.html", HTTP_GET, [this](AsyncWebServerRequest* req) { req->send(200, "text/html", debug_html); });
        server.on("/model.json", HTTP_GET, [this](AsyncWebServerRequest* req) { req->send(200, "application/json", fieldsToJson()); });
        server.on("/reboot", HTTP_GET, [this](AsyncWebServerRequest* req) { req->send(200, "text/plain", "Rebooting..."); delay(500); ESP.restart(); });

        server.begin();
        Serial.printf("[WEB] Server started at http://%s/\n", WiFi.localIP().toString().c_str());
    }

    void loop() { ws.cleanupClients(); }

    String fieldsToJson() {
        DynamicJsonDocument doc(4096);
        JsonArray arr = doc.to<JsonArray>();
        for (auto& f : model->getFields()) {
            JsonObject o = arr.createNestedObject();
            o["id"] = f.getId();
            o["name"] = f.getName();
            o["type"] = f.getType();
            o["value"] = f.getValue();
            o["description"] = f.getDescription();
        }
        String out;
        serializeJson(doc, out);
        return out;
    }

    void broadcastFields() { ws.textAll(fieldsToJson()); }
};

// ---------------- Globals ----------------
Model model;
Web* web;

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("[BOOT] ESP32 Model Sync");
    web = new Web(&model);
    web->begin();
}

void loop() {
    web->loop();
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        if (cmd == "?") {
            Serial.println("Available fields (name = value type) - description:");
            for (auto& f : model.getFields())
                Serial.printf("%s = %s (%s) - %s\n", f.getName().c_str(), f.getValue().c_str(), f.getType().c_str(), f.getDescription().c_str());
            Serial.println("Syntax: fieldName=newValue | j=dump JSON");
        } else if (cmd == "j") {
            Serial.println(web->fieldsToJson());
        } else {
            int eq = cmd.indexOf('=');
            if (eq > 0) {
                String name = cmd.substring(0, eq);
                String val = cmd.substring(eq + 1);
                Field* f = model.findFieldByName(name);
                if (f) {
                    f->setValue(val);
                    model.saveToSPIFFS();
                    web->broadcastFields();
                    Serial.printf("[SERIAL] Updated %s = %s\n", name.c_str(), val.c_str());
                } else
                    Serial.println("[SERIAL] Field not found");
            }
        }
    }
}
