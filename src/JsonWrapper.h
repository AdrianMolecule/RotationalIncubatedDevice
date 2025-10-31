#pragma once
#include <ArduinoJson.h>
#include <vector>
#include "Field.h"

class JsonWrapper {
public:
    // Convert vector of Fields → JSON string
    static String fieldsToJson(const std::vector<Field>& fields);

    // Convert a single Field → JSON string
    static String fieldToJson(const Field& f);

    // Parse JSON string → single Field
    static bool jsonToField(const String& jsonStr, Field& f);

    // Parse JSON string → vector<Field>
    static bool jsonToFields(const String& jsonStr, std::vector<Field>& fields);

    // Generate JSON for delete action
    static String deleteFieldJson(const String& id);

    // Generate JSON for update action
    static String updateFieldJson(const Field& f);

    // Save fields to SPIFFS file
    static bool saveToFile(const char* path, const std::vector<Field>& fields);

    // Load fields from SPIFFS file
    static bool loadFromFile(const char* path, std::vector<Field>& fields);
};
