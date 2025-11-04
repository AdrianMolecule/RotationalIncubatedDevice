#pragma once
#include <Arduino.h>

class Field {
   private:
    String id;
    String name;
    String type;
    String value;
    bool readOnly;
    String description;

   public:
    Field() = default;
    Field(const String& id, const String& name, const String& type = "string",
          const String& value = "0", const String& description = "default description", bool readOnly = false) {
        this->id = id;
        this->name = name;
        this->type = type;
        this->value = value;
        this->description = description;
        this->readOnly = readOnly;
    };
    void fromJson(const JsonObject& obj);
    void toJson(JsonObject& obj) const;
    // Getters
    const String getId() const {
        return id;
    }
    const String getName() const {
        return name;
    }
    const String getType() const { return type; }
    const String getValue() const { return value; }
    bool getReadOnly() const { return readOnly; }
    const String getDescription() const { return description; }

    // Setters with logging
    void setValue(const String& val) {
        Serial.printf("[Field] setValue: %s -> %s\n", value.c_str(), val.c_str());
        value = val;
    }

    void setName(const String& val) {
        Serial.printf("[Field] setName: %s -> %s\n", name.c_str(), val.c_str());
        name = val;
    }

    void setId(const String& val) {
        Serial.printf("[Field] setId: %s -> %s\n", id.c_str(), val.c_str());
        id = val;
    }

    void setType(const String& val) {
        Serial.printf("[Field] setType: %s -> %s\n", type.c_str(), val.c_str());
        type = val;
    }

    void setReadOnly(bool val) {
        Serial.printf("[Field] setReadOnly: %d -> %d\n", readOnly, val);
        readOnly = val;
    }

    void setDescription(const String& val) {
        Serial.printf("[Field] setDescription: %s -> %s\n", description.c_str(), val.c_str());
        description = val;
    }
};
