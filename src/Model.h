#pragma once
#include "Field.h"
#include "Helper.h"
#include "JsonWrapper.h"

class Model {
   private:
    std::vector<Field> fields;

   public:
    std::vector<Field>& getFields() {
        return fields;
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
    }

    void initialize() {
        Serial.println("[MODEL] Initialize model by loading factory model");
        Helper::initialize(fields);
    }

    void initializeSample() {
        Serial.println("[MODEL] Initialize just SAMPLE model by loading factory model");
        Helper::initializeSample(fields);
    }

    bool load() {
        JsonWrapper::loadFieldsFromFile(fields);
        if (fields.empty()) {
            initialize();
            return false;
        }
        return true;
    }

    bool saveToFile() {
        File file = SPIFFS.open("/model.json", "w");
        if (!file) {
            Serial.println("error in model saveToFile file");
            return false;
        }
        return JsonWrapper::saveModelToFile(fields);
    }

    String toJsonString() {
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
};  // end Model
