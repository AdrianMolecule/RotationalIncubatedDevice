#include "JsonWrapper.h"

#include <MyMusic.h>  // Include to use MyMusic::MajorAlarm
#include <SPIFFS.h>

const char* FILE_LOCATION = "/model.json";
// STATIC BUFFER: Pre-allocated memory to prevent heap fragmentation
static char jsonBuffer[8192];

// Forward Declarations for Helpers
void appendRaw(char*& ptr, const char* str, const char* end);
void appendEscaped(char*& ptr, const char* str, const char* end);

const char* JsonWrapper::toJsonString(const std::vector<Field>& fields, bool justPersisted) {
    char* ptr = jsonBuffer;
    const char* end = jsonBuffer + sizeof(jsonBuffer);
    appendRaw(ptr, "[", end);
    for (size_t i = 0; i < fields.size(); i++) {
        const auto& f = fields[i];
        if (i > 0) appendRaw(ptr, ",", end);
        // CRITICAL CHECK: Ensure we haven't already exceeded the buffer size
        if (ptr >= end - 50) {
            MyMusic::MajorAlarm("JsonBuffer Overflow: Data truncated!");  // CHANGED CALL
            *ptr = ']';                                                   // Terminate the array immediately
            *(ptr + 1) = '\0';
            return jsonBuffer;
        }
        appendRaw(ptr, "{\"id\":\"", end);
        appendEscaped(ptr, f.getId().c_str(), end);
        appendRaw(ptr, "\",\"name\":\"", end);
        appendEscaped(ptr, f.getName().c_str(), end);
        appendRaw(ptr, "\",\"type\":\"", end);
        appendEscaped(ptr, f.getType().c_str(), end);
        appendRaw(ptr, "\",\"value\":\"", end);
        if (justPersisted && !f.getIsPersisted()) {
            appendEscaped(ptr, "", end);
        }else{
            appendEscaped(ptr, f.getValue().c_str(), end);
        }
        appendRaw(ptr, "\",\"description\":\"", end);
        appendEscaped(ptr, f.getDescription().c_str(), end);
        appendRaw(ptr, "\",\"readOnly\":", end);
        appendRaw(ptr, f.getReadOnly() ? "true" : "false", end);
        appendRaw(ptr, ",\"isShown\":", end);
        appendRaw(ptr, f.getIsShown() ? "true" : "false", end);
        appendRaw(ptr, ",\"isPersisted\":", end);
        appendRaw(ptr, f.getIsPersisted() ? "true" : "false", end);
        appendRaw(ptr, "}", end);
    }
    appendRaw(ptr, "]", end);
    return jsonBuffer;
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
    obj["isShown"] = f.getIsShown();
    obj["isPersisted"] = f.getIsPersisted();
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
    f.setIsShown(obj["isShown"] | false);
    f.setIsPersisted(obj["isPersisted"] | true);
    return true;
}

bool JsonWrapper::jsonToFields(const String& jsonStr, std::vector<Field>& fields) {
    JsonDocument doc;
    auto error = deserializeJson(doc, jsonStr);
    if (error) {
        String er = String(": jsonToFields could not read as valid Json the string:") + String(jsonStr);
        MyMusic::MajorAlarm(er.c_str());
        return false;
    }
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
        f.setIsShown(obj["isShown"] | false);
        f.setIsPersisted(obj["isPersisted"] | true);
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
    Serial.println(">>>>>>>>>>>>>>>>>>>>>>>>>>>>Saving Model to Flash >>>>>>>>>>>>>>>>>>>>>>>>>>>>");
    File file = SPIFFS.open(FILE_LOCATION, FILE_WRITE);
    if (!file) {
        MyMusic::MajorAlarm("SPIFFS File Open Failed - Save Aborted.");  // CHANGED CALL
        return false;
    }
    file.print(toJsonString(fields, JUST_PERSISTED_FIELDS));
    file.close();
    return true;
}

bool JsonWrapper::checkJson(const String& jsonStr) {
    JsonDocument doc;
    auto error = deserializeJson(doc, jsonStr);
    if (error) return false;
    return true;
}

bool JsonWrapper::loadFieldsFromFile(std::vector<Field>& fields) {
    if (!SPIFFS.begin(false)) {
        MyMusic::MajorAlarm("SPIFFS Initialization Failed - Cannot Load Model.");  // CHANGED CALL
        return false;
    }
    if (!SPIFFS.exists(FILE_LOCATION)) return false;
    File file = SPIFFS.open(FILE_LOCATION, FILE_READ);
    if (!file) {
        MyMusic::MajorAlarm("SPIFFS File Open Failed - Load Aborted.");  // CHANGED CALL
        return false;
    }
    String s = file.readString();
    file.close();
    return jsonToFields(s, fields);
}

// Helper Implementations moved to bottom
void appendRaw(char*& ptr, const char* str, const char* end) {
    while (*str && ptr < end - 1) {
        *ptr++ = *str++;
    }
    *ptr = '\0';
}

void appendEscaped(char*& ptr, const char* str, const char* end) {
    while (*str && ptr < end - 2) {
        if (*str == '"' || *str == '\\') {
            *ptr++ = '\\';
        }
        *ptr++ = *str++;
    }
    *ptr = '\0';
}