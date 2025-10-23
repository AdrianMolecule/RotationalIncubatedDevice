// #include <Arduino.h>
// #include <AsyncTCP.h>
// #include <ESPAsyncWebServer.h>
// #include <WiFi.h>

// #include <vector>

// #include "pass.h"
// //WORKS!!!
// // ===================== Model class =====================
// class Model {
//    public:
//     std::vector<int> fields;

//     Model(size_t count, int initial = 0) {
//         fields.resize(count, initial);
//     }

//     size_t size() const { return fields.size(); }

//     int get(size_t i) const { return fields[i]; }
//     void set(size_t i, int value) {
//         if (i < fields.size()) {
//         }
//         fields[i] = value;
//     }
// };

// Model model(5, 0);  // Creates F1..F5 with value 0

// // ===================== Globals =====================
// AsyncWebServer server(80);
// AsyncWebSocket ws("/ws");

// // ===================== Forward declarations =====================
// void notifyClients();
// String generateHtmlPage(size_t fieldCount);

// // ===================== WebSocket events =====================
// void handleWebSocketMessage(void* arg, uint8_t* data, size_t len) {
//     AwsFrameInfo* info = (AwsFrameInfo*)arg;
//     if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
//         String msg = String((char*)data);
//         // Expected format: "F1=123" or "F2=456"
//         int sep = msg.indexOf('=');
//         if (sep > 0) {
//             String field = msg.substring(0, sep);
//             int value = msg.substring(sep + 1).toInt();
//             if (field.startsWith("F")) {
//                 int index = field.substring(1).toInt() - 1;
//                 if (index >= 0 && index < (int)model.size()) {
//                     model.set(index, value);
//                     Serial.printf("Updated via WebSocket: %s = %d\n", field.c_str(), value);
//                     notifyClients();
//                 }
//             }
//         }
//     }
// }

// void onEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
//              AwsEventType type, void* arg, uint8_t* data, size_t len) {
//     switch (type) {
//         case WS_EVT_CONNECT:
//             Serial.printf("WebSocket client #%u connected\n", client->id());
//             notifyClients();
//             break;
//         case WS_EVT_DISCONNECT:
//             Serial.printf("WebSocket client #%u disconnected\n", client->id());
//             break;
//         case WS_EVT_DATA:
//             handleWebSocketMessage(arg, data, len);
//             break;
//         default:
//             break;
//     }
// }

// // ===================== Notify clients =====================
// void notifyClients() {
//     String json = "{";
//     for (size_t i = 0; i < model.size(); i++) {
//         json += "\"F" + String(i + 1) + "\":" + String(model.get(i));
//         if (i < model.size() - 1) json += ",";
//     }
//     json += "}";
//     ws.textAll(json);
// }

// // ===================== Serial input handler =====================
// void handleSerialInput() {
//     if (Serial.available()) {
//         String input = Serial.readStringUntil('\n');
//         input.trim();
//         int sep = input.indexOf('=');
//         if (sep > 0) {
//             String field = input.substring(0, sep);
//             int value = input.substring(sep + 1).toInt();
//             if (field.startsWith("F")) {
//                 int index = field.substring(1).toInt() - 1;
//                 if (index >= 0 && index < (int)model.size()) {
//                     model.set(index, value);
//                     Serial.printf("Updated via Serial: %s = %d\n", field.c_str(), value);
//                     notifyClients();
//                 }
//             } else {
//                 Serial.println("Use format: F1=123");
//             }
//         }
//     }
// }

// // ===================== HTML Page generator =====================
// String generateHtmlPage(size_t fieldCount) {
//     String html = R"rawliteral(
// <!DOCTYPE html>
// <html>
// <head>
//   <title>ESP32 Dynamic Model Sync</title>
//   <meta charset="UTF-8">
//   <style>
//     table { border-collapse: collapse; }
//     td, th { padding: 8px; border: 1px solid #ccc; }
//     input { width: 80px; }
//   </style>
// </head>
// <body>
//   <h2>Model Fields</h2>
//   <table>
//     <tr><th>Field</th><th>Value</th></tr>
// )rawliteral";

//     for (size_t i = 0; i < fieldCount; i++) {
//         html += "<tr><td>F" + String(i + 1) + "</td><td><input id=\"F" + String(i + 1) + "\" type=\"number\" onchange=\"sendUpdate('F" + String(i + 1) + "')\"></td></tr>";
//     }

//     html += R"rawliteral(
//   </table>

//   <script>
//     const ws = new WebSocket(`ws://${window.location.host}/ws`);

//     ws.onmessage = (event) => {
//       const data = JSON.parse(event.data);
//       for (const [key, value] of Object.entries(data)) {
//         const input = document.getElementById(key);
//         if (input && input.value != value) input.value = value;
//       }
//     };

//     function sendUpdate(field) {
//       const val = document.getElementById(field).value;
//       ws.send(`${field}=${val}`);
//     }
//   </script>
// </body>
// </html>
// )rawliteral";

//     return html;
// }

// // ===================== Setup =====================
// void setup() {
//     Serial.begin(115200);
//     WiFi.begin(readSsid(), readPassword());
//     Serial.println("Connecting to WiFi...");
//     while (WiFi.status() != WL_CONNECTED) {
//         delay(500);
//         Serial.print(".");
//     }
//     Serial.println("\nConnected!");
//     Serial.print("IP Address: ");
//     Serial.println(WiFi.localIP());

//     ws.onEvent(onEvent);
//     server.addHandler(&ws);

//     // Serve dynamically generated HTML
//     server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
//         String html = generateHtmlPage(model.size());
//         request->send(200, "text/html", html);
//     });

//     server.begin();
// }

// // ===================== Loop =====================
// void loop() {
//     handleSerialInput();
// }
