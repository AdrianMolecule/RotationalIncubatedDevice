#pragma once
#include <ESPAsyncWebServer.h>

#include "Field.h"
#include "Model.h"

class Controller {
   public:
    inline static Model model = Model{};
    
    inline static AsyncWebSocket webSocket = AsyncWebSocket{"/ws"};
    // ^ Declare the variable here (doesn't allocate memory yet)

    // void incrementCounter() {
    //     global_counter++;  // Access it directly within member functions
    // }
};