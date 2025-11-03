// #include "Model.h"

// #include <SPIFFS.h>


// bool Model::load(const char* path) {
//     if (!SPIFFS.exists(path)) {
//         Serial.printf("[FS] %s not found, using defaults\n", path);
//         //createDefaultFields();
//         save(path);
//         return false;
//     }
//     return JsonWrapper::loadModelFromFile(path, fields);
// }

// bool Model::save(const char* path) const {
//     return JsonWrapper::saveModelToFile(path, fields);
// }

// Field* Model::findField(const String& name) {
//     for (auto& f : fields)
//         if (f.getName() == name) return &f;
//     return nullptr;
// }

// bool Model::setValue(const String& name, const String& value) {
//     Field* f = findField(name);
//     if (!f) return false;
//     if (f->getReadOnly()) return false;
//     f->setValue(value);
//     return true;
// }

// String Model::getValue(const String& name) const {
//     for (const auto& f : fields)
//         if (f.getName() == name)
//             return f.getValue();
//     return "";
// }
