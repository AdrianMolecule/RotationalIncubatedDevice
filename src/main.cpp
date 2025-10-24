#include <Arduino.h>

#include "Field.h"
#include "Model.h"
#include "Web.h"
#include "pass.h"

Model model = {
    Field("F1", "int", 1, "10", "Integer field example"),
    Field("F2", "float", 2, "2.718", "Floating-point field"),
    Field("Enabled", "bool", 3, "true", "Enable or disable feature"),
    Field("Label", "string", 4, "Hello", "Label text for display")};

Web web(model);

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
                model.saveField(*field);
                Serial.printf("Updated via Serial: %s = %s (saved)\n", name.c_str(), value.c_str());
                web.updateClientValues();
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
    model.begin();
    web.begin(readSsid(), readPassword());
}

void loop() {
    handleSerialInput();
}
