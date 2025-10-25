#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WebSocketsServer.h>
#include <WiFi.h>

#include "pass.h"  // preexisting file with getSsid() and getPass()

// -----------------------------
// Field class
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

    String getName() { return name; }
    int getId() { return id; }
    String getType() { return type; }
    String getValue() { return value; }
    String getDescription() { return description; }

    void setValue(String v) { value = v; }
};

// -----------------------------
// Model class
// -----------------------------
class Model {
   private:
    const char* filename = "/model.json";

   public:
    std::vector<Field> fields;

    Model() {}

    void addField(Field f) { fields.push_back(f); }

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
        DeserializationError error = deserializeJson(doc, file);
        file.close();
        if (error) return;

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
// Web class
// -----------------------------
class Web {
   private:
    AsyncWebServer server;
    WebSocketsServer ws;
    Model* model;
    std::vector<Field> initialFields;  // to reset to startup values

    void notifyAllClients(const String& msg) {
        String copy = msg;  // workaround for WebSocketsServer
        ws.broadcastTXT(copy);
    }

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
        String output;
        serializeJson(doc, output);
        return output;
    }

   public:
    Web(Model* m, std::vector<Field> initial) : server(80), ws(81), model(m), initialFields(initial) {}

    void printModel() {
        Serial.println("Current Model:");
        for (auto& f : model->fields) {
            Serial.println(f.getName() + " = " + f.getValue());
        }
        Serial.println("------");
    }

    void resetToInitial() {
        for (size_t i = 0; i < initialFields.size(); i++) {
            Field* f = model->getFieldById(initialFields[i].getId());
            if (f) f->setValue(initialFields[i].getValue());
        }
        model->saveToSPIFFS();
        notifyAllClients(fieldsToJson());
        printModel();
    }

    void begin() {
        // WiFi
        WiFi.begin(getSsid(), getPass());
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }
        Serial.println("\nWiFi connected: " + WiFi.localIP().toString());

        if (!SPIFFS.begin(true)) {
            Serial.println("SPIFFS Mount Failed");
            return;
        }

        // mDNS
        if (!MDNS.begin("biodevice")) {
            Serial.println("Error setting up MDNS responder!");
        } else {
            Serial.println("mDNS responder started: http://biodevice.local");
        }

        Serial.println("Access the web server at:");
        Serial.println("  http://" + WiFi.localIP().toString());
        Serial.println("  http://biodevice.local");

        Serial.println("Current Model JSON:");
        Serial.println(fieldsToJson());

        // WebSocket events
        ws.onEvent([this](uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
            if (type == WStype_CONNECTED) {
                String json = fieldsToJson();
                ws.sendTXT(num, json);
            } else if (type == WStype_TEXT) {
                JsonDocument doc;
                deserializeJson(doc, payload);
                int id = doc["id"];
                String value = doc["value"].as<String>();
                Field* f = model->getFieldById(id);
                if (f) {
                    f->setValue(value);
                    model->saveToSPIFFS();
                    notifyAllClients(fieldsToJson());
                    printModel();
                }
            }
        });
        ws.begin();

        // Serve index.html
        server.on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
            String html = "<!DOCTYPE html><html><head><title>ESP32 Fields</title></head><body>";
            html += "<h2>Fields</h2>";
            html += "<button onclick='reset()'>Reset to Startup Values</button><br><br>";
            html += "<div id='fields'></div>";
            html += R"(
<script>
let ws = new WebSocket('ws://' + location.hostname + ':81');
ws.onmessage = (msg) => {
  let fields = JSON.parse(msg.data);
  let html = '';
  for (let f of fields) {
    let inputHtml = '';
    if(f.type === 'bool') {
        inputHtml = `<input type='checkbox' id='f${f.id}' ${f.value=='1'?'checked':''}>`;
    } else if(f.type === 'int' || f.type === 'float') {
        inputHtml = `<input type='number' step='any' id='f${f.id}' value='${f.value}'>`;
    } else {
        inputHtml = `<input type='text' id='f${f.id}' value='${f.value}'>`;
    }
    html += `<div>
      <b>${f.name}</b> (${f.type}): ${inputHtml}
      <button onclick='save(${f.id},"${f.type}")'>Save</button>
      <br>${f.description}
    </div>`;
  }
  document.getElementById('fields').innerHTML = html;
};

function save(id,type){
  let val;
  if(type==='bool') {
    val = document.getElementById('f'+id).checked ? '1' : '0';
  } else {
    val = document.getElementById('f'+id).value;
  }
  ws.send(JSON.stringify({id:id,value:val}));
}

function reset(){
  ws.send(JSON.stringify({reset:true}));
}
</script>
            )";
            html += "</body></html>";
            request->send(200, "text/html", html);
        });

        // Handle reset messages from WebSocket
        server.begin();
    }

    void SerialUpdate() {
        if (Serial.available()) {
            String line = Serial.readStringUntil('\n');
            line.trim();
            int sep = line.indexOf('=');
            if (sep > 0) {
                String name = line.substring(0, sep);
                String val = line.substring(sep + 1);
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
// Global Model and Web
// -----------------------------
Model model;
Web* web;
std::vector<Field> initialFields;

// -----------------------------
// Setup and Loop
// -----------------------------
void setup() {
    Serial.begin(115200);

    // Sample fields
    initialFields.push_back(Field("Temperature", 1, "float", "25.0", "Room temperature"));
    initialFields.push_back(Field("LED State", 2, "bool", "0", "LED on/off"));
    initialFields.push_back(Field("Device Name", 3, "string", "ESP32", "Device identifier"));
    initialFields.push_back(Field("Threshold", 4, "int", "100", "Alert threshold"));

    // Add initial fields to model
    for (auto& f : initialFields) model.addField(f);

    // Load saved model if exists
    model.loadFromSPIFFS();

    web = new Web(&model, initialFields);
    web->begin();
}   

void loop() {
    web->loop();
}
