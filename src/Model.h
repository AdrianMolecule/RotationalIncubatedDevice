// #pragma once
// #include <Arduino.h>

// #include <vector>

// #include "Field.h"
// #include "JsonWrapper.h"

// class Model {
//    private:
//     std::vector<Field> fields;

//    public:
//     Model() {}

//     // Load / save
//     bool save(const char* path) const;
//     bool load(const char* path);
//     void loadDefaults() {
//         this->fields = std::vector<Field>{
//             Field("1", "Speed", "float", "0.0", "Motor speed in RPM", false),
//             Field("2", "Duration", "int", "10", "Operation duration in seconds", false),
//             Field("3", "Enabled", "bool", "true", "Enable the system", false),
//             Field("4", "Mode", "string", "Auto", "Operation mode", true)};
//         Serial.println("model initaialized with default fields");
//     };

//     // Access
//     std::vector<Field>& getFields() { return fields; }
//     const std::vector<Field>& getFields() const { return fields; }

//     Field* findField(const String& name);
//     bool setValue(const String& name, const String& value);
//     String getValue(const String& name) const;
//     // std::vector<Field> createDefaultFields() ;
// };
