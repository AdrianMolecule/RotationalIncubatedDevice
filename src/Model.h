#pragma once
#include <SPIFFS.h>

#include "Field.h"
#include "Config.h"
#include "JsonWrapper.h"
#include "MyMusic.h"

class Model {
   private:
    std::vector<Field> fields;

   public:
    std::vector<Field>& getFields() {
        return fields;
    }
    std::vector<Field> getScreenFields() {
        std::vector<Field> shownFields;
        for (const Field& field : fields) {
            if (field.getIsShown()) {
                shownFields.push_back(field);
            }
        }
        return shownFields;
    }
    Field* getById(const String& id) {
        for (auto& f : fields)
            if (f.getId() == id) return &f;
        return nullptr;
    }
    Field* getByName(const String& name) {
        for (auto& f : fields)
            if (f.getName() == name) return &f;
        return nullptr;
    }
    void add(Field f) {
        fields.push_back(f);
        Serial.println("[Model] added field " + f.getName());
    }
    bool remove(const String& id) {
        for (size_t i = 0; i < fields.size(); i++) {
            if (fields[i].getId() == id) {
                fields.erase(fields.begin() + i);
                return true;
                Serial.println("{Model] removed field " + fields[i].getName());
            }
        }
        return false;
    }
    void reorder(const String& id, bool up) {
        for (size_t i = 0; i < fields.size(); i++) {
            if (fields[i].getId() == id) {
                if (up && i > 0)
                    std::swap(fields[i], fields[i - 1]);
                else if (!up && i < fields.size() - 1)
                    std::swap(fields[i], fields[i + 1]);
                break;
            }
        }
    }//
    void preSeed() {
        Serial.println("[MODEL] preseed harcoded fields");
        Config::initializeHardcodedFields(fields);
    }
    //
    void initialize() {
        Serial.println("[MODEL] Initialize model by loading factory hardcoded model");
        Config::initialize(fields);
    }
    //
    bool loadFromJson(const String& json) {
        return JsonWrapper::jsonToFields(json, fields);
    }
    //
    /** normal load returns true, initialize returns false */
    bool load() {
        // Attempt to load from file. If successful AND fields are present, return true.
        bool loadedFromFile = JsonWrapper::loadFieldsFromFile(fields);
        if (loadedFromFile && !fields.empty()) {
            return true;
        } else {           
            initialize();   // If load failed or file was empty, initialize factory model.
            return false;
        }
    }
    bool saveToFile() {
        File file = SPIFFS.open("/model.json", "w");
        if (!file) {
            Serial.println("error in model saveToFile file");
            return false;
        }
        return JsonWrapper::saveModelToFile(fields);
    }
    const char* toJsonString() {
        return JsonWrapper::toJsonString(fields);
    }
    String toBriefJsonString() {
        String result = "Brief for Fields size:" + String(fields.size());
        if (fields.size() >= 2) {
            result += ", " + fields.at(0).getName() + " ,";
            result += fields.at(fields.size() - 1).getName();
        }
        return result;
    }
    void listSerial() {
        for (auto& f : fields) Serial.printf("%s (%s) = %s\n", f.getName().c_str(), f.getType().c_str(), f.getValue().c_str());
    }
};