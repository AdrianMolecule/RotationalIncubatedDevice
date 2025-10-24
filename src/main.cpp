#include <Arduino.h>

#include "Field.h"
#include "Model.h"
#include "Web.h"
#include "pass.h"

// Model holds all field objects (single source of truth)
Model model;

// Web encapsulates WiFi, WebSocket, and server logic
Web web(model);

void setup() {
    Serial.begin(115200);

    // Add fields to the model
    model.addField(Field(1, "f1", "int", "First integer", "0"));
    model.addField(Field(2, "f2", "int", "Second integer", "0"));
    model.addField(Field(3, "flag", "bool", "A boolean flag", "false"));
    model.addField(Field(4, "name", "string", "User name", "ESP32"));

    // Initialize WiFi and start web server
    web.begin(readSsid(), readPassword());
}

void loop() {
    // Handle serial input to update model
    // Format: fieldname=value, e.g., f1=42
    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        int eq = line.indexOf('=');
        if (eq > 0) {
            String field = line.substring(0, eq);
            String value = line.substring(eq + 1);
            model.setValue(field, value);
            Serial.println("Updated " + field + " = " + value);

            // Push updated values to all connected web clients
            web.updateClientValues();
        }
    }
}
