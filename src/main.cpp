#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>

#include <vector>

#include "BackEnd.h"
#include "Controller.h"
#include "Field.h"
#include "Helper.h"
#include "HtmlHelper.h"
#include "JsonWrapper.h"
#include "Model.h"
#include "Pass.h"
#include "TimeManager.h"

AsyncWebServer server(80);  // needs to persist beyond the method
void setupOTA();

void handleWebSocketMessage(String msg) {  // from the UI to board
    JsonDocument doc;
    if (deserializeJson(doc, msg)) return;
    String action = doc["action"] | "";
    if (action == "update") {
        String id = doc["id"] | "";
        String val = doc["value"] | "";
        Field* f = Controller::model.getById(id);
        if (f && !f->getReadOnly()) {
            f->setValue(val);
            Serial.println("[SYS] From UI set value for:" + f->getName() + " value:" + f->getValue());
            if (f->getIsPersisted()) {
                Controller::model.saveToFile();
            }
            Controller::webSocket.textAll(Controller::model.toJsonString());
        }
    } else if (action == "delete") {
        String id = doc["id"] | "";
        if (Controller::model.remove(id)) {
            Controller::model.saveToFile();
            Controller::webSocket.textAll(Controller::model.toJsonString());
        }
    } else if (action == "moveUp" || action == "moveDown") {
        String id = doc["id"] | "";
        Controller::model.reorder(id, action == "moveUp");
        Controller::model.saveToFile();
        Controller::webSocket.textAll(Controller::model.toJsonString());
    } else if (action == "add") {
        JsonObject fld = doc["field"].as<JsonObject>();
        Field f;
        f.fromJson(fld);
        Controller::model.add(f);
        Controller::model.saveToFile();
        Controller::webSocket.textAll(Controller::model.toJsonString());
    } else if (action == "uploadModel") {
        Serial.println("got a uploadModel Ws action");
        String jsonStr = doc["json"] | "";
        if (JsonWrapper::checkJson(jsonStr)) {
            Controller::model.loadFromJson(jsonStr);
            Controller::model.saveToFile();
            Serial.println("[WEB] Uploaded new model via Advanced page");
        } else {
            Serial.println("[WEB] Invalid JSON upload ignored");
        }
        Controller::webSocket.textAll(Controller::model.toJsonString());
    } else if (action == "factoryReset") {
        Serial.println("[WEB] Factory reset requested from Advanced page");
        Controller::model.initialize();
        Controller::model.saveToFile();
        Controller::webSocket.textAll(Controller::model.toJsonString());
    } else if (action == "showFactoryModel") {
        Serial.println("[WEB] Showing factory default model (object view)");
        Model temp;
        temp.initialize();  // create default fields
        Controller::webSocket.textAll(temp.toJsonString());
    } else if (action == "showFactoryJson") {
        Serial.println("[WEB] Showing factory default model (raw JSON view)");
        Model temp;
        temp.initialize();
        String json = temp.toJsonString();
        Controller::webSocket.textAll(json);
    }
}

// NTP Server settings
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -18000;    // Set your GMT offset in seconds (e.g., EST is -5 hours * 3600 seconds/hour = -18000)
const int daylightOffset_sec = 3600;  // Set your daylight saving offset in seconds (e.g., 1 hour = 3600 seconds)
//
void loopBackendTask(void* param);

void spiffInit() {
    // Attempt to mount without formatting
    if (!SPIFFS.begin(false)) {
        Serial.println("E (1843) SPIFFS: mount failed, -10025: Trying to format...");
        Controller::fatalErrorAlarm("[FS] Mount failed! Trying to reformat");  
        // Attempt to format and mount (this is the crucial step)
        if (!SPIFFS.begin(true)) {
            // If the format/mount also fails, this is a serious, unrecoverable error
            Serial.println("FATAL: Format and Mount failed! Stopping here.");
            // Stop execution, e.g., while(true); or an error handler
            while (true);
        } else {
            // SUCCESS after a forced format
            Serial.println("[FS] Reformat successful and Mounted.");
        }
    } else {
        // SUCCESS on the initial attempt
        Serial.println("[FS] Mounted successfully.");
    }
}
//
void setup() {
    Serial.begin(115200);
    MyMusic::play(MyMusic::wakeUp);
    Serial.println("[SYS] Booting...");
    WiFi.begin(getSsid(), getPass());
    Serial.printf("[WiFi] Connecting to %s ", getSsid());
    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 20) {
        delay(500);
        Serial.print(".");
        -timeout++;
    }
    Serial.println();
    String wifiStatus;
    String when;
    if (WiFi.status() == WL_CONNECTED) {
        MyMusic::play(MyMusic::wifi);
        wifiStatus = "[WiFi] IP: " + WiFi.localIP().toString();
        Serial.println(wifiStatus);
        setupOTA();
    } else {
        wifiStatus = "[WiFi] Connection failed!";
        Controller::fatalErrorAlarm(wifiStatus.c_str());
    }
    Controller::status(wifiStatus + ", " + DNS);
    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        if (line.startsWith("!")) {
            Serial.println("!!!!! forced reinitialization of the model in setup");
            Controller::model.initialize();
        }
        if (line.startsWith("@")) {
            Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!bailing out after getting WIFi connection to avoid reboots");
            return;  // last resort so we can still use OTA
        }
    }
    spiffInit();
    bool res = Controller::model.load();  // check for emergency reinitialize if we messed up the model
    if (!res) {
        Controller::fatalErrorAlarm("[FS] Could not load the model so we initialized from code.");
    }
    // Initialize and get the time from NTP server
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    const char* bootTime = TimeManager::getBootTimeAsString();
    Controller::set("bootTime", bootTime);
    Serial.println("Controller::model object created and content is:" + Controller::model.toBriefJsonString());
    Serial.println(Controller::Controller::webSocket.url());
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/html", HtmlHelper::generateStatusPage(true)); });
    server.on("/extended", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/html  charset=utf-8", HtmlHelper::generateStatusPage(false)); });
    server.on("/metadata", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/html", HtmlHelper::generateMetadataPage()); });
    server.on("/advanced", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/html", HtmlHelper::generateAdvancedPage()); });
    server.on("/chart", HTTP_GET, [](AsyncWebServerRequest* r) { r->send(200, "text/html", HtmlHelper::generateChartPage()); });
    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest* r) {r->send(200,"text/plain","Rebooting...");delay(100);ESP.restart(); });

    Controller::webSocket.onEvent([](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
        if (type == WS_EVT_DATA) {
            String msg;
            for (size_t i = 0; i < len; i++)
                msg += (char)data[i];
            handleWebSocketMessage(msg);
        }
    });
    server.addHandler(&Controller::webSocket);
    server.begin();
    BackEnd::setupBackend();
    xTaskCreate(loopBackendTask, "LoopBackend", 8192, nullptr, 1, nullptr);
    Serial.println("[SYS] Main Setup complete.");
}
//
void loopBackendTask(void* param) {
    for (;;) {
        // Place the original code from loopBackend here
        BackEnd::loopBackend();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
//
bool first = true;
void serialLoop() {
    if (first) {
        Serial.println("serial loop started");
        Serial.println("in loop brief Controller::model via Controller is:" + Controller::model.toBriefJsonString());
        first = false;
    }
    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line == "m")
            Controller::model.listSerial();
        else if (line == "j") {
            Serial.println("memory model is:");
            Serial.println(Controller::model.toJsonString());
        } else if (line.startsWith("upload ")) {
            String jsonStr = line.substring(line.indexOf(' ') + 1);
            jsonStr.trim();
            if (JsonWrapper::checkJson(jsonStr)) {
                Controller::model.loadFromJson(jsonStr);
                Serial.println("Json replaced but not persisted !!!!!!!!!!!");
            } else {
                Serial.println("New entered Json does not parse so Model remained unchanged");
            }
            Controller::webSocket.textAll(Controller::model.toJsonString());
        } else if (line.startsWith("add ")) {
            Field f;
            f.setReadOnly(false);
            int idx;
            idx = line.indexOf("name=");
            if (idx >= 0) {
                int e = line.indexOf(" ", idx);
                if (e < 0) e = line.length();
                f.setName(line.substring(idx + 5, e));
            }
            idx = line.indexOf("id=");
            if (idx >= 0) {
                int e = line.indexOf(" ", idx);
                if (e < 0) e = line.length();
                f.setId(line.substring(idx + 3, e));
            } else {
                Serial.println("TODO should default the id if not present int the field");
            }
            idx = line.indexOf("type=");
            if (idx >= 0) {
                int e = line.indexOf(" ", idx);
                if (e < 0) e = line.length();
                f.setType(line.substring(idx + 5, e));
            }
            idx = line.indexOf("value=");
            if (idx >= 0) {
                int e = line.indexOf(" ", idx);
                if (e < 0) e = line.length();
                f.setValue(line.substring(idx + 6, e));
            }
            idx = line.indexOf("description=");
            if (idx >= 0) {
                int e = line.indexOf(" ", idx);
                if (e < 0) e = line.length();
                f.setDescription(line.substring(idx + 12, e));
            }
            idx = line.indexOf("readonly=");
            if (idx >= 0) {
                f.setReadOnly((line.substring(idx + 9, idx + 10) == "1"));
            }
            idx = line.indexOf("isshown=");
            if (idx >= 0) {
                f.setIsShown((line.substring(idx + 9, idx + 10) == "1"));
            }
            idx = line.indexOf("ispersisted=");
            if (idx >= 0) {
                f.setIsPersisted((line.substring(idx + 9, idx + 10) == "1"));
            }
            if (f.getName() != "") {
                Controller::model.add(f);
                Serial.printf("[SERIAL] Added field %s\n", f.getName().c_str());
                Controller::model.saveToFile();
                Controller::webSocket.textAll(Controller::model.toJsonString());
            }
        } else if (line.startsWith("delete ")) {
            String name = line.substring(7);
            Field* f = Controller::model.getByName(name);
            if (f) {
                Controller::model.remove(f->getId());
                Serial.printf("[SERIAL] Deleted field  %s\n", name.c_str());
                Controller::model.saveToFile();
                Controller::webSocket.textAll(Controller::model.toJsonString());
            }
        } else if (line == "r") {
            Controller::model.initialize();
            Serial.printf("[SERIAL] Reinitialized the whole Controller::model");
        } else if (line.startsWith("restart")) {
            Serial.println("[SERIAL] restart ESP32...");
            delay(200);
            ESP.restart();
        } else if (line == "?") {
            Serial.println("\n--- Available Serial Commands ---");
            Serial.println("? or help          : Display this help message");
            Serial.println("m                  : List current Controller::model fields details");
            Serial.println("j                  : Print full Controller::model as JSON string");
            Serial.println("upload <json_str>  : Replace entire Controller::model with new JSON data");
            Serial.println("add name=... id=...: Add a new field (supply all params)");
            Serial.println("delete <name>      : Delete a field by name");
            Serial.println("<name>=<value>     : Update the value of an existing field");
            Serial.println("r                  : reset reinitialize fields with factory setting");
            Serial.println("restart            : Restart the ESP32");
            Serial.println("-----------------------------------\n");
        } else {  // we assume this is like FieldName=newValue
            int eq = line.indexOf('=');
            if (eq > 0) {
                String name = line.substring(0, eq);
                String val = line.substring(eq + 1);
                Field* f = Controller::model.getByName(name);
                if (f) {
                    f->setValue(val);
                    if (f->getIsPersisted()) Controller::model.saveToFile();
                    Serial.printf("[SERIAL] Updated %s = %s\n", f->getName().c_str(), f->getValue().c_str());
                    Controller::webSocket.textAll(Controller::model.toJsonString());
                } else {
                    Serial.printf("[SERIAL] !!! cannot find any field with this name");
                }
            }
        }
    }
}
//
void loop() {
    ArduinoOTA.handle();
    serialLoop();
}
// --- OTA Setup Function ---
void setupOTA() {
    // Use the same mDNS hostname
    const char* hostname = DNS;
    ArduinoOTA.setHostname(hostname);
    Serial.printf("ArduinoOTA.setHostname done, %s %s:", hostname, ".local");
    // Optional: Set a password for security  // ArduinoOTA.setPassword("your_ota_password");
    // Configure callbacks for OTA events
    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else {  // U_SPIFFS
            type = "filesystem";
        }
        Serial.println("Start updating " + type);
        // You may want to stop web services or motors here during an update
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd OTA update.");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("OTA Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
            Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
            Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
            Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
            Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)
            Serial.println("End Failed");
    });
    ArduinoOTA.begin();
    Serial.println("OTA service initialized.");
}
// --------------------------