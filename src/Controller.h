#pragma once
#include <ESPAsyncWebServer.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>

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
            Controller::fatalErrorAlarm(("!!!!!Controller::getI did not find an entry for the name[" + name + "]").c_str());
            return -1;
        }
        return f->getValue().toInt();
    }
    //
    static bool getPresent(String name) {
        const auto f = Controller::model.getByName(name);
        if (f == nullptr) {
            Serial.println("Controller::getPresent called for the name " + name);
            Controller::fatalErrorAlarm(("!!!!!Controller::getPresent did not find an entry for the name[" + name + "]").c_str());
            return false;
        }
        // Serial.println("!!!!!Controller::getPresent found value:" + model.getByName(name)->getValue());
        else
            return !(f->getValue() == Config::NOT_PRESENT);
    }
    //
    static bool getBool(String name) {
        const auto f = Controller::model.getByName(name);
        if (f == nullptr) {
            Controller::fatalErrorAlarm(("!!!!!!!!!!!!!!!!!!Controller::getBool did not find an entry for the name[" + name + "]").c_str());
            return false;
        } else
            return f->getValue().compareTo("0");
    }
    //
    static String getS(String name) {
        const auto f = Controller::model.getByName(name);
        if (f == nullptr) {
            Controller::fatalErrorAlarm(("!!!!!Controller::getS did not find an entry for the name[" + name + "]").c_str());
            return "NOT FOUND";
        } else
            return f->getValue();
    }
    //
    static void set(String name, String value) {
        Serial.println("Controller::set called for the name " + name + " with value:" + value);
        const auto f = Controller::model.getByName(name);
        if (f == nullptr) {
            Controller::fatalErrorAlarm(("!!!!!Controller::set did not find an entry for the name[" + name + "]").c_str());
        } else
            f->setValue(value);
    }
    //
    static void setQuiet(String name, String value) {
        const auto f = Controller::model.getByName(name);
        if (f == nullptr) {
            Controller::fatalErrorAlarm(("!!!!!Controller::set did not find an entry for the name[" + name + "]").c_str());
        } else {
            Field::logSets = false;
            f->setValue(value);
            Field::logSets = true;
        }
    }
    //
    static void setBool(String name, bool value) {
        Serial.println("Controller::set called for the name " + name + " with value:" + value);
        const auto f = Controller::model.getByName(name);
        if (f == nullptr) {
            Controller::fatalErrorAlarm(("!!!!!Controller::setBool did not find an entry for the name[" + name + "]").c_str());
        } else
            f->setValue(value ? "1" : "0");
    }
    //
    static void fatalErrorAlarm(const char* message) {
        MyMusic::play(MyMusic::fatalErrorAlarmMusic);
        Controller::error(message);
    }
    //
    static void errorAlarm(const char* message) {
        Controller::error(message);
        MyMusic::play(MyMusic::errorAlarmMusic);
    }
    //
    static void warningAlarm(const char* message) {
        Controller::warning(message);
        MyMusic::play(MyMusic::warningAlarmMusic);
    }
    //
    static void infoAlarm(const char* message, Melody m = MyMusic::infoAlarmMusic) {
        MyMusic::play(m);
        Controller::log("Info Alarm: %s", message);
    }
    //
    static void eraseInfo() {
        Controller::set("info", "");
    }
    //
    static void log(const char* format, ...) {  // Changed from const String& msg to const char* format
        const size_t BUFFER_SIZE = 200;
        char logMessageBuffer[BUFFER_SIZE];
        // Start variadic arguments handling
        va_list args;
        va_start(args, format);
        // Safely format the arguments into the buffer
        vsnprintf(logMessageBuffer, BUFFER_SIZE, format, args);
        // End variadic arguments handling
        va_end(args);
        // Now proceed with your JSON serialization using the buffer
        JsonDocument doc;
        doc["action"] = "log";
        doc["msg"] = logMessageBuffer;  // Use the C-style string buffer for the message
        String out;
        serializeJson(doc, out);
        Controller::webSocket.textAll(out);
        Serial.println(logMessageBuffer);
    }
    //
   private:
    static void error(const char* msg) {
        Controller::log("Error: %s", msg);
        const auto f = Controller::model.getByName("error");
        if (f == nullptr) {
            Serial.println("!!!! We cannot set the controller error because field \"error\" does not exist in the current model");
        } else {
            if (Controller::getS("error").length() < 400) {
                Controller::set("error", Controller::getS("error") + "; " + msg);
            } else if (Controller::getS("error").length() < 450) {
                Controller::set("error", Controller::getS("error") + "; ...");
            }
        }
    }
    //
    static void warning(const char* msg) {
        Controller::log("Warning: %s", msg);
        const auto f = Controller::model.getByName("warning");
        if (f == nullptr) {
            Serial.println("!!!!! We cannot set the controller warning because field \"warning\" does not exist in the current model");
        } else {
            if (Controller::getS("warning").length() < 400) {
                Controller::set("warning", Controller::getS("warning") + "; " + msg);
            } else if (Controller::getS("warning").length() < 450) {
                Controller::set("warning", Controller::getS("warning") + "; ...");
            }
        }
    }
    //
    static void info(const char* msg) {
        Serial.printf("info: %s", msg);
        const auto f = Controller::model.getByName("info");
        if (f == nullptr) {
            Serial.println("!!!!!! We cannot set the controller info because field \"info\" does not exist in the current model");
        } else {
            Controller::set("info", msg);
        }
    }
};