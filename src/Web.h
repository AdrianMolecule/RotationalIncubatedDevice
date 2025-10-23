#pragma once
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <WiFi.h>
#include "Model.h"

class Web {
private:
    AsyncWebServer server;
    AsyncWebSocket ws;
    Model &model;

    void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
    void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                 AwsEventType type, void *arg, uint8_t *data, size_t len);
    void notifyClients();
    String generateHtmlPage();

public:
    Web(Model &m);
    void begin(const String &ssid, const String &password);
    void updateClientValues(); // to be called externally if model changes
};
