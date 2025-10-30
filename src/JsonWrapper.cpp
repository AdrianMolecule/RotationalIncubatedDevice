#include "JsonWrapper.h"
#include <SPIFFS.h>

bool JsonWrapper::saveToFile(const char* path, const std::vector<Field>& fields) {
    File file = SPIFFS.open(path, FILE_WRITE);
    if (!file) return false;

    JsonDocument doc;
    JsonArray arr = doc["fields"].to<JsonArray>();
    for (const auto& f : fields) {
        JsonObject o = arr.add<JsonObject>();
        f.toJson(o);
    }

    if (serializeJson(doc, file) == 0) {
        file.close();
        return false;
    }
    file.close();
    return true;
}

bool JsonWrapper::loadFromFile(const char* path, std::vector<Field>& fields) {
    File file = SPIFFS.open(path, FILE_READ);
    if (!file) return false;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, file);
    file.close();
    if (err) return false;

    fields.clear();
    JsonArrayConst arr = doc["fields"].as<JsonArrayConst>();
    for (JsonObjectConst o : arr) {
        Field f;
        f.fromJson(o);
        if (f.isValid()) fields.push_back(f);
    }
    return true;
}

String JsonWrapper::toString(const std::vector<Field>& fields) {
    JsonDocument doc;
    JsonArray arr = doc["fields"].to<JsonArray>();
    for (const auto& f : fields) {
        JsonObject o = arr.add<JsonObject>();
        f.toJson(o);
    }
    String out;
    serializeJson(doc, out);
    return out;
}

bool JsonWrapper::fromString(const String& json, std::vector<Field>& fields) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) return false;

    fields.clear();
    JsonArrayConst arr = doc["fields"].as<JsonArrayConst>();
    for (JsonObjectConst o : arr) {
        Field f;
        f.fromJson(o);
        if (f.isValid()) fields.push_back(f);
    }
    return true;
}
