// #include "Controller.h"
// #include "SerialView.h"


// void Controller::begin() {
//     // Try loading model
//     bool loaded = JsonWrapper::loadModelFromFile("/model.json", model.getFields());
//     if (!loaded || model.getFields().empty()) {
//         Serial.println("[Controller] No model file found, creating defaults...");
//         model.loadDefaults();
//         model.save("/model.json");
//     } else {
//         Serial.printf("[Controller] Loaded %d fields from SPIFFS\n", model.getFields().size(), JsonWrapper::fieldsToJsonString(model.getFields()));
//     }

//     WebView::begin();
//     SerialView::begin();
//     BackEnd::setupBackend();
//     applyHooks();
// }
// Model& Controller::getModel() {
//     return model;
// }

// void Controller::applyHooks() {
//     // Example LED hook
//     Field* led = model.findField("LED");
//     if (led) {
//         pinMode(2, OUTPUT);
//         digitalWrite(2, led->getValue() == "true" ? HIGH : LOW);
//     }
// }

// void Controller::broadcastFullModel() {
//     String json =JsonWrapper::fieldsToJsonString( model.getFields());
//     s_serial.broadcast(json);
//     s_web.broadcast(json);
// }

// void Controller::loopBackend() {
//     BackEnd::loopBackend();
// }
// void Controller::handleWebSocketMessage(const String& msg) {
//     Serial.printf("[Controller] WS message received: %s\n", msg.c_str());
//     // TODO: parse JSON or key=value updates later
// }