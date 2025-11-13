#pragma once
#include <ArduinoJson.h>

#include <vector>

#include "Field.h"

class JsonWrapper {
   public:
    // Convert vector of Fields -> JSON string
    static const char* toJsonString(const std::vector<Field>& fields);
    // Convert a single Field -> JSON string
    static String fieldToJsonString(const Field& f);
    // Parse JSON string -> single Field
    static bool jsonToField(const String& jsonStr, Field& f);
    // Parse JSON string -> vector<Field>
    static bool jsonToFields(const String& jsonStr, std::vector<Field>& fields);
    // Generate JSON for delete action
    static String deleteFieldJson(const String& id);
    // Generate JSON for update action
    static String updateFieldJson(const Field& f);
    // Save fields to SPIFFS file
    static bool saveModelToFile(const std::vector<Field>& fields);
    // Load fields from SPIFFS file
    static bool loadFieldsFromFile(std::vector<Field>& fields);
    // checks if a string is proper Json when upload from Serial or Web
    static bool checkJson(const String& jsonStr);
};