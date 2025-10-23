#include <Arduino.h>

#include "Field.h"
#include "Model.h"
#include "Web.h"
#include "pass.h"

// Create model
Model model = {
    Field("F1", "int", 1, "10"),
    Field("F2", "int", 2, "20"),
    Field("F3", "float", 3, "3.14"),
    Field("F4", "string", 4, "Hello")};

// Create Web interface
Web web(model);

// Only handle serial input here (Web logic encapsulated)
void handleSerialInput() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        int sep = input.indexOf('=');
        if (sep > 0) {
            String name = input.substring(0, sep);
            String value = input.substring(sep + 1);
            Field* field = model.getFieldByName(name);
            if (field) {
                field->setValue(value);
                Serial.printf("Updated via Serial: %s = %s\n", name.c_str(), value.c_str());
                web.updateClientValues();  // push to clients
            } else {
                Serial.println("Unknown field name");
            }
        } else {
            Serial.println("Use format: F1=123 or fieldName=value");
        }
    }
}

void setup() {
    Serial.begin(115200);
    web.begin(readSsid(), readPassword());
}

void loop() {
    handleSerialInput();
}
