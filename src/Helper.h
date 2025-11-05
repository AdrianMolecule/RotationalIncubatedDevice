#pragma once
#include <Arduino.h>
#include <time.h>

const String NOT_PRESENT = String("255");

class Helper {
   public:
    Helper() = default;
    static void initialize(std::vector<Field>& fields) {
        Serial.println("[MODEL] Loading factory default");
        int idCounter = 0;
        // desired or set values
        fields.emplace_back(String(idCounter++), "desiredTemperature", "float", "37", "desired Temperature");
        fields.emplace_back(String(idCounter++), "currentTemperature", "float", "-1", "current Temperature", true);
        fields.emplace_back(String(idCounter++), "currentRPM", "float ", "300", "current RPM");
        fields.emplace_back(String(idCounter++), "currentHeatingEndDurationInMinutes", "unsigned long ", NOT_PRESENT, "currentHeatingEndDurationInMinutes", false);
        // pins
        fields.emplace_back(String(idCounter++), "HeaterPin", "uint8_t ", NOT_PRESENT, "Heater Pin", false);
        fields.emplace_back(String(idCounter++), "HeaterPwmChannel", "uint8_t ", NOT_PRESENT, "HeaterPwmChannel");
        fields.emplace_back(String(idCounter++), "StepperPwmStepPin", "uint8_t ", String(25), "StepperPwmStepPin");
        fields.emplace_back(String(idCounter++), "StepperPwmChannel", "uint8_t ", String(2), "StepperPwmChannel");
        fields.emplace_back(String(idCounter++), "StepperEnablePin", "uint8_t ", String(26), "StepperEnablePin");
        fields.emplace_back(String(idCounter++), "I2SoClockPin", "uint8_t ", NOT_PRESENT, "I2SoClockPin");
        fields.emplace_back(String(idCounter++), "I2SoLatchPin", "uint8_t ", NOT_PRESENT, "I2SoLatchPin");
        fields.emplace_back(String(idCounter++), "LedPin", "uint8_t ", String(2), "LedPin");
        fields.emplace_back(String(idCounter++), "PotentiometerPin", "uint8_t ", NOT_PRESENT, "PotentiometerPin");
        fields.emplace_back(String(idCounter++), "TempSensorPin", "uint8_t ", NOT_PRESENT, "TempSensorPin");
        fields.emplace_back(String(idCounter++), "SpeakerPin", "uint8_t ", String(12), "SpeakerPin");
        fields.emplace_back(String(idCounter++), "SpeakerChannel", "uint8_t ", String(4), "SpeakerChannel");
        fields.emplace_back(String(idCounter++), "FanPin", "uint8_t ", NOT_PRESENT, "FanPin");
        fields.emplace_back(String(idCounter++), "FanPwmChannel", "uint8_t ", NOT_PRESENT, "FanPwmChannel");
        fields.emplace_back(String(idCounter++), "MemoryCsPin", "uint8_t ", NOT_PRESENT, "MemoryCsPin");
        fields.emplace_back(String(idCounter++), "SpindleEnablePin", "uint8_t ", NOT_PRESENT, "SpindleEnablePin");
        // misc
        fields.emplace_back(String(idCounter++), "UseOneWireForTemperature", "bool", "0", "UseOneWireForTemperature");
        fields.emplace_back(String(idCounter++), "MKSBoard", "bool", "0", "MKSBoard");
        // Preferences
        fields.emplace_back(String(idCounter++), "preference_TimeDisplay", "bool", "1", "preference_TimeDisplay");
        fields.emplace_back(String(idCounter++), "preference_TemperatureDisplay", "bool", "1", "preference_TemperatureDisplay");
        fields.emplace_back(String(idCounter++), "preference_MostMusic_OFF", "bool", "0", "preference_MostMusic_OFF");
        fields.emplace_back(String(idCounter++), "preference_TemperatureReached_MusicOn", "bool", "1", "preference_TemperatureReached_MusicOn");
        // Debug Flags
        fields.emplace_back(String(idCounter++), "DEBUG_HEATER", "bool", "1", "DEBUG_HEATER");
        fields.emplace_back(String(idCounter++), "DEBUG_FAN", "bool", "1", "DEBUG_FAN");
        fields.emplace_back(String(idCounter++), "DEBUG_SWITCH", "bool", "1", "DEBUG_SWITCH");
        fields.emplace_back(String(idCounter++), "LowHumidityAlert", "bool", "1", "Alert if LowHumidity detected, works only when we sue DH not UseOneWireForTemperature");
        fields.emplace_back(String(idCounter++), "VERSION", "string", "1.1", "Version", true);
        uint8_t maxHeaterDutyCyclePercentage;
        int currentStepsPerRotation;
        unsigned long currentStartTime;  // time since ESP power on in millis
    }

    static String getTime() {
        struct tm timeinfo;
        // getLocalTime fills the 'timeinfo' structure.
        // It returns true on success, false on failure (e.g., no Wi-Fi/NTP sync issue)
        if (!getLocalTime(&timeinfo)) {
            return "Failed to obtain time";
        }
        // A buffer is required to hold the formatted C-style string temporarily
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%A, %b %d %Y %I:%M:%S %p", &timeinfo);
        return String(buffer);
    }
};
