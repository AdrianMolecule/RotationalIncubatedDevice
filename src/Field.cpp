#include <ArduinoJson.h>
#include "Field.h"

void Field::fromJson(const JsonObject& obj) {
    id = obj["id"] | "";
    name = obj["name"] | "";
    type = obj["type"] | "";
    value = obj["value"] | "";
    description = obj["description"] | "";
    readOnly = obj["readOnly"] | false;
}

void Field::toJson(JsonObject& obj) const {
    obj["id"] = id;
    obj["name"] = name;
    obj["type"] = type;
    obj["value"] = value;
    obj["description"] = description;
    obj["readOnly"] = readOnly;
}