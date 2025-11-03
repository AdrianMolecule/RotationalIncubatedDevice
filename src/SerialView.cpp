// #include "SerialView.h"

// void SerialView::begin() {
//     Serial.println("[Serial] Ready. Type ? or j");
// }
// void SerialView::broadcast(const String& json) {
//     Serial.println("[SERIAL] Broadcast: " + json);
// }
// void SerialView::handleSerial(Model& model) {
//     if (Serial.available()) {
//         String cmd = Serial.readStringUntil('\n');
//         cmd.trim();

//         if (cmd == "?") {
//             for (const auto& f : model.getFields())
//                 Serial.printf("%s = %s\n", f.getName().c_str(), f.getValue().c_str());
//         } else if (cmd == "j") {
//             String j= JsonWrapper::fieldsToJsonString( model.getFields());
//             Serial.println(j);
//         } else if (cmd.indexOf('=') > 0) {
//             int eq = cmd.indexOf('=');
//             String name = cmd.substring(0, eq);
//             String val = cmd.substring(eq + 1);
//             if (model.setValue(name, val)) {
//                 Serial.printf("[SET] %s = %s\n", name.c_str(), val.c_str());
//                 model.save("/model.json");
//             } else {
//                 Serial.printf("[ERR] Failed to set %s\n", name.c_str());
//             }
//         }
//     }
// }
