#pragma once
#include <ESPAsyncWebServer.h>

#include "Field.h"
#include "Model.h"

class Controller {
   public:
    inline static Model model = Model{};

    inline static AsyncWebSocket webSocket = AsyncWebSocket{"/ws"};  // Declare the variable here (doesn't allocate memory yet)

    static int getI(String name) {
        // Serial.println("Controller::getI called for the name "+name);
        const auto f = Controller::model.getByName(name);
        if (f == nullptr) {
            Controller::error("!!!!!Controller::getI did not find an entry for the name[" + name + "]");
            return -1;
        }
        return f->getValue().toInt();
    }
    //
    static bool getPresent(String name) {
        const auto f = Controller::model.getByName(name);
        if (f == nullptr) {
            Serial.println("Controller::getPresent called for the name " + name);
            Controller::error("!!!!!Controller::getPresent did not find an entry for the name[" + name + "]");
            return false;
        }
        // Serial.println("!!!!!Controller::getPresent found value:" + model.getByName(name)->getValue());
        else
            return !(f->getValue() == NOT_PRESENT);
    }
    //
    static bool getBool(String name) {
        const auto f = Controller::model.getByName(name);
        if (f == nullptr) {
            Controller::error("!!!!!!!!!!!!!!!!!!Controller::getBool did not find an entry for the name[" + name + "]");
            return false;
        } else
            return f->getValue().compareTo("0");
    }
    //
    static String getS(String name) {
        const auto f = Controller::model.getByName(name);
        if (f == nullptr) {
            Controller::error("!!!!!Controller::getS did not find an entry for the name[" + name + "]");
            return "NOT FOUND";
        } else
            return f->getValue();
    }
    //
    static void set(String name, String value) {
        Serial.println("Controller::setI called for the name " + name + " with value:" + value);
        const auto f = Controller::model.getByName(name);
        if (f == nullptr) {
            Controller::error("!!!!!Controller::set did not find an entry for the name[" + name + "]");
        } else
            f->setValue(value);
    }
    //
    static void setNoLog(String name, String value) {
        const auto f = Controller::model.getByName(name);
        if (f == nullptr) {
            Controller::error("!!!!!Controller::set did not find an entry for the name[" + name + "]");
        } else
            f->setValueQuiet(value);
    }
    //
    static void setBool(String name, bool value) {
        const auto f = Controller::model.getByName(name);
        if (f == nullptr) {
            Controller::error("!!!!!Controller::setBool did not find an entry for the name[" + name + "]");
        } else
            f->setValue(value ? "1" : "0");
    }
    //
    static void status(String msg) {
        Controller::set("status", Controller::getS("status") + "; " + msg);
    }
    //
    static void error(String msg) {
        Serial.println("Error" + String(msg));
        const auto f = Controller::model.getByName("error");
        if (f == nullptr) {
            Serial.println("we cannot set the controller error because field \"error\" does not exist in the current model");
        } else {
            if (Controller::getS("error").length() < 400) {
                Controller::set("error", Controller::getS("error") + "; " + msg);
            } else if (Controller::getS("error").length() < 450) {
                Controller::set("error", Controller::getS("error") + "; ...");
            }
        }
    }
    //
    static void warning(String msg) {
        Serial.println("warning" + String(msg));
        const auto f = Controller::model.getByName("warning");
        if (f == nullptr) {
            Serial.println("we cannot set the controller warning because field \"warning\" does not exist in the current model");
        } else {
            if (Controller::getS("warning").length() < 400) {
                Controller::set("warning", Controller::getS("warning") + "; " + msg);
            } else if (Controller::getS("warning").length() < 450) {
                Controller::set("warning", Controller::getS("warning") + "; ...");
            }
        }
    }
};