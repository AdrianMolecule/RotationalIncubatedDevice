#include "Field.h"

Field::Field(String n, String t, int i, String v)
    : name(n), type(t), id(i), value(v) {}

String Field::getValue() const {
    return value;
}

void Field::setValue(const String& v) {
    value = v;
}
