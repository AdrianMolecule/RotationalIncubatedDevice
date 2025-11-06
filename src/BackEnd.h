#pragma once

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <FS.h>
#include <SPIFFS.h>

#include <vector>

#include "Controller.h"
#include "Field.h"
#include "Helper.h"
#include "JsonWrapper.h"

class BackEnd {
   public:
    static inline unsigned long lastModelUpdateInSeconds = 0;
    static void setupBackend() {
        Serial.println("[SYS] BackEnd Setup complete.");
        
    }

    static void loopBackend() {
        Serial.println("[SYS] loopBackend Started.");
        ledcDetachPin(33); 
        unsigned long durationSinceRebootInSeconds = millis() / 1000;
        while (true) {
            // if Controller::model.
            durationSinceRebootInSeconds = millis() / 1000;
            if (durationSinceRebootInSeconds - lastModelUpdateInSeconds >= 5) {
                lastModelUpdateInSeconds = durationSinceRebootInSeconds;
                Field* f = Controller::model.getByName("duration");
                if (!f) {
                    Field nf("10", "duration", "string", String(durationSinceRebootInSeconds), "time since start", false);
                    Controller::model.add(nf);
                } else
                    f->setValue(String(durationSinceRebootInSeconds));
                Controller::model.saveToFile();
                Controller::webSocket.textAll(Controller::model.toJsonString());
            }
            Field* f = Controller::model.getByName("delay");
            String delayAsString;
            if (!f) {
                Field nf("11", "delay", "string", "7", "blocking delay in backend", false);
                Controller::model.add(nf);
                Controller::model.saveToFile();
                Controller::webSocket.textAll(Controller::model.toJsonString());
                delayAsString = nf.getValue();
            } else {
                delayAsString = f->getValue();
            }
            int delayAsInt = delayAsString.toInt() * 1000;
            Serial.println("[BACKEND] Blocking delay " + delayAsString + "s");
            Controller::webSocket.textAll(Controller::model.toJsonString());
            delay(delayAsInt);
        }
    }
};