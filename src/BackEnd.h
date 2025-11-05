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
    static void setupBackend() {
        Serial.println("[SYS] BackEnd Setup complete.");
    }
};
// static void loopBackend() {
//     Serial.println("[SYS] loopBackend Started.");
//     while (true) {
//         if Controller::model.
//         durationSinceRebootInSeconds = millis() / 1000;
//         static unsigned long lastModelUpdateInSeconds = 0;
//         if (durationSinceRebootInSeconds - lastModelUpdateInSeconds >= 5) {
//             lastModelUpdateInSeconds = durationSinceRebootInSeconds;
//             Field* f = model.getByName("duration");
//             if (!f) {
//                 Field nf("10", "duration", "string", String(durationSinceRebootInSeconds), "time since start", false);
//                 model.add(nf);
//             } else
//                 f->setValue(String(durationSinceRebootInSeconds));
//             model.saveToFile();
//             webSocket.textAll(model.toJson());
//         }
//         Field* f = model.getByName("delay");
//         String delayAsString;
//         if (!f) {
//             Field nf("11", "delay", "string", "7", "blocking delay in backend", false);
//             model.add(nf);
//             model.saveToFile();
//             webSocket.textAll(model.toJson());
//             delayAsString = nf.getValue();
//         } else {
//             delayAsString = f->getValue();
//         }
//         int delayAsInt = delayAsString.toInt() * 1000;
//         Serial.println("[BACKEND] Blocking delay " + delayAsString + "s");
//         webSocket.textAll(model.toJson());
//         delay(delayAsInt);
//     }
// }
