#pragma once
#include <ESPAsyncWebServer.h>

#include "Field.h"
#include "Model.h"

class Controller {
   public:
    inline static Model model = Model{};

    inline static AsyncWebSocket webSocket = AsyncWebSocket{"/ws"};  // Declare the variable here (doesn't allocate memory yet)
    static int getILogged(String name) {
        Serial.println("Controller::getI called for the name LOGGED LOGGED LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL " + name);
        if (Controller::model.getByName(name) == nullptr) {
            Serial.println("!!!!!Controller::getI did not find an entry for the name[" + name + "]");
            return -1;
        }
        return model.getByName(name)->getValue().toInt();
    }
    static int getI(String name) {
        //Serial.println("Controller::getI called for the name "+name);
        if(Controller::model.getByName(name)==nullptr){
            Serial.println("!!!!!Controller::getI did not find an entry for the name[" + name + "]");
            return -1;
        }
        return model.getByName(name)->getValue().toInt();
    }
    static bool getPresent(String name) {
        if(Controller::model.getByName(name)==nullptr){
            Serial.println("Controller::getPresent called for the name " + name);
            Serial.println("!!!!!Controller::getPresent did not find an entry for the name[" + name + "]");
            return false;
        }
        //Serial.println("!!!!!Controller::getPresent found value:" + model.getByName(name)->getValue());
        return !(model.getByName(name)->getValue()==NOT_PRESENT);
    }
    static bool getBool(String name) {
        //Serial.println("Controller::getBool called for the name " + name);
        if(Controller::model.getByName(name)==nullptr){
            Serial.println("!!!!!!!!!!!!!!!!!!Controller::getBool did not find an entry for the name[" + name+"]");
            return false;
        }
        return ! model.getByName(name)->getValue().compareTo("1");
    }
    static String getS(String name) {
        if (Controller::model.getByName(name) == nullptr) {
            Serial.println("!!!!!Controller::getS did not find an entry for the name[" + name + "]");
            return "NOT FOUND";
        }
        return model.getByName(name)->getValue();
    }
    static void set(String name, String value) {
        if (Controller::model.getByName(name) == nullptr) {
            Serial.println("!!!!!Controller::set did not find an entry for the name[" + name + "]");
        }
        model.getByName(name)->setValue(value);
    }
    static void setBool(String name, bool value) {
        if (Controller::model.getByName(name) == nullptr) {
            Serial.println("!!!!!Controller::setBool did not find an entry for the name[" + name + "]");
        }
        model.getByName(name)->setValue(value?"1":"0");
    }

};