#include "Model.h"

Model::Model(std::initializer_list<Field> initList)
    : fields(initList) {}

size_t Model::size() const {
    return fields.size();
}

Field* Model::getFieldById(int id) {
    for (auto& f : fields)
        if (f.id == id) return &f;
    return nullptr;
}

Field* Model::getFieldByName(const String& name) {
    for (auto& f : fields)
        if (f.name == name) return &f;
    return nullptr;
}
