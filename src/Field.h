#pragma once
#include <Arduino.h>

// Class Field represents a single field in the model
// Each Field has an ID, name, type, description, and a string value
class Field {
public:
    int id;
    String name;
    String type;
    String description;
private:
    String value;

public:
    Field(int _id, String _name, String _type, String _description, String _value = "")
        : id(_id), name(_name), type(_type), description(_description), value(_value) {}

    String getValue() const { return value; }
    void setValue(const String &val) { value = val; }

    bool isNumeric() const { return type == "int" || type == "float" || type == "double"; }
    bool isBoolean() const { return type == "bool"; }
};
