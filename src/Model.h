#pragma once
#include <Arduino.h>
#include <vector>
#include "Field.h"

class Model {
public:
    std::vector<Field> fields;

    Model(std::initializer_list<Field> initList);
    size_t size() const;
    Field* getFieldById(int id);
    Field* getFieldByName(const String &name);
};
