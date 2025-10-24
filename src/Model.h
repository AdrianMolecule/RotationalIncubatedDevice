#pragma once
#include <Arduino.h>
#include <vector>
#include "Field.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

// Model stores multiple fields and can save/load to JSON in SPIFFS
class Model {
public:
    std::vector<Field> fields;

    Model() {}

    void addField(const Field &field) {
        fields.push_back(field);
    }

    Field* getFieldByName(const String &name) {
        for (auto &f : fields) {
            if (f.name == name) return &f;
        }
        return nullptr;
    }

    void setValue(const String &name, const String &value) {
        Field* f = getFieldByName(name);
        if (f) f->setValue(value);
    }

    String getValue(const String &name) {
        Field* f = getFieldByName(name);
        return f ? f->getValue() : "";
    }

    // Persist model to SPIFFS as JSON
    void saveToSPIFFS(const String &path = "/model.json") {
        StaticJsonDocument<1024> doc;
        for (auto &f : fields) {
            doc[f.name] = f.getValue();
        }

        File file = SPIFFS.open(path, FILE_WRITE);
        if (!file) {
            Serial.println("Failed to open file for writing");
            return;
        }
        serializeJson(doc, file);
        file.close();
    }

    // Load model from SPIFFS JSON
    void loadFromSPIFFS(const String &path = "/model.json") {
        if (!SPIFFS.exists(path)) return;

        File file = SPIFFS.open(path, FILE_READ);
        if (!file) {
            Serial.println("Failed to open file for reading");
            return;
        }

        StaticJsonDocument<1024> doc;
        DeserializationError error = deserializeJson(doc, file);
        file.close();

        if (error) {
            Serial.println("Failed to parse JSON");
            return;
        }

        for (auto &f : fields) {
            if (doc.containsKey(f.name)) {
                f.setValue(doc[f.name].as<String>());
            }
        }
    }
};
