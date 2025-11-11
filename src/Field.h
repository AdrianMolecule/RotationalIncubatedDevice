#pragma once
#include <Arduino.h>

class Field {
   private:
    String id;//todo change to int or some smaller type
    String name;
    String type;
    String value;
    String description;
    bool readOnly;
    bool isShown;
    bool isPersisted;

   public:
    Field() = default;
    Field(const String& id, const String& name, const String& type = "string",
          const String& value = "0", const String& description = "default description",
          bool readOnly = false, bool isShown = false, bool isPersisted = true) {
        this->id = id;
        this->name = name;
        this->type = type;
        this->value = value;
        this->description = description;
        this->readOnly = readOnly;
        this->isShown = isShown;
        this->isPersisted = isPersisted;
    }

    void fromJson(const JsonObject& obj) {
        id = obj["id"] | "";
        name = obj["name"] | "";
        type = obj["type"] | "";
        value = obj["value"] | "";
        description = obj["description"] | "";
        readOnly = obj["readOnly"] | false;
        isShown = obj["isShown"] | false;
        isPersisted = obj["isPersisted"] | true;
    }

    void toJson(JsonObject& obj) const {
        obj["id"] = id;
        obj["name"] = name;
        obj["type"] = type;
        obj["value"] = value;
        obj["description"] = description;
        obj["readOnly"] = readOnly;
        obj["isShown"] = isShown;
        obj["isPersisted"] = isPersisted;
    }

    const String getId() const { return id; }
    const String getName() const { return name; }
    const String getType() const { return type; }
    const String getValue() const { return value; }
    bool getReadOnly() const { return readOnly; }
    const String getDescription() const { return description; }
    bool getIsShown() const { return isShown; }
    bool getIsPersisted() const { return isPersisted; }

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
    void setValueQuiet(const String& val) {
        value = val;
    }
    void setDescription(const String& val) {
        Serial.printf("[Field] %s setDescription: %s -> %s\n", name.c_str(), description.c_str(), val.c_str());
        description = val;
    }
    void setReadOnly(bool val) {
        Serial.printf("[Field] %s setReadOnly: %d -> %d\n", name.c_str(), readOnly, val);
        readOnly = val;
    }
    void setIsShown(bool val) {
        Serial.printf("[Field] %s setIsShown: %d -> %d\n", name.c_str(), isShown, val);
        isShown = val;
    }
    void setIsPersisted(bool val) {
        Serial.printf("[Field] %s setIsPersisted: %d -> %d\n", name.c_str(), isPersisted, val);
        isPersisted = val;
    }
};
