#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WebSocketsServer.h>
#include <WiFi.h>

#include "pass.h"  // Provides getSsid() and getPass()

// -----------------------------
// Field class: represents one data field
// -----------------------------
class Field {
   public:
    String name;
    int id;
    String type;  // "int", "float", "string", "bool"
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

// -----------------------------
// Model class: collection of fields with SPIFFS persistence
// -----------------------------
class Model {
   private:
    const char* filename = "/model.json";

   public:
    std::vector<Field> fields;

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
        if (!SPIFFS.exists(filename)) return;
        File file = SPIFFS.open(filename, "r");
        if (!file) return;
        JsonDocument doc;
        if (deserializeJson(doc, file)) return;
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
        if (!file) return;
        serializeJson(doc, file);
        file.close();
    }
};

// -----------------------------
// Web class: handles Wi-Fi, WebServer, WebSocket, and Serial sync
// -----------------------------
class Web {
   private:
    AsyncWebServer server;
    WebSocketsServer ws;
    Model* model;
    std::vector<Field> initialFields;  // snapshot of startup values

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
        String copy = msg;  // workaround for sendTXT(String&)
        ws.broadcastTXT(copy);
    }

    void printModel() {
        Serial.println("Current Model:");
        for (auto& f : model->fields)
            Serial.println(f.getName() + " = " + f.getValue());
        Serial.println("------------------");
    }

    void resetToInitial() {
        for (auto& init : initialFields) {
            Field* f = model->getFieldById(init.getId());
            if (f) f->setValue(init.getValue());
        }
        model->saveToSPIFFS();
        notifyAllClients(fieldsToJson());
        printModel();
    }

   public:
    Web(Model* m, const std::vector<Field>& initial)
        : server(80), ws(81), model(m), initialFields(initial) {}

    void begin() {
        if (!SPIFFS.begin(true)) {
            Serial.println("SPIFFS mount failed!");
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
            Serial.println("Failed to start mDNS");
        }

        Serial.println("Access URLs:");
        Serial.println("  http://" + WiFi.localIP().toString());
        Serial.println("  http://biodevice.local");
        Serial.println("Loaded model source:");
        printModel();

        // WebSocket handler
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

        // Serve index page
        server.on("/", HTTP_GET, [this](AsyncWebServerRequest* req) {
            String html = R"HTML(
<!DOCTYPE html><html><head><meta charset="utf-8">
<title>ESP32 Model Sync</title></head><body>
<h2>ESP32 Model Fields</h2>
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

        server.begin();
    }

    void SerialUpdate() {
        if (Serial.available()) {
            String line = Serial.readStringUntil('\n');
            line.trim();
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
                    Serial.println("Field not found: " + name);
                }
            }
        }
    }

    void loop() {
        ws.loop();
        SerialUpdate();
    }
};

// -----------------------------
// Global objects
// -----------------------------
Model model;
Web* web;
std::vector<Field> initialFields;

// -----------------------------
// Setup and Loop
// -----------------------------
void setup() {
    Serial.begin(115200);

    // Initial field definitions
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
