#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WebSocketsServer.h>
#include <WiFi.h>

#include "pass.h"  // getSsid(), getPass()

// ==================================================
//                 FIELD CLASS
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

    String getName() const { return name; }
    int getId() const { return id; }
    String getType() const { return type; }
    String getValue() const { return value; }
    String getDescription() const { return description; }
    void setValue(const String& v) { value = v; }
};

// ==================================================
//                 MODEL CLASS
// ==================================================
class Model {
   private:
    const char* filename = "/model.json";

   public:
    std::vector<Field> fields;
    bool loadedFromSPIFFS = false;

    void addField(const Field& f) { fields.push_back(f); }

    Field* getFieldById(int id) {
        for (auto& f : fields)
            if (f.getId() == id) return &f;
        return nullptr;
    }

    Field* getFieldByName(const String& name) {
        for (auto& f : fields)
            if (f.getName() == name) return &f;
        return nullptr;
    }

    void loadFromSPIFFS() {
        if (!SPIFFS.exists(filename)) {
            loadedFromSPIFFS = false;
            return;
        }
        File file = SPIFFS.open(filename, "r");
        if (!file) {
            loadedFromSPIFFS = false;
            return;
        }
        JsonDocument doc;
        if (deserializeJson(doc, file)) {
            file.close();
            loadedFromSPIFFS = false;
            return;
        }
        file.close();

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
            JsonObject obj = arr.createNestedObject();
            obj["name"] = f.getName();
            obj["id"] = f.getId();
            obj["type"] = f.getType();
            obj["value"] = f.getValue();
            obj["description"] = f.getDescription();
        }

        File file = SPIFFS.open(filename, "w");
        if (!file) {
            Serial.println("[ERROR] Failed to open /model.json for writing!");
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
};

// ==================================================
//                 WEB CLASS
// ==================================================
class Web {
   private:
    AsyncWebServer server;
    WebSocketsServer ws;
    Model* model;
    std::vector<Field> initialFields;

    String fieldsToJson() {
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();
        for (auto& f : model->fields) {
            JsonObject obj = arr.createNestedObject();
            obj["id"] = f.getId();
            obj["name"] = f.getName();
            obj["type"] = f.getType();
            obj["value"] = f.getValue();
            obj["description"] = f.getDescription();
        }
        String out;
        serializeJson(doc, out);
        return out;
    }

    void notifyAllClients(const String& msg) {
        String copy = msg;
        ws.broadcastTXT(copy);
    }

    void printModel() {
        Serial.println("Current Model Values:");
        for (auto& f : model->fields)
            Serial.printf("  %-15s = %-10s (%s)\n",
                          f.getName().c_str(),
                          f.getValue().c_str(),
                          f.getType().c_str());
        Serial.println("------------------------------");
    }

    void printHelp() {
        Serial.println("\n=== Available Fields ===");
        for (auto& f : model->fields) {
            Serial.printf("  %-15s (%s): %s [current: %s]\n",
                          f.getName().c_str(), f.getType().c_str(),
                          f.getDescription().c_str(), f.getValue().c_str());
        }
        Serial.println("\nUse syntax: FieldName=newValue");
        Serial.println("Example: Temperature=27.5 or LED State=1");
        Serial.println("Type '?' to show this list again.\n");
    }

    void resetToInitial() {
        for (auto& init : initialFields) {
            Field* f = model->getFieldById(init.getId());
            if (f) f->setValue(init.getValue());
        }
        model->saveToSPIFFS();
        notifyAllClients(fieldsToJson());
        Serial.println("\n[INFO] Model reset to startup values.");
        printModel();
    }

   public:
    Web(Model* m, const std::vector<Field>& initial)
        : server(80), ws(81), model(m), initialFields(initial) {}

    void begin() {
        if (!SPIFFS.begin(true)) {
            Serial.println("[ERROR] SPIFFS mount failed!");
            return;
        }

        WiFi.begin(getSsid(), getPass());
        Serial.print("Connecting to WiFi");
        while (WiFi.status() != WL_CONNECTED) {
            delay(400);
            Serial.print(".");
        }
        Serial.println("\nConnected! IP: " + WiFi.localIP().toString());

        if (MDNS.begin("biodevice")) {
            Serial.println("mDNS responder started: http://biodevice.local");
        } else {
            Serial.println("[WARN] Failed to start mDNS");
        }

        Serial.println("Access URLs:");
        Serial.println("  http://" + WiFi.localIP().toString());
        Serial.println("  http://biodevice.local");

        if (model->loadedFromSPIFFS)
            Serial.println("[INFO] Model loaded from SPIFFS (/model.json)");
        else
            Serial.println("[INFO] No saved model found. Using initial startup values.");

        printModel();
        printHelp();

        // --------------------------
        // WebSocket handler
        // --------------------------
        ws.onEvent([this](uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
            if (type == WStype_CONNECTED) {
                String json = fieldsToJson();
                ws.sendTXT(num, json);
            } else if (type == WStype_TEXT) {
                JsonDocument doc;
                if (deserializeJson(doc, payload)) return;
                if (doc.containsKey("reset")) {
                    resetToInitial();
                    return;
                }
                int id = doc["id"];
                String val = doc["value"].as<String>();
                Field* f = model->getFieldById(id);
                if (f) {
                    f->setValue(val);
                    model->saveToSPIFFS();
                    notifyAllClients(fieldsToJson());
                    printModel();
                }
            }
        });
        ws.begin();

        // --------------------------
        // index.html
        // --------------------------
        server.on("/", HTTP_GET, [this](AsyncWebServerRequest* req) {
            String html = R"HTML(
<!DOCTYPE html><html><head><meta charset="utf-8">
<title>ESP32 Model Sync</title></head><body>
<h2>ESP32 Model Fields</h2>
<nav><a href="/">Home</a> | <a href="/debug.html">Debug JSON</a></nav><hr>
<button onclick="reset()">Reset to Startup Values</button><br><br>
<div id="fields">Loading...</div>
<script>
let ws = new WebSocket('ws://' + location.hostname + ':81');
ws.onmessage = e => {
  let fields = JSON.parse(e.data);
  let html = '';
  for (let f of fields) {
    let inputHtml = '';
    if (f.type === 'bool') {
      inputHtml = `<input type='checkbox' id='f${f.id}' ${f.value=='1'?'checked':''}>`;
    } else if (f.type === 'int' || f.type === 'float') {
      inputHtml = `<input type='number' step='any' id='f${f.id}' value='${f.value}'>`;
    } else {
      inputHtml = `<input type='text' id='f${f.id}' value='${f.value}'>`;
    }
    html += `<div><b>${f.name}</b> (${f.type}): ${inputHtml}
      <button onclick="save(${f.id},'${f.type}')">Save</button><br>
      <small>${f.description}</small></div><br>`;
  }
  document.getElementById('fields').innerHTML = html;
};
function save(id,type){
  let val = (type==='bool') ? 
      (document.getElementById('f'+id).checked ? '1' : '0') :
      document.getElementById('f'+id).value;
  ws.send(JSON.stringify({id:id,value:val}));
}
function reset(){
  ws.send(JSON.stringify({reset:true}));
}
</script></body></html>
)HTML";
            req->send(200, "text/html", html);
        });

        // --------------------------
        // debug.html
        // --------------------------
        server.on("/debug.html", HTTP_GET, [this](AsyncWebServerRequest* req) {
            String html = R"HTML(
<!DOCTYPE html><html><head><meta charset="utf-8">
<title>Debug JSON</title></head><body>
<h2>SPIFFS Model JSON</h2>
<nav><a href="/">Home</a> | <a href="/debug.html">Debug JSON</a></nav><hr>
<pre id="content">Loading...</pre>
<button onclick="reload()">Reload</button>
<script>
function reload() {
  fetch('/debug.json')
    .then(r => r.text())
    .then(t => document.getElementById('content').textContent = t);
}
reload();
</script>
</body></html>
)HTML";
            req->send(200, "text/html", html);
        });

        // --------------------------
        // debug.json endpoint
        // --------------------------
        server.on("/debug.json", HTTP_GET, [this](AsyncWebServerRequest* req) {
            req->send(200, "application/json", model->readJsonFromSPIFFS());
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
        int eq = line.indexOf('=');
        if (eq > 0) {
            String name = line.substring(0, eq);
            String val = line.substring(eq + 1);
            Field* f = model->getFieldByName(name);
            if (f) {
                f->setValue(val);
                model->saveToSPIFFS();
                notifyAllClients(fieldsToJson());
                printModel();
            } else {
                Serial.println("[WARN] Field not found: " + name);
            }
        }
    }

    void loop() {
        ws.loop();
        SerialUpdate();
    }
};

// ==================================================
//                 GLOBALS & SETUP
// ==================================================
Model model;
Web* web;
std::vector<Field> initialFields;

void setup() {
    Serial.begin(115200);

    initialFields.push_back(Field("Temperature", 1, "float", "25.0", "Room temperature (°C)"));
    initialFields.push_back(Field("LED State", 2, "bool", "0", "Turn LED on/off"));
    initialFields.push_back(Field("Device Name", 3, "string", "ESP32", "Device identifier"));
    initialFields.push_back(Field("Threshold", 4, "int", "100", "Alert threshold"));

    for (auto& f : initialFields) model.addField(f);
    model.loadFromSPIFFS();

    web = new Web(&model, initialFields);
    web->begin();
}

void loop() {
    web->loop();
}
