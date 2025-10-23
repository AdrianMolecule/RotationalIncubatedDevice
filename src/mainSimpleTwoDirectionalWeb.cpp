// #include <WebServer.h>
// #include <WebSocketsServer.h>
// #include <WiFi.h>
// #include "pass.h"

// const char* ssid = "YOUR_SSID";          // Replace with your WiFi SSID
// const char* password = "YOUR_PASSWORD";  // Replace with your WiFi password

// WebServer server(80);
// WebSocketsServer webSocket = WebSocketsServer(81);

// int sensorValue = 0;
// int screenToBoard = 0;

// void setup() {
//     Serial.begin(115200);
//     WiFi.begin(readSsi(), readPassword());

//     while (WiFi.status() != WL_CONNECTED) {
//         delay(1000);
//         Serial.println("Connecting to WiFi...");
//     }
//     Serial.println("Connected to WiFi");

//     server.on("/", HTTP_GET, []() {
//         server.send(200, "text/html", htmlPage());
//     });

//     webSocket.onEvent(webSocketEvent);
//     server.begin();
//     webSocket.begin();
// }

// void loop() {
//     server.handleClient();
//     webSocket.loop();
//     readSensorValue();
// }

// void readSensorValue() {
//     if (Serial.available() > 0) {
//         sensorValue = Serial.parseInt();
//         webSocket.broadcastTXT(String(sensorValue));  // Send updated sensor value to all connected clients
//     }
// }

// void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
//     if (type == WStype_TEXT) {
//         String message = String((char*)payload);
//         if (message.startsWith("screenToBoard:")) {
//             screenToBoard = message.substring(14).toInt();
//             Serial.print("screenToBoard updated to: ");
//             Serial.println(screenToBoard);
//         }
//     }
// }

// String htmlPage() {
//     String html = "<!DOCTYPE html><html><head><title>ESP32 WebSocket</title>";
//     html += "<script>var connection = new WebSocket('ws://' + window.location.hostname + ':81/');";
//     html += "connection.onmessage = function (event) { document.getElementById('sensorValue').innerText = event.data; };";
//     html += "function sendValue() { var value = document.getElementById('screenToBoard').value; connection.send('screenToBoard:' + value); }</script>";
//     html += "</head><body><h1>ESP32 WebSocket Example</h1>";
//     html += "<p>Sensor Value: <span id='sensorValue'>0</span></p>";
//     html += "<input type='number' id='screenToBoard' onchange='sendValue()' />";
//     html += "</body></html>";
//     return html;
// }
