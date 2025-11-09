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
    bool isShown;

   public:
    Field() = default;
    Field(const String& id, const String& name, const String& type = "string",
          const String& value = "0", const String& description = "default description",
          bool readOnly = false, bool isShown = true) {
        this->id = id;
        this->name = name;
        this->type = type;
        this->value = value;
        this->description = description;
        this->readOnly = readOnly;
        this->isShown = isShown;
    }

    void Field::fromJson(const JsonObject& obj) {
        id = obj["id"] | "";
        name = obj["name"] | "";
        type = obj["type"] | "";
        value = obj["value"] | "";
        description = obj["description"] | "";
        readOnly = obj["readOnly"] | false;
        readOnly = obj["isShown"] | false;
    }

    void toJson(JsonObject& obj) const {
        obj["id"] = id;
        obj["name"] = name;
        obj["type"] = type;
        obj["value"] = value;
        obj["description"] = description;
        obj["readOnly"] = readOnly;
        obj["isShown"] = isShown;
    }

    const String getId() const { return id; }
    const String getName() const { return name; }
    const String getType() const { return type; }
    const String getValue() const { return value; }
    bool getReadOnly() const { return readOnly; }
    const String getDescription() const { return description; }
    bool getIsShown() const { return isShown; }

    void setId(const String& val) {
        Serial.printf("[Field] setId: %s -> %s\n", id.c_str(), val.c_str());
        id = val;
    }
    void setName(const String& val) {
        Serial.printf("[Field] setName: %s -> %s\n", name.c_str(), val.c_str());
        name = val;
    }

    void setType(const String& val) {
        Serial.printf("[Field] setType: %s -> %s\n", type.c_str(), val.c_str());
        type = val;
    }
    // Setters with logging
    void setValue(const String& val) {
        Serial.printf("[Field] %s setValue: %s -> %s\n", name.c_str(), value.c_str(), val.c_str());
        value = val;
    }
    void setDescription(const String& val) {
        Serial.printf("[Field] setDescription: %s -> %s\n", description.c_str(), val.c_str());
        description = val;
    }
    void setReadOnly(bool val) {
        Serial.printf("[Field] setReadOnly: %d -> %d\n", readOnly, val);
        readOnly = val;
    }

    void setIsShown(bool val) {
        Serial.printf("[Field] %s setIsShown: %d -> %d\n", name.c_str(), isShown, val);
        isShown = val;
    }
};
