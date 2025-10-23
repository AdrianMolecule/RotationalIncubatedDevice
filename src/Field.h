#pragma once
#include <Arduino.h>

class Field {
private:
    String value;

public:
    String name;
    String type; // e.g. "int", "float", "string"
    int id;

    Field(String n, String t, int i, String v = "");

    String getValue() const;
    void setValue(const String &v);
};
#include "Field.h"