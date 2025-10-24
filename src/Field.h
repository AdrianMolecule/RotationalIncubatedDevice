#pragma once
#include <Arduino.h>

class Field {
private:
    String value;

public:
    String name;
    String type; // e.g. "int", "float", "string", "bool"
    String description; // 🆕 New: human-readable label or explanation
    int id;

    Field(String n, String t, int i, String v = "", String d = "");

    String getValue() const;
    void setValue(const String &v);

    // Helpers for UI logic
    bool isNumeric() const { return (type == "int" || type == "float" || type == "double"); }
    bool isBoolean() const { return (type == "bool" || type == "boolean"); }
};
