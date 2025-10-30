#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include "Field.h"

// Centralized JSON handling layer
// Ensures full ArduinoJson 7.4.2 compliance and isolates JSON logic
class JsonWrapper {
public:
    // Save fields to SPIFFS file
    static bool saveToFile(const char* path, const std::vector<Field>& fields);

    // Load fields from SPIFFS file
    static bool loadFromFile(const char* path, std::vector<Field>& fields);

    // Serialize fields vector → JSON string
    static String toString(const std::vector<Field>& fields);

    // Parse JSON string → vector<Field>
    static bool fromString(const String& json, std::vector<Field>& fields);
};
