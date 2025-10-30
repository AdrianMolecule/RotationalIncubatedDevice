#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

class Field {
public:
    String id;
    String name;
    String type;
    String value;
    String description;
    bool readOnly;

    Field();
    Field(const String& _id, const String& _name, const String& _type,
          const String& _value, const String& _description, bool _readOnly);

    // Load field data from JSON
    void fromJson(const JsonVariantConst& obj);

    // Serialize field to JSON
    void toJson(JsonVariant obj) const;

    // Basic validity check (id + name must exist)
    bool isValid() const;

    // Update the value safely (obey readOnly)
    bool updateValue(const String& newValue);

    // Validate type-specific format
    bool validateValue(const String& newValue) const;
};
