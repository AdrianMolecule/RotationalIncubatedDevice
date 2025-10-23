#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "pass.h"

// ===================== Model class =====================
class Model {
public:
    int f1;
    int f2;

    Model(int a = 0, int b = 0) : f1(a), f2(b) {}
};

Model model(10, 20);

// ===================== Globals =====================
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Forward declarations
void notifyClients();

// ===================== WebSocket events =====================
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        String msg = String((char *)data);
        // Expected format: "f1=123" or "f2=456"
        int sep = msg.indexOf('=');
        if (sep > 0) {
            String field = msg.substring(0, sep);
            int value = msg.substring(sep + 1).toInt();
            if (field == "f1") model.f1 = value;
            else if (field == "f2") model.f2 = value;
            Serial.printf("Updated via WebSocket: %s = %d\n", field.c_str(), value);
            notifyClients();
        }
    }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
             AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
    case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected\n", client->id());
        notifyClients();
        break;
    case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
    case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
    default:
        break;
    }
}

// ===================== HTML page =====================
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Model Sync</title>
  <meta charset="UTF-8">
  <style>
    table { border-collapse: collapse; }
    td, th { padding: 8px; border: 1px solid #ccc; }
    input { width: 80px; }
  </style>
</head>
<body>
  <h2>Model Fields</h2>
  <table>
    <tr><th>Field</th><th>Value</th></tr>
    <tr><td>f1</td><td><input id="f1" type="number" onchange="sendUpdate('f1')"></td></tr>
    <tr><td>f2</td><td><input id="f2" type="number" onchange="sendUpdate('f2')"></td></tr>
  </table>

  <script>
    const ws = new WebSocket(`ws://${window.location.host}/ws`);

    ws.onmessage = (event) => {
      const data = JSON.parse(event.data);
      document.getElementById('f1').value = data.f1;
      document.getElementById('f2').value = data.f2;
    };

    function sendUpdate(field) {
      const val = document.getElementById(field).value;
      ws.send(`${field}=${val}`);
    }
  </script>
</body>
</html>
)rawliteral";

// ===================== Notify clients =====================
void notifyClients() {
    String json = "{\"f1\":" + String(model.f1) + ",\"f2\":" + String(model.f2) + "}";
    ws.textAll(json);
}

// ===================== Serial input handler =====================
void handleSerialInput() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        int sep = input.indexOf('=');
        if (sep > 0) {
            String field = input.substring(0, sep);
            int value = input.substring(sep + 1).toInt();
            if (field == "f1") model.f1 = value;
            else if (field == "f2") model.f2 = value;
            Serial.printf("Updated via Serial: %s = %d\n", field.c_str(), value);
            notifyClients();
        } else {
            Serial.println("Use format: f1=123 or f2=456");
        }
    }
}

// ===================== Setup =====================
void setup() {
    Serial.begin(115200);
    WiFi.begin(readSsid(), readPassword());
    Serial.println("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    ws.onEvent(onEvent);
    server.addHandler(&ws);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", htmlPage);
    });

    server.begin();
}

// ===================== Loop =====================
void loop() {
    handleSerialInput();
}
