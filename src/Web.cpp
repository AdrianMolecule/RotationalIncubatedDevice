#include "Web.h"

Web::Web(Model& m)
    : server(80), ws("/ws"), model(m) {}

void Web::handleWebSocketMessage(void* arg, uint8_t* data, size_t len) {
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        String msg = String((char*)data);
        int sep = msg.indexOf('=');
        if (sep > 0) {
            String fieldName = msg.substring(0, sep);
            String value = msg.substring(sep + 1);
            Field* field = model.getFieldByName(fieldName);
            if (field) {
                field->setValue(value);
                Serial.printf("Updated via WebSocket: %s = %s\n", fieldName.c_str(), value.c_str());
                notifyClients();
            }
        }
    }
}

void Web::onEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                  AwsEventType type, void* arg, uint8_t* data, size_t len) {
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

void Web::notifyClients() {
    String json = "{";
    for (size_t i = 0; i < model.size(); i++) {
        const Field& f = model.fields[i];
        json += "\"" + f.name + "\":\"" + f.getValue() + "\"";
        if (i < model.size() - 1) json += ",";
    }
    json += "}";
    ws.textAll(json);
}

String Web::generateHtmlPage() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Model Fields</title>
  <meta charset="UTF-8">
  <style>
    table { border-collapse: collapse; }
    td, th { padding: 8px; border: 1px solid #ccc; }
    input { width: 100px; }
  </style>
</head>
<body>
  <h2>Model Fields</h2>
  <table>
    <tr><th>ID</th><th>Name</th><th>Type</th><th>Value</th></tr>
)rawliteral";

    for (auto& f : model.fields) {
        html += "<tr>";
        html += "<td>" + String(f.id) + "</td>";
        html += "<td>" + f.name + "</td>";
        html += "<td>" + f.type + "</td>";
        html += "<td><input id=\"" + f.name + "\" type=\"text\" onchange=\"sendUpdate('" + f.name + "')\"></td>";
        html += "</tr>";
    }

    html += R"rawliteral(
  </table>
  <script>
    const ws = new WebSocket(`ws://${window.location.host}/ws`);
    ws.onmessage = (event) => {
      const data = JSON.parse(event.data);
      for (const [key, value] of Object.entries(data)) {
        const input = document.getElementById(key);
        if (input && input.value != value) input.value = value;
      }
    };
    function sendUpdate(field) {
      const val = document.getElementById(field).value;
      ws.send(`${field}=${val}`);
    }
  </script>
</body>
</html>
)rawliteral";

    return html;
}

void Web::begin(const String& ssid, const String& password) {
    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.println("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    ws.onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client,
                      AwsEventType type, void* arg, uint8_t* data, size_t len) {
        this->onEvent(server, client, type, arg, data, len);
    });
    server.addHandler(&ws);

    server.on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
        request->send(200, "text/html", generateHtmlPage());
    });

    server.begin();
}

void Web::updateClientValues() {
    notifyClients();
}
