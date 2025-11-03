// #include "BackEnd.h"

// #include "Controller.h"

// void BackEnd::setupBackend() {
//     Serial.println("[Backend] Initialized");
// }

// void BackEnd::loopBackend() {
//     Serial.printf(" BackEnd::loopBackend");
//     return;
//     Model& model = Controller::getModel();
//     // Example periodic task
//     static unsigned long last = 0;
//     if (millis() - last > 5000) {
//         last = millis();
//         Field* temp = model.findField("Temperature");
//         if (temp) {
//             float val = temp->getValue().toFloat();
//             val += 0.1f;
//             temp->setValue(String(val, 1));
//             model.save("/model.json");
//             Serial.printf("[Backend] Updated Temperature: %.1f\n", val);
//         }
//     }
// }
