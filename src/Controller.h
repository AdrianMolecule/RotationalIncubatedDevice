// #pragma once
// #include "BackEnd.h"
// #include "JsonWrapper.h"
// #include "Model.h"
// #include "SerialView.h"
// #include "WebView.h"
// #include "Field.h"

// class Controller {
//    private:
//     static inline Model model=Model();

//     static inline WebView s_web;
//     static inline SerialView s_serial;

//    public:
//     static void begin();
//     static Model& getModel();

//     static void addField(const Field& f);
//     static void updateField(const Field& f);
//     static void deleteField(const String& id);

//     static void applyHooks();
//     static void broadcastFullModel();
//     static void loopBackend();
//     static void handleWebSocketMessage(const String& msg);
// };
