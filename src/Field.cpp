#include "Field.h"

Field::Field(const String& _id, const String& _name, const String& _type,
             const String& _value, const String& _description, bool _readOnly)
    : id(_id), name(_name), type(_type), value(_value),
      description(_description), readOnly(_readOnly) {}

Field::Field() : id(""), name(""), type("string"), value(""), description(""), readOnly(false) {}

void Field::fromJson(const JsonVariantConst& obj) {
    if (!obj.is<JsonObjectConst>()) return;
    id = obj["id"] | "";
    name = obj["name"] | "";
    type = obj["type"] | "string";
    value = obj["value"] | "";
    description = obj["description"] | "";
    readOnly = obj["readOnly"] | false;
}

void Field::toJson(JsonVariant obj) const {
    obj["id"] = id;
    obj["name"] = name;
    obj["type"] = type;
    obj["value"] = value;
    obj["description"] = description;
    obj["readOnly"] = readOnly;
}

bool Field::isValid() const {
    return id.length() > 0 && name.length() > 0;
}

bool Field::updateValue(const String& newValue) {
    if (readOnly) return false;
    if (validateValue(newValue)) {
        value = newValue;
        return true;
    }
    return false;
}

bool Field::validateValue(const String& newValue) const {
    if (type == "int") {
        for (uint16_t i = 0; i < newValue.length(); i++)
            if (!isDigit(newValue[i]) && !(i == 0 && newValue[i] == '-')) return false;
        return true;
    }
    else if (type == "float") {
        bool dotFound = false;
        for (uint16_t i = 0; i < newValue.length(); i++) {
            char c = newValue[i];
            if (c == '.') {
                if (dotFound) return false;
                dotFound = true;
            } else if (!isDigit(c) && !(i == 0 && c == '-')) return false;
        }
        return true;
    }
    else if (type == "bool") {
        String lower = newValue; lower.toLowerCase();
        return (lower == "true" || lower == "false" || lower == "1" || lower == "0");
    }
    // accept all strings
    return true;
}
