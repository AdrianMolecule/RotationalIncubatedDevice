#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WebSocketsServer.h>
#include <WiFi.h>

#include "pass.h"  // provides getSsid(), getPass()

// ==================================================
//                  FIELD CLASS
// ==================================================
class Field {
   public:
    String name;
    int id;
    String type;
    String value;
    String description;

    Field() {}
    Field(String n, int i, String t, String v, String d)
        : name(n), id(i), type(t), value(v), description(d) {}
};

// ==================================================
//                  MODEL CLASS
// ==================================================
class Model {
   private:
    const char* filename = "/model.json";

   public:
    std::vector<Field> fields;
    bool loadedFromSPIFFS = false;

    Field* getFieldById(int id) {
        for (auto& f : fields)
            if (f.id == id) return &f;
        return nullptr;
    }

    Field* getFieldByName(const String& name) {
        for (auto& f : fields)
            if (f.name == name) return &f;
        return nullptr;
    }

    void loadFromSPIFFS() {
        if (!SPIFFS.exists(filename)) {
            loadedFromSPIFFS = false;
            return;
        }
        File file = SPIFFS.open(filename, "r");
        if (!file) return;

        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, file);
        file.close();
        if (err) {
            Serial.println("[WARN] Failed to parse /model.json");
            loadedFromSPIFFS = false;
            return;
        }

        fields.clear();
        for (JsonObject f : doc.as<JsonArray>()) {
            fields.push_back(Field(
                f["name"].as<String>(),
                f["id"].as<int>(),
                f["type"].as<String>(),
                f["value"].as<String>(),
                f["description"].as<String>()));
        }
        loadedFromSPIFFS = true;
    }

    void saveToSPIFFS() {
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();
        for (auto& f : fields) {
            JsonObject o = arr.createNestedObject();
            o["name"] = f.name;
            o["id"] = f.id;
            o["type"] = f.type;
            o["value"] = f.value;
            o["description"] = f.description;
        }

        File file = SPIFFS.open(filename, "w");
        if (!file) {
            Serial.println("[ERROR] Failed to open /model.json for write");
            return;
        }
        serializeJsonPretty(doc, file);
        file.close();
        Serial.println("[INFO] Model saved to SPIFFS (/model.json)");
    }

    String readJsonFromSPIFFS() {
        if (!SPIFFS.exists(filename)) return "{}";
        File file = SPIFFS.open(filename, "r");
        if (!file) return "{}";
        String content = file.readString();
        file.close();
        return content;
    }

    int nextId() {
        int maxId = 0;
        for (auto& f : fields)
            if (f.id > maxId) maxId = f.id;
        return maxId + 1;
    }
};

// ==================================================
//                   WEB CLASS
// ==================================================
class Web {
   private:
    AsyncWebServer server;
    WebSocketsServer ws;
    Model* model;

    String fieldsToJson() {
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();
        for (auto& f : model->fields) {
            JsonObject o = arr.createNestedObject();
            o["id"] = f.id;
            o["name"] = f.name;
            o["type"] = f.type;
            o["value"] = f.value;
            o["description"] = f.description;
        }
        String out;
        serializeJson(doc, out);
        return out;
    }

    void notifyAllClients() {
        String json = fieldsToJson();
        ws.broadcastTXT(json);
    }

    void printModel() {
        Serial.println("\n[MODEL STATE]");
        for (auto& f : model->fields)
            Serial.printf("  %-15s = %-10s (%s)\n", f.name.c_str(), f.value.c_str(), f.type.c_str());
    }

    void printHelp() {
        Serial.println("\n=== Serial Commands ===");
        Serial.println("? : List all fields and syntax help");
        Serial.println("j : Dump JSON model from SPIFFS");
        Serial.println("field=value : Update field by name");
        Serial.println("--------------------------");
        for (auto& f : model->fields)
            Serial.printf("  %-15s (%s) -> %s\n", f.name.c_str(), f.type.c_str(), f.value.c_str());
    }

   public:
    Web(Model* m) : server(80), ws(81), model(m) {}

    void begin() {
        if (!SPIFFS.begin()) {
            Serial.println("[ERROR] SPIFFS mount failed!");
            return;
        }

        WiFi.begin(getSsid(), getPass());
        Serial.print("Connecting");
        while (WiFi.status() != WL_CONNECTED) {
            delay(300);
            Serial.print(".");
        }
        Serial.println("\nConnected! IP: " + WiFi.localIP().toString());

        if (MDNS.begin("biodevice"))
            Serial.println("mDNS responder started: http://biodevice.local");

        Serial.println("[INFO] Model " + String(model->loadedFromSPIFFS ? "loaded from SPIFFS" : "created from defaults"));
        printModel();
        printHelp();

        ws.onEvent([this](uint8_t num, WStype_t type, uint8_t* payload, size_t len) {
            if (type == WStype_CONNECTED) {
                String json = fieldsToJson();
                ws.sendTXT(num, json);
                return;
            }
            if (type != WStype_TEXT) return;

            JsonDocument doc;
            if (deserializeJson(doc, payload)) return;

            // Update field value
            if (doc.containsKey("id") && doc.containsKey("value")) {
                int id = doc["id"];
                String val = doc["value"].as<String>();
                Field* f = model->getFieldById(id);
                if (f) {
                    f->value = val;
                    model->saveToSPIFFS();
                    Serial.printf("[WEB] Updated %s = %s\n", f->name.c_str(), val.c_str());
                    notifyAllClients();
                    printModel();
                }
            }

            // Field delete
            if (doc.containsKey("delete")) {
                int id = doc["delete"];
                model->fields.erase(
                    std::remove_if(model->fields.begin(), model->fields.end(),
                                   [id](Field& f) { return f.id == id; }),
                    model->fields.end());
                Serial.printf("[WEB] Deleted field id=%d\n", id);
                model->saveToSPIFFS();
                notifyAllClients();
                printModel();
            }

            // Field add
            if (doc.containsKey("add")) {
                JsonObject n = doc["add"];
                int newId = model->nextId();
                model->fields.push_back(Field(
                    n["name"].as<String>(), newId,
                    n["type"].as<String>(),
                    n["value"].as<String>(),
                    n["description"].as<String>()));
                Serial.printf("[WEB] Added new field '%s'\n", n["name"].as<String>().c_str());
                model->saveToSPIFFS();
                notifyAllClients();
                printModel();
            }

            // Field edit
            if (doc.containsKey("edit")) {
                JsonObject e = doc["edit"];
                int id = e["id"];
                Field* f = model->getFieldById(id);
                if (f) {
                    f->name = e["name"].as<String>();
                    f->type = e["type"].as<String>();
                    f->value = e["value"].as<String>();
                    f->description = e["description"].as<String>();
                    Serial.printf("[WEB] Edited field id=%d (%s)\n", id, f->name.c_str());
                    model->saveToSPIFFS();
                    notifyAllClients();
                    printModel();
                }
            }
        });
        ws.begin();

        // ---------- index.html ----------
        server.on("/", HTTP_GET, [this](AsyncWebServerRequest* req) {
            String html = R"HTML(
<!DOCTYPE html><html><head><meta charset="utf-8"><title>Model</title></head>
<body>
<h2>Model</h2>
<nav>
<a href="/">Home</a> | <a href="/metadata.index">Metadata</a> | <a href="/debug.html">Debug</a>
</nav><hr>
<div id="fields">Loading...</div>
<script>
let ws=new WebSocket('ws://'+location.hostname+':81');
ws.onmessage=e=>{
 let arr=JSON.parse(e.data),h='';
 for(let f of arr){
  let i='';
  if(f.type=='bool')i=`<input type='checkbox' id='f${f.id}' ${f.value=='1'?'checked':''}>`;
  else if(f.type=='int'||f.type=='float')i=`<input type='number' id='f${f.id}' value='${f.value}' step='any'>`;
  else i=`<input type='text' id='f${f.id}' value='${f.value}'>`;
  h+=`<div><b>${f.name}</b> (${f.type}): ${i}
  <button onclick="save(${f.id},'${f.type}')">Save</button><br><small>${f.description}</small></div><br>`;
 }
 document.getElementById('fields').innerHTML=h;
};
function save(id,t){
 let v=(t=='bool')?(document.getElementById('f'+id).checked?'1':'0'):document.getElementById('f'+id).value;
 ws.send(JSON.stringify({id:id,value:v}));
}
</script></body></html>)HTML";
            req->send(200, "text/html", html);
        });

        // ---------- debug.html ----------
        server.on("/debug.html", HTTP_GET, [this](AsyncWebServerRequest* req) {
            String html = R"HTML(
<!DOCTYPE html><html><head><meta charset="utf-8"><title>Debug</title></head><body>
<h2>SPIFFS JSON</h2><nav><a href="/">Home</a> | <a href="/metadata.index">Metadata</a></nav><hr>
<pre id="out">Loading...</pre><button onclick="r()">Reload</button>
<script>
function r(){fetch('/debug.json').then(r=>r.text()).then(t=>out.textContent=t);}
r();
</script></body></html>)HTML";
            req->send(200, "text/html", html);
        });
        server.on("/debug.json", HTTP_GET, [this](AsyncWebServerRequest* req) {
            req->send(200, "application/json", model->readJsonFromSPIFFS());
        });

        // ---------- metadata.index ----------
        server.on("/metadata.index", HTTP_GET, [this](AsyncWebServerRequest* req) {
            String html = R"HTML(
<!DOCTYPE html><html><head><meta charset="utf-8"><title>Metadata</title></head><body>
<h2>Metadata Editor</h2>
<nav><a href="/">Home</a> | <a href="/metadata.index">Metadata</a> | <a href="/debug.html">Debug</a></nav><hr>
<table border="1" cellpadding="4"><thead><tr>
<th>ID</th><th>Name</th><th>Type</th><th>Value</th><th>Description</th><th>Action</th></tr></thead>
<tbody id="rows"></tbody></table><br>
<h3>Add New Field</h3>
Name:<input id="n"> Type:<select id="t"><option>string</option><option>int</option><option>float</option><option>bool</option></select>
Value:<input id="v"> Desc:<input id="d"><button onclick="add()">Add</button>
<script>
let ws=new WebSocket('ws://'+location.hostname+':81');
function render(a){
 let h='';
 for(let f of a){
  h+=`<tr><td>${f.id}</td>
  <td><input id='n${f.id}' value='${f.name}'></td>
  <td><select id='t${f.id}'>
    <option ${f.type=='string'?'selected':''}>string</option>
    <option ${f.type=='int'?'selected':''}>int</option>
    <option ${f.type=='float'?'selected':''}>float</option>
    <option ${f.type=='bool'?'selected':''}>bool</option>
  </select></td>
  <td><input id='v${f.id}' value='${f.value}'></td>
  <td><input id='d${f.id}' value='${f.description}'></td>
  <td><button onclick='edit(${f.id})'>Save</button> <button onclick='del(${f.id})'>Del</button></td></tr>`;
 }
 document.getElementById('rows').innerHTML=h;
}
ws.onmessage=e=>render(JSON.parse(e.data));
function del(id){ws.send(JSON.stringify({delete:id}));}
function add(){
 ws.send(JSON.stringify({add:{name:n.value,type:t.value,value:v.value,description:d.value}}));
 n.value=v.value=d.value='';
}
function edit(id){
 ws.send(JSON.stringify({edit:{
  id:id,name:document.getElementById('n'+id).value,
  type:document.getElementById('t'+id).value,
  value:document.getElementById('v'+id).value,
  description:document.getElementById('d'+id).value
 }}));
}
</script></body></html>)HTML";
            req->send(200, "text/html", html);
        });

        server.begin();
    }

    void SerialUpdate() {
        if (!Serial.available()) return;
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line == "?") {
            printHelp();
            return;
        }
        if (line == "j") {
            Serial.println("\n--- /model.json ---");
            Serial.println(model->readJsonFromSPIFFS());
            Serial.println("--------------------\n");
            return;
        }
        int eq = line.indexOf('=');
        if (eq > 0) {
            String name = line.substring(0, eq);
            String val = line.substring(eq + 1);
            Field* f = model->getFieldByName(name);
            if (f) {
                f->value = val;
                model->saveToSPIFFS();
                Serial.printf("[SERIAL] Updated %s = %s\n", f->name.c_str(), val.c_str());
                notifyAllClients();
                printModel();
            } else
                Serial.println("[WARN] Field not found: " + name);
        }
    }

    void loop() {
        ws.loop();
        SerialUpdate();
    }
};

// ==================================================
//                   GLOBALS
// ==================================================
Model model;
Web* web;

void setup() {
    Serial.begin(115200);
    model.fields.push_back(Field("Temperature", 1, "float", "25.0", "Room temp (°C)"));
    model.fields.push_back(Field("LED", 2, "bool", "0", "LED state"));
    model.fields.push_back(Field("Threshold", 3, "int", "100", "Alarm limit"));
    model.loadFromSPIFFS();
    web = new Web(&model);
    web->begin();
}

void loop() {
    web->loop();
}
