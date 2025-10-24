#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "Model.h"

// Class Web encapsulates all network functionality
// - Handles WiFi connection
// - Manages AsyncWebServer and AsyncWebSocket
// - Communicates with the Model to sync values with browser clients
class Web {
private:
    AsyncWebServer server;
    AsyncWebSocket ws;
    Model &model;

    void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                 AwsEventType type, void *arg, uint8_t *data, size_t len);

public:
    Web(Model &m);
    void begin(const String &ssid, const String &password);
    void updateClientValues();  // Push model updates to browser
};
