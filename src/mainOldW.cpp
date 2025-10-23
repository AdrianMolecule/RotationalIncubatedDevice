// #include <Arduino.h>
// #include <AsyncTCP.h>
// #include <ESPAsyncWebServer.h>
// #include <WiFi.h>

// #include <vector>

// #include "pass.h"

// // ===================== Field class =====================
// class Field {
//    private:
//     String value;

//    public:
//     String name;
//     String type;  // e.g. "int", "float", "string"
//     int id;

//     Field(String n, String t, int i, String v = "")
//         : name(n), type(t), id(i), value(v) {}

//     String getValue() const { return value; }
//     void setValue(const String& v) { value = v; }
// };

// // ===================== Model class =====================
// class Model {
//    public:
//     std::vector<Field> fields;

//     Model(std::initializer_list<Field> initList) : fields(initList) {}

//     size_t size() const { return fields.size(); }

//     Field* getFieldById(int id) {
//         for (auto& f : fields)
//             if (f.id == id) return &f;
//         return nullptr;
//     }

//     Field* getFieldByName(const String& name) {
//         for (auto& f : fields)
//             if (f.name == name) return &f;
//         return nullptr;
//     }
// };

// // Example: create fields F1..F3
// Model model = {
//     Field("First int", "int", 1, "10"),
//     Field("Second int", "int", 2, "20"),
//     Field("First string", "string", 2, "20"),
//     Field("Float field", "float", 3, "3.14")};

// // ===================== Globals =====================
// AsyncWebServer server(80);
// AsyncWebSocket ws("/ws");

// // ===================== Forward declarations =====================
// void notifyClients();
// String generateHtmlPage();

// // ===================== WebSocket events =====================
// void handleWebSocketMessage(void* arg, uint8_t* data, size_t len) {//From HTML to model for motor commands and desired values
//     AwsFrameInfo* info = (AwsFrameInfo*)arg;
//     if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
//         String msg = String((char*)data);
//         // Expected format: "F1=123" or "temperature=45"
//         int sep = msg.indexOf('=');
//         if (sep > 0) {
//             String fieldName = msg.substring(0, sep);
//             String value = msg.substring(sep + 1);
//             Field* field = model.getFieldByName(fieldName);
//             if (field) {
//                 field->setValue(value);
//                 Serial.printf("Updated via WebSocket: %s = %s\n", fieldName.c_str(), value.c_str());
//                 notifyClients();
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
//         const Field& f = model.fields[i];
//         json += "\"" + f.name + "\":\"" + f.getValue() + "\"";
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
//             String name = input.substring(0, sep);
//             String value = input.substring(sep + 1);
//             Field* field = model.getFieldByName(name);
//             if (field) {
//                 field->setValue(value);
//                 Serial.printf("Updated via Serial: %s = %s\n", name.c_str(), value.c_str());
//                 notifyClients();
//             } else {
//                 Serial.println("Unknown field name");
//             }
//         } else {
//             Serial.println("Use format: F1=123 or fieldName=value");
//         }
//     }
// }

// // ===================== HTML generator =====================
// String generateHtmlPage() {
//     String html = R"rawliteral(
// <!DOCTYPE html>
// <html>
// <head>
//   <title>ESP32 Field Model</title>
//   <meta charset="UTF-8">
//   <style>
//     table { border-collapse: collapse; }
//     td, th { padding: 8px; border: 1px solid #ccc; }
//     input { width: 100px; }
//   </style>
// </head>
// <body>
//   <h2>Model Fields</h2>
//   <table>
//     <tr><th>ID</th><th>Name</th><th>Type</th><th>Value</th></tr>
// )rawliteral";

//     for (auto& f : model.fields) {
//         html += "<tr>";a
//         html += "<td>" + String(f.id) + "</td>";
//         html += "<td>" + f.name + "</td>";
//         html += "<td>" + f.type + "</td>";
//         html += "<td><input id=\"" + f.name + "\" type=\"text\" onchange=\"sendUpdate('" + f.name + "')\"></td>";
//         html += "</tr>";
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

//     server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
//         String html = generateHtmlPage();
//         request->send(200, "text/html", html);
//     });

//     server.begin();
// }

// // ===================== Loop =====================
// void loop() {
//     handleSerialInput();
// }

