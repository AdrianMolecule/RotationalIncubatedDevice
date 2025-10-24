#include "Field.h"

Field::Field(String n, String t, int i, String v, String d)
    : name(n), type(t), id(i), value(v), description(d) {}

String Field::getValue() const {
    return value;
}

void Field::setValue(const String& v) {
    value = v;
}
