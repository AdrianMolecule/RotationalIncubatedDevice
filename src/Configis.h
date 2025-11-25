#pragma once
#include <Arduino.h>
#include "Field.h"


class Config {
    public:
    static inline const char* NOT_PRESENT = "255";
    static inline const int NOT_PRESENT_INT = 255;
    static inline const char* FIRMWARE_VERSION = "IS-Incubated Shaker 1.4";
    static inline const char* DNS = "biois";  // 192.168.0.190
    static constexpr const char* COMPILE_DATE = __DATE__;
    static constexpr const char* COMPILE_TIME = __TIME__;
    //
    Config() = default;
    static void initializeHardcodedFields(std::vector<Field>& fields) {
        fields.clear();
        fields.emplace_back(getNextIdStr(), "error", "string", "", "Error status for device", true /*RO*/, true /*shown */, false /*not persisted */);    // MANDATORY
        fields.emplace_back(getNextIdStr(), "warning", "string", "", "warnings for the device", true /*RO*/, true /*shown */, false /*not persisted */);  // MANDATORY
    };
    static inline int idCounter = 0;
    static const int initialSpeakerPin = 23;
    static void initialize(std::vector<Field>& fields) {
        initializeHardcodedFields(fields);
        // desired or set values AT this point for Tube rotator
        fields.emplace_back(getNextIdStr(), "status", "string", "", "Status for device", true /*RO*/, true /*shown */, false /*not persisted */);         // MANDATORY
        fields.emplace_back(getNextIdStr(), "error", "string", "", "Error status for device", true /*RO*/, true /*shown */, false /*not persisted */);    // MANDATORY
        fields.emplace_back(getNextIdStr(), "warning", "string", "", "warnings for the device", true /*RO*/, true /*shown */, false /*not persisted */);  // MANDATORY
        fields.emplace_back(getNextIdStr(), "desiredTemperature", "float", "37", "Desired Temperature", false, true);
        fields.emplace_back(getNextIdStr(), "currentTemperature", "float", "-1", "Current Temperature", true, true, false);
        fields.emplace_back(getNextIdStr(), "stepperOn", "bool", "0", "Turns stepper on off ", false, true, false);
        fields.emplace_back(getNextIdStr(), "currentHeaterOn", "bool", "0", "Shows current/desired heater state controlled by the device but overridden by HeaterDisabled ", true, true, false);
        fields.emplace_back(getNextIdStr(), "rpm", "float", "250", "desired RPM. You need to restart stepper to achieve this RPM.", false, true);
        fields.emplace_back(getNextIdStr(), "stepsPerRotation", "int", "200", "Desired microstepping, only 200,400 ... 6400");
        fields.emplace_back(getNextIdStr(), "currentStepperOnOffSwitchPosition", "bool", "1", "Shows current StepperOnOffSwitch Position. Off means turn stepper Off. Ignored if no switch", true, false, false);
        fields.emplace_back(getNextIdStr(), "time", "string", "no time", "Shows device current time/date", true, true, false);  // readonly isSHown, is not persisted
        fields.emplace_back(getNextIdStr(), "bootTime", "string", "0", "Device startup time", true, true, false);
        // on offs read from UI and set on the board. They might be overridden by physical switches
        fields.emplace_back(getNextIdStr(), "fanOn", "bool", "1", "Turns on off fan if capability exists", false, true);
        fields.emplace_back(getNextIdStr(), "HeaterDisabled", "bool", "0", "Disables heater even if current temp lower than desired temp");
        // pins
        fields.emplace_back(getNextIdStr(), "TempSensorPin", "uint8_t", "19", "TempSensorPin");  // we use one main pin for either dh or OneWIre and that is pin GPIO19/LCD_MISO on pin 1 of Expansion 2 connector
        // google says use GPIO25- LCD-CS0  for driving the spindle: Pin 6 on the EXP2 header
        fields.emplace_back(getNextIdStr(), "HeaterPwmPin", "uint8_t", "32", "Heater Pwm Pin");  // 32 is internally connected to spindle*/,
        fields.emplace_back(getNextIdStr(), "StepperPwmStepPin", "uint8_t", "22", "StepperPwmStepPin");
        // misc
        fields.emplace_back(getNextIdStr(), "I2SoDataPin", "uint8_t", "21", "I2SoDataPin");
        fields.emplace_back(getNextIdStr(), "I2SoClockPin", "uint8_t", "16", "I2SoClockPin");
        fields.emplace_back(getNextIdStr(), "I2SoLatchPin", "uint8_t", "17", "I2SoLatchPin");
        fields.emplace_back(getNextIdStr(), "SpeakerPin", "uint8_t", String(initialSpeakerPin), "SpeakerPin");  // String(32) or String(12) The physical interface on the board is a 2-pin connector typically labeled BZ or Buzzer, with one pin providing providing a GND (ground) connection.
        fields.emplace_back(getNextIdStr(), "fanPin", "uint8_t", NOT_PRESENT, "fanPin");   // 33 is probably available but Probably will not work with any decent size fan because it's too weak. 33 LCD-RS currently pin 8 of expansion 1. see schematics in root
        fields.emplace_back(getNextIdStr(), "LedPwmPin", "uint8_t", "2", "LedPwmPin");
        fields.emplace_back(getNextIdStr(), "PotentiometerPin", "uint8_t", NOT_PRESENT, "PotentiometerPin");
        fields.emplace_back(getNextIdStr(), "MemoryCsPin", "uint8_t", NOT_PRESENT, "MemoryCsPin");
        //
        fields.emplace_back(getNextIdStr(), "maxHeaterDutyCycle", "int", "85", "Max Heater Duty Cycle- normally 80-100%");
        fields.emplace_back(getNextIdStr(), "MKSBoard", "bool", "1", "MKSBoard");
        fields.emplace_back(getNextIdStr(), "StepperOnOffSwitchInputPin", "int", "36", "sets the pin to read the on-off-physical button if present ");                                  // yes 36 is for the big OS with MKS Input /*Sensor_VP SVP -*/
        fields.emplace_back(getNextIdStr(), "StepperOnOffSoftwareSwitchOutputPin", "uint8_t", NOT_PRESENT, "on my board we can control the on off by using ENABLEpin in output mode");  // maybe 26
        //
        fields.emplace_back(getNextIdStr(), "MostMusicOff", "bool", "0", "Most Music Off except for alarms");        // turns off all music except for errors, warnings, time reached and first time desired temperature reached
        fields.emplace_back(getNextIdStr(), "temperatureReachedMusicOn", "bool", "1", "TemperatureReachedMusicOn", false, true);  // turns off all  music except for errors, warnings, time reached and first time desired temperature reached
        //
        fields.emplace_back(getNextIdStr(), "UseOneWireForTemperature", "bool", "0", "UseOneWireForTemperature");  // turns off all music except for errors, warnings, time reached and first time desired temperature reached

        fields.emplace_back(getNextIdStr(), "desiredProcessEndTime", "string", "-1", "Process End Time in 2025-11-12 13:00:00 format or -1 for no cutoff", false, true);
        fields.emplace_back(getNextIdStr(), "alarmTurnsHeatingOff", "bool", "0", "Timed alarm will also Turn Heating Off if alarm time is reached");
        fields.emplace_back(getNextIdStr(), "LowHumidityAlert", "bool", "0", "Alert if LowHumidity detected, works only for sensor DH..");
    }
    //
    static void initializeSample(std::vector<Field>& fields) {
        int idCounter = 0;
        // desired or set values
        fields.emplace_back(getNextIdStr(), "a", "float", "37", "desired Temperature");
        fields.emplace_back(getNextIdStr(), "b", "int", "0", "b");
        fields.emplace_back(getNextIdStr(), "sample version", "string", "sample", "Version", true);
        // todo add currentStartTime
    }
//
    static String getUptimeString() {
        uint64_t ms = millis();
        uint32_t sec = ms / 1000;
        uint32_t min = sec / 60;
        uint32_t hr = min / 60;
        uint32_t day = hr / 24;
        sec %= 60;
        min %= 60;
        hr %= 24;
        char buf[64];
        sprintf(buf, "%u d %02u:%02u:%02u", day, hr, min, sec);
        return String(buf);
    }
    static String getVersionString() {
        char buf[64];
        sprintf(buf, "%s, url:%s%s", FIRMWARE_VERSION, DNS, ".local");
        return String(buf);
    }

   private:
    static inline char buffer[4] = {0};  // can hold 3 digits so up to 999

    static const char* getNextIdStr() {
        // snprintf safely formats the integer into the static buffer. We use the current value of counter, then increment it.
        std::snprintf(buffer, sizeof(buffer), "%d", idCounter++);
        return buffer;  // Return the address of he static buffer.the memory is valid after the function returns.
    }
};