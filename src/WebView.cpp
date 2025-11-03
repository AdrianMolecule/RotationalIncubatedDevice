// #include "WebView.h"

// #include <Arduino.h>
// #include <AsyncTCP.h>
// #include <ESPAsyncWebServer.h>
// #include <SPIFFS.h>

// #include "Controller.h"
// #include "Field.h"
// #include "Helper.h"

// AsyncWebServer WebView::server(80);
// AsyncWebSocket WebView::ws("/ws");


// void WebView::begin() {

//     // ========== ROUTES ==========
//     server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
//         String html = Helper::generateIndexPage(true);
//         request->send(200, "text/html", html);
//     });

//     server.on("/metadata", HTTP_GET, [](AsyncWebServerRequest* request) {
//         String html = Helper::generateMetadataPage();
//         request->send(200, "text/html", html);
//     });

//     server.on("/debug", HTTP_GET, [](AsyncWebServerRequest* request) {
//         String html = Helper::generateDebugPage();
//         request->send(200, "text/html", html);
//     });

//     // Optional: REST API for model retrieval
//     server.on("/api/model", HTTP_GET, [](AsyncWebServerRequest* request) {
//         String json = JsonWrapper::fieldsToJsonString(Controller::getModel().getFields());
//         request->send(200, "application/json", json);
//     });

//     // WebSocket for live updates
//     ws.onEvent(handleWebSocketEvent);
//     server.addHandler(&ws);

//     // Catch-all handler for debugging 404s
//     server.onNotFound([](AsyncWebServerRequest* request) {
//         Serial.printf("[WEB] 404 Not Found: %s\n", request->url().c_str());
//         request->send(404, "text/plain", "Not found");
//     });

//     // Start server
//     server.begin();
//     Serial.println("[WEB] Server started on port 80");
// }

// void WebView::broadcast(const String& json) {
//     ws.textAll(json);
// }

// void WebView::handleWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
//                                    AwsEventType type, void* arg, uint8_t* data, size_t len) {
//     if (type == WS_EVT_CONNECT) {
//         Serial.printf("[WS] Client connected: %u\n", client->id());
//     } else if (type == WS_EVT_DISCONNECT) {
//         Serial.printf("[WS] Client disconnected: %u\n", client->id());
//     } else if (type == WS_EVT_DATA) {
//         String msg = String((char*)data).substring(0, len);
//         Serial.printf("[WS] Received: %s\n", msg.c_str());
//         Controller::handleWebSocketMessage(msg);
//     }
// }
