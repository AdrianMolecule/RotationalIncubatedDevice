#pragma once
#include <Arduino.h>
#include <time.h>
#include "MyMusic.h"

const String NOT_PRESENT = String("255");

class Helper {
   public:
    Helper() = default;
    static inline int idCounter = 0;
    static void initialize(std::vector<Field>& fields) {
        fields.clear();
        // desired or set values AT this point for Tube rotator
        fields.emplace_back(getNextIdStr(), "status", "string", "", "Status for device", true /*RO*/, true /*shown */, false /*not persisted */);      // MANDATORY
        fields.emplace_back(getNextIdStr(), "error", "error", "", "Error status for device", true /*RO*/, true /*shown */, false /*not persisted */);  // MANDATORY
        fields.emplace_back(getNextIdStr(), "warning", "warning", "", "warnings", true /*RO*/, true /*shown */, false /*not persisted */);  // MANDATORY
        fields.emplace_back(getNextIdStr(), "desiredTemperature", "float", "37", "Desired Temperature", false, true);
        fields.emplace_back(getNextIdStr(), "currentTemperature", "float", "-1", "Current Temperature", true, true, false);
        fields.emplace_back(getNextIdStr(), "currentHeaterOn", "bool", "0", "Shows current/desired heater state controlled by the device but overridden by HeaterDisabled ", true, true, false);
        fields.emplace_back(getNextIdStr(), "Rpm", "float", "200", "desired RPM. You need to restart stepper to achieve this RPM.", false, true);
        fields.emplace_back(getNextIdStr(), "stepsPerRotation", "int", "200", "Desired microstepping, only 200,400 ... 6400");
        fields.emplace_back(getNextIdStr(), "currentStepperOnOffSwitchPosition", "bool", "1", "Shows current StepperOnOffSwitch Position. Off means turn stepper Off. Ignored if no switch", true, true, false);
        fields.emplace_back(getNextIdStr(), "time", "string", "no time", "Shows device current time/date", true, true, false);  // readonly isSHown, is not persisted
        fields.emplace_back(getNextIdStr(), "bootTime", "string", "0", "Device startup time", true, true, false);
        // on offs read from UI and set on the board. They might be overridden by physical switches
        fields.emplace_back(getNextIdStr(), "StepperOn", "bool", "0", "Turns on off stepper", false, true, false);
        fields.emplace_back(getNextIdStr(), "FanOn", "bool", "1", "Turns on off fan if capability exists", false, true);
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
        fields.emplace_back(getNextIdStr(), "SpeakerPin", "uint8_t", "23", "SpeakerPin");  // String(32) or String(12) The physical interface on the board is a 2-pin connector typically labeled BZ or Buzzer, with one pin providing providing a GND (ground) connection.
        fields.emplace_back(getNextIdStr(), "FanPin", "uint8_t", NOT_PRESENT, "FanPin");   // 33 is probably available but Probably will not work with any decent size fan because it's too weak. 33 LCD-RS currently pin 8 of expansion 1. see schematics in root
        fields.emplace_back(getNextIdStr(), "LedPwmPin", "uint8_t", "2", "LedPwmPin");
        fields.emplace_back(getNextIdStr(), "PotentiometerPin", "uint8_t", NOT_PRESENT, "PotentiometerPin");
        fields.emplace_back(getNextIdStr(), "MemoryCsPin", "uint8_t", NOT_PRESENT, "MemoryCsPin");
        //
        fields.emplace_back(getNextIdStr(), "maxHeaterDutyCycle", "int", "80", "Max Heater Duty Cycle- normally 80-100%");
        fields.emplace_back(getNextIdStr(), "MKSBoard", "bool", "1", "MKSBoard");
        fields.emplace_back(getNextIdStr(), "StepperOnOffSwitchInputPin", "int", "36", "sets the pin to read the on-off-physical button if present ");  //yes 36 is for the big OS with MKS Input /*Sensor_VP SVP -*/   
        fields.emplace_back(getNextIdStr(), "StepperOnOffSoftwareSwitchOutputPin", "uint8_t", NOT_PRESENT, "on my board we can control the on off by using ENABLEpin in output mode");  // maybe 26
        //
        fields.emplace_back(getNextIdStr(), "MostMusicOff", "bool", "0", "Most Music Off except for alarms", false, true);        // turns off all music except for errors, warnings, time reached and first time desired temperature reached
        fields.emplace_back(getNextIdStr(), "TemperatureReachedMusicOn", "bool", "1", "TemperatureReachedMusicOn", false, true);  // turns off all music except for errors, warnings, time reached and first time desired temperature reached
        //
        fields.emplace_back(getNextIdStr(), "UseOneWireForTemperature", "bool", "0", "UseOneWireForTemperature");  // turns off all music except for errors, warnings, time reached and first time desired temperature reached

        fields.emplace_back(getNextIdStr(), "desiredHeatingEndTime", "string", "-1", "Heat cutoff time in 2025-11-12 13:00:00 format or -1 for no cutoff", false, true);
        fields.emplace_back(getNextIdStr(), "alarmTurnsHeatingOff", "bool", "0", "Timed alarm will also Turn Heating Off if alarm time is reached");
        fields.emplace_back(getNextIdStr(), "LowHumidityAlert", "bool", "0", "Alert if LowHumidity detected, works only for sensor DH..");
        //
        fields.emplace_back(getNextIdStr(), "version", "string", DNS, "Version", true, true, true);
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

   private:
    static inline char buffer[4] = {0};  // can hold 3 digits so up to 999

    static const char* getNextIdStr() {
        // snprintf safely formats the integer into the static buffer. We use the current value of counter, then increment it.
        std::snprintf(buffer, sizeof(buffer), "%d", idCounter++);        
        return buffer;// Return the address of he static buffer.the memory is valid after the function returns.
    }
};
// Just for doc from prev version for MKS
// // for m17hs16-2004s1 https://www.omc-stepperonline.com/nema-17-bipolar-45ncm-64oz-in-2a-42x42x40mm-4-wires-w-1m-cable-connector-17hs16-2004s1 and Imax=2 AMp hold torque=Holding Torque: 45Ncm(64oz.in)
// // and https://www.youtube.com/watch?v=BV-ouxhZamI
// //   to measure on screw-driver=2*8*.068 about 1 volt but I'll go less maybe .9 volts
// //
// // good doc for pins at https://github.com/diruuu/FluidNC/blob/main/example_configs/MKS-DLC32-v2.0.yaml
// enum {  // the number means IO pin heater=32 means heather is GPIO32
//     HEATER_PIN = 32 /*this is internally connected to spindle*/,
//     HEATER_PWM_CHANNEL = 0,
//     STEPPER_PWM_STEP_PIN = 22 /*probe pin*/,
//     STEPPER_PWM_CHANNEL = 2, /*NOT USED I2SO_BUZZ_PIN = 0000???,*/
//     I2SO_DATA_PIN = 21,
//     I2SO_CLOCK_PIN = 16,
//     I2SO_LATCH_PIN = 17,
//     LED_PIN = 2,
//     POTENTIOMETER_PIN = 127,
//     TEMP_SENSOR_PIN = 19 /* LCD_MISO*/,
//     TURN_ON_STEPPER_PIN = 36 /*Sensor_VP SVP -*/,
//     SPEAKER = 23,
//     /*LCD_MOSI*/ SPEAKER_CHANNEL = 4,
//     FAN_PIN = 33 /*LCD_RS*/,
//     MEMORY_CS_PIN = 15,
//     /*used internally by SD no need to declare*/ SD_D0_PIN = 12,
//     SD_DI = 13,
//     SD_CK = 14,
//     SD_CS = 15,
//     SD_DET = 39,
//     SPINDLE_ENABLE_PIN = 27, /*internally connected*/
// };

//stepper button candidates for OS mks: 36No,26,25
// some potential probably for my custom pcb:
//  desired or set values AT this point for Tube rotator
// fields.emplace_back(getNextIdStr(), "desiredTemperature", "float", "37", "desired Temperature");
// fields.emplace_back(getNextIdStr(), "currentTemperature", "float", "-1", "current Temperature", true);
// fields.emplace_back(getNextIdStr(), "Rpm", "float ", "300", "current and desired RPM");
// fields.emplace_back(getNextIdStr(), "currentHeatingEndDurationInMinutes", "int ", NOT_PRESENT, "desiredEndTime. Please enter an end time for end time alarm in minutes e 60 for 1 hour from start. You can separately reset the start time to now.", false);
// // on offs
// fields.emplace_back(getNextIdStr(), "StepperOn", "bool", "36   ", "turns on off stepper");  //"1"
// fields.emplace_back(getNextIdStr(), "FanOn", "bool", "1", "turns on off fan if capability exists");
// fields.emplace_back(getNextIdStr(), "HeaterOn", "bool", "1", "turns on off heater");
// // pins
// fields.emplace_back(getNextIdStr(), "HeaterPwmPin", "uint8_t", NOT_PRESENT, "Heater Pwm Pin", false);
// fields.emplace_back(getNextIdStr(), "StepperPwmStepPin", "uint8_t", String(25), "StepperPwmStepPin");
// fields.emplace_back(getNextIdStr(), "StepperEnablePin", "uint8_t", String(26), "StepperEnablePin");
// fields.emplace_back(getNextIdStr(), "I2SoClockPin", "uint8_t", NOT_PRESENT, "I2SoClockPin");
// fields.emplace_back(getNextIdStr(), "I2SoLatchPin", "uint8_t", NOT_PRESENT, "I2SoLatchPin");
// fields.emplace_back(getNextIdStr(), "LedPwmPin", "uint8_t", String(2), "LedPwmPin");
// fields.emplace_back(getNextIdStr(), "PotentiometerPin", "uint8_t", NOT_PRESENT, "PotentiometerPin");
// fields.emplace_back(getNextIdStr(), "TempSensorPin", "uint8_t", "19", "TempSensorPin");  // we use one main pin for either dh or OneWIre and that is pin GPIO19/LCD_MISO on pin 1 of Expansion 2 connector
// fields.emplace_back(getNextIdStr(), "SpeakerPin", "uint8_t", String(32), "SpeakerPin");  // or String(12)
// fields.emplace_back(getNextIdStr(), "SpeakerChannel", "uint8_t", String(4), "SpeakerChannel");
// fields.emplace_back(getNextIdStr(), "FanPin", "uint8_t", NOT_PRESENT, "FanPin");
// fields.emplace_back(getNextIdStr(), "FanPwmChannel", "uint8_t", NOT_PRESENT, "FanPwmChannel");
// fields.emplace_back(getNextIdStr(), "MemoryCsPin", "uint8_t", NOT_PRESENT, "MemoryCsPin");
