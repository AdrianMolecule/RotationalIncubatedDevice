#include "JsonWrapper.h"
#include <SPIFFS.h>

const char* FILE_LOCATION = "/model.json";
String JsonWrapper::fieldsToJsonString(const std::vector<Field>& fields) {
    JsonDocument doc;  // sufficient size for fields
    JsonArray arr = doc.to<JsonArray>();
    for (const auto& f : fields) {
        JsonObject obj = arr.add<JsonObject>();
        obj["id"] = f.getId();
        obj["name"] = f.getName();
        obj["type"] = f.getType();
        obj["value"] = f.getValue();
        obj["description"] = f.getDescription();
        obj["readOnly"] = f.getReadOnly();
    }
    String result;
    serializeJson(doc, result);
    return result;
}

String JsonWrapper::fieldToJsonString(const Field& f) {
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    obj["id"] = f.getId();
    obj["name"] = f.getName();
    obj["type"] = f.getType();
    obj["value"] = f.getValue();
    obj["description"] = f.getDescription();
    obj["readOnly"] = f.getReadOnly();
    String result;
    serializeJson(doc, result);
    return result;
}

bool JsonWrapper::jsonToField(const String& jsonStr, Field& f) {
    JsonDocument doc;
    auto error = deserializeJson(doc, jsonStr);
    if (error) return false;
    JsonObject obj = doc.as<JsonObject>();
    f.setId(obj["id"] | "");
    f.setName(obj["name"] | "");
    f.setType(obj["type"] | "");
    f.setValue(obj["value"] | "");
    f.setDescription(obj["description"] | "");
    f.setReadOnly(obj["readOnly"] | false);

    return true;
}

bool JsonWrapper::jsonToFields(const String& jsonStr, std::vector<Field>& fields) {
    JsonDocument doc;
    auto error = deserializeJson(doc, jsonStr);
    if (error) return false;
    JsonArray arr = doc.as<JsonArray>();
    fields.clear();
    for (JsonObject obj : arr) {
        Field f;
        f.setId(obj["id"] | "");
        f.setName(obj["name"] | "");
        f.setType(obj["type"] | "");
        f.setValue(obj["value"] | "");
        f.setDescription(obj["description"] | "");
        f.setReadOnly(obj["readOnly"] | false);
        fields.push_back(f);
    }
    return true;
}

String JsonWrapper::deleteFieldJson(const String& id) {
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    obj["action"] = "delete";
    obj["id"] = id;
    String s;
    serializeJson(doc, s);
    return s;
}

String JsonWrapper::updateFieldJson(const Field& f) {
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    obj["action"] = "update";
    obj["id"] = f.getId();
    obj["value"] = f.getValue();
    String s;
    serializeJson(doc, s);
    return s;
}

bool JsonWrapper::saveModelToFile(const std::vector<Field>& fields) {
    File file = SPIFFS.open(FILE_LOCATION, FILE_WRITE);
    if (!file) {
        Serial.println("Error when trying to open the saveto file. Maybe the location of:" + String(FILE_LOCATION)+ " is not correct or possible");
        return false;
    }
    String s = fieldsToJsonString(fields);
    file.print(s);
    file.close();
    return true;
}

bool JsonWrapper::loadFieldsFromFile(std::vector<Field>& fields) {
    if (!SPIFFS.begin(true)) return false;
    if (!SPIFFS.exists(FILE_LOCATION)) return false;
    File file = SPIFFS.open(FILE_LOCATION, FILE_READ);
    if (!file) return false;
    String s = file.readString();
    file.close();
    return jsonToFields(s, fields);
}
