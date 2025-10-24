#include "Web.h"

#include <ESPmDNS.h>

#include "WebPageIndex.h"

Web::Web(Model& m)
    : server(80), ws("/ws"), model(m) {}

void Web::begin(const String& ssid, const String& password) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    WiFi.setHostname("ad");

    Serial.println("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    if (MDNS.begin("ad")) {
        Serial.println("mDNS responder started as: http://ad.local");
    }

    ws.onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client,
                      AwsEventType type, void* arg, uint8_t* data, size_t len) {
        this->onEvent(server, client, type, arg, data, len);
    });
    server.addHandler(&ws);

    server.on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
        request->send(200, "text/html", generateHtmlPageIndex(model));
    });

    server.begin();
}

void Web::onEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                  AwsEventType type, void* arg, uint8_t* data, size_t len) {
    if (type != WS_EVT_DATA) return;

    String msg = "";
    for (size_t i = 0; i < len; i++) msg += (char)data[i];

    int eq = msg.indexOf('=');
    if (eq < 0) return;

    String field = msg.substring(0, eq);
    String value = msg.substring(eq + 1);
    model.setValue(field, value);

    Serial.println("Updated " + field + " = " + value);

    updateClientValues();
}

void Web::updateClientValues() {
    String json = "{";
    for (size_t i = 0; i < model.fields.size(); i++) {
        Field& f = model.fields[i];
        json += "\"" + f.name + "\":\"" + f.getValue() + "\"";
        if (i < model.fields.size() - 1) json += ",";
    }
    json += "}";
    ws.textAll(json);
}
