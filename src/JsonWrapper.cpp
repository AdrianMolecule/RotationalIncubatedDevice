#include "JsonWrapper.h"
#include <SPIFFS.h>

String JsonWrapper::fieldsToJson(const std::vector<Field>& fields) {
    JsonDocument doc(2048);  // sufficient size for fields
    JsonArray arr = doc.to<JsonArray>();
    for (const auto& f : fields) {
        JsonObject obj = arr.addEmptyObject();
        obj["id"] = f.id;
        obj["name"] = f.name;
        obj["type"] = f.type;
        obj["value"] = f.value;
        obj["description"] = f.description;
        obj["readOnly"] = f.readOnly;
    }
    String result;
    serializeJson(doc, result);
    return result;
}

String JsonWrapper::fieldToJson(const Field& f) {
    JsonDocument doc(256);
    JsonObject obj = doc.to<JsonObject>();
    obj["id"] = f.id;
    obj["name"] = f.name;
    obj["type"] = f.type;
    obj["value"] = f.value;
    obj["description"] = f.description;
    obj["readOnly"] = f.readOnly;
    String result;
    serializeJson(doc, result);
    return result;
}

bool JsonWrapper::jsonToField(const String& jsonStr, Field& f) {
    JsonDocument doc(256);
    auto error = deserializeJson(doc, jsonStr);
    if (error) return false;
    JsonObject obj = doc.as<JsonObject>();
    f.id = obj["id"] | "";
    f.name = obj["name"] | "";
    f.type = obj["type"] | "";
    f.value = obj["value"] | "";
    f.description = obj["description"] | "";
    f.readOnly = obj["readOnly"] | false;
    return true;
}

bool JsonWrapper::jsonToFields(const String& jsonStr, std::vector<Field>& fields) {
    JsonDocument doc(2048);
    auto error = deserializeJson(doc, jsonStr);
    if (error) return false;
    JsonArray arr = doc.as<JsonArray>();
    fields.clear();
    for (JsonObject obj : arr) {
        Field f;
        f.id = obj["id"] | "";
        f.name = obj["name"] | "";
        f.type = obj["type"] | "";
        f.value = obj["value"] | "";
        f.description = obj["description"] | "";
        f.readOnly = obj["readOnly"] | false;
        fields.push_back(f);
    }
    return true;
}

String JsonWrapper::deleteFieldJson(const String& id) {
    JsonDocument doc(128);
    JsonObject obj = doc.to<JsonObject>();
    obj["action"] = "delete";
    obj["id"] = id;
    String s;
    serializeJson(doc, s);
    return s;
}

String JsonWrapper::updateFieldJson(const Field& f) {
    JsonDocument doc(256);
    JsonObject obj = doc.to<JsonObject>();
    obj["action"] = "update";
    obj["id"] = f.id;
    obj["value"] = f.value;
    String s;
    serializeJson(doc, s);
    return s;
}

bool JsonWrapper::saveToFile(const char* path, const std::vector<Field>& fields) {
    if (!SPIFFS.begin(true)) return false;
    File file = SPIFFS.open(path, FILE_WRITE);
    if (!file) return false;
    String s = fieldsToJson(fields);
    file.print(s);
    file.close();
    return true;
}

bool JsonWrapper::loadFromFile(const char* path, std::vector<Field>& fields) {
    if (!SPIFFS.begin(true)) return false;
    if (!SPIFFS.exists(path)) return false;
    File file = SPIFFS.open(path, FILE_READ);
    if (!file) return false;
    String s = file.readString();
    file.close();
    return jsonToFields(s, fields);
}
