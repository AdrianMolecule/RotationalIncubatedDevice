// #pragma once
// #include <Arduino.h>

// class Field {
//    private:
//     String id_;
//     String name_;
//     String type_;
//     String value_;
//     bool readOnly_;
//     String description_;

//    public:
//     Field() = default;
//     Field(const String& id, const String& name, const String& type="string",
//           const String& value="0", const String& description="default description", bool readOnly=false) {
//         this->id_ = id;
//         this->name_ = name;
//         this->type_ = type;
//         this->value_= value;
//         this->description_ = description;
//         this->readOnly_ = readOnly;
//     };

//     // Getters
//     const String getId() const { return id_; }
//     const String getName() const { return name_; }
//     const String getType() const { return type_; }
//     const String getValue() const { return value_; }
//     bool getReadOnly() const { return readOnly_; }
//     const String getDescription() const { return description_; }

//     // Setters with logging
//     void setValue(const String& val) {
//         Serial.printf("[Field] setValue: %s -> %s\n", value_.c_str(), val.c_str());
//         value_ = val;
//     }

//     void setName(const String& val) {
//         Serial.printf("[Field] setName: %s -> %s\n", name_.c_str(), val.c_str());
//         name_ = val;
//     }

//     void setId(const String& val) {
//         Serial.printf("[Field] setId: %s -> %s\n", id_.c_str(), val.c_str());
//         id_ = val;
//     }

//     void setType(const String& val) {
//         Serial.printf("[Field] setType: %s -> %s\n", type_.c_str(), val.c_str());
//         type_ = val;
//     }

//     void setReadOnly(bool val) {
//         Serial.printf("[Field] setReadOnly: %d -> %d\n", readOnly_, val);
//         readOnly_ = val;
//     }

//     void setDescription(const String& val) {
//         Serial.printf("[Field] setDescription: %s -> %s\n", description_.c_str(), val.c_str());
//         description_ = val;
//     }
// };
