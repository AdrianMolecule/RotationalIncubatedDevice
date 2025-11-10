#pragma once
#include <Arduino.h>
#include <time.h>

const String NOT_PRESENT = String("255");

class Helper {
   public:
    Helper() = default;
    static void initialize(std::vector<Field>& fields) {
        fields.clear();
        int idCounter = 0;
        // desired or set values AT this point for Tube rotator
        fields.emplace_back(String(idCounter++), "status", "string", "No Errors", "error status for device",true/*RO*/,true/*shown */,false/*not persisted */); //MANDATORY
        fields.emplace_back(String(idCounter++), "desiredTemperature", "float", "37", "desired Temperature",false,true);
        fields.emplace_back(String(idCounter++), "currentTemperature", "float", "-1", "current Temperature", false,true,false);
        fields.emplace_back(String(idCounter++), "currentHeaterOn", "bool", "0", "shows current heater state controlled by the device but overridden by HeaterDisabled ", true,true,false);
        fields.emplace_back(String(idCounter++), "Rpm", "float", "80", "desired RPM. You need to restart stepper to achieve this RPM.",false,true);
        fields.emplace_back(String(idCounter++), "desiredHeatingEndDurationInMinutes", "int", NOT_PRESENT, "desiredEndTime. Set an end time alarm in minutes like 60 for 1 hour from start or -1 for no alarm. You can separately reset the start time to now.",false,true);
        fields.emplace_back(String(idCounter++), "stepsPerRotation", "int", "200", "desired microstepping, only 200,400 ... 6400",false,true);
        fields.emplace_back(String(idCounter++), "heartBeat", "string", "-", "Should change all every time a new temp is read",true,true,false);//readonly isSHown, is not persisted
        // on offs read from UI and set on the board. They might be overridden by physical switches
        fields.emplace_back(String(idCounter++), "StepperOn", "bool", "0", "turns on off stepper",false,true,false);  //"1"
        fields.emplace_back(String(idCounter++), "FanOn", "bool", "1", "turns on off fan if capability exists",false,true);
        fields.emplace_back(String(idCounter++), "HeaterDisabled", "bool", "0", "disables heater even if current temp lower than desired temp",false,true);
        // pins
        fields.emplace_back(String(idCounter++), "TempSensorPin", "uint8_t", "19", "TempSensorPin", false);  // we use one main pin for either dh or OneWIre and that is pin GPIO19/LCD_MISO on pin 1 of Expansion 2 connector
        fields.emplace_back(String(idCounter++), "HeaterPwmPin", "uint8_t", "32", "Heater Pwm Pin", false);  // 32 is internally connected to spindle*/,
        fields.emplace_back(String(idCounter++), "StepperPwmStepPin", "uint8_t", String(22), "StepperPwmStepPin", false);
        // misc
        fields.emplace_back(String(idCounter++), "I2SoDataPin", "uint8_t", "21", "I2SoDataPin", false);
        fields.emplace_back(String(idCounter++), "I2SoClockPin", "uint8_t", "16", "I2SoClockPin", false);
        fields.emplace_back(String(idCounter++), "I2SoLatchPin", "uint8_t", "17", "I2SoLatchPin", false);
        fields.emplace_back(String(idCounter++), "SpeakerPin", "uint8_t", "25", "SpeakerPin", false);  // String(32) or String(12) The physical interface on the board is a 2-pin connector typically labeled BZ or Buzzer, with one pin providing the GPIO 25 signal and the other providing a GND (ground) connection.
        fields.emplace_back(String(idCounter++), "FanPin", "uint8_t", NOT_PRESENT, "FanPin", false);
        fields.emplace_back(String(idCounter++), "LedPin", "uint8_t", String(2), "LedPin", false);
        fields.emplace_back(String(idCounter++), "PotentiometerPin", "uint8_t", NOT_PRESENT, "PotentiometerPin", false);
        fields.emplace_back(String(idCounter++), "MemoryCsPin", "uint8_t", NOT_PRESENT, "MemoryCsPin", false);
        //
        fields.emplace_back(String(idCounter++), "maxHeaterDutyCycle", "int", "90", "maxHeaterDutyCycle", false, true);
        fields.emplace_back(String(idCounter++), "MKSBoard", "bool", "1", "MKSBoard", false, true);
        fields.emplace_back(String(idCounter++), "StepperOnOffSwitchInputPin", "int", NOT_PRESENT, "set to the pin to read the on-off-physical button if present ",false,true);  // 36 Input /*Sensor_VP SVP -*/,
        fields.emplace_back(String(idCounter++), "StepperOnOffSoftwareSwitchOutputPin", "uint8_t", NOT_PRESENT, "on my board we can control the on off by using ENABLEpin in output mode",false,true);//maybe 26
        //
        fields.emplace_back(String(idCounter++), "MostMusicOff", "bool", "0", "MostMusicOff", false, true);                            // turns off all music except for errors, warnings, time reached and first time desired temperature reached
        fields.emplace_back(String(idCounter++), "TemperatureReachedMusicOn", "bool", "1", "TemperatureReachedMusicOn", false, true);  // turns off all music except for errors, warnings, time reached and first time desired temperature reached
        //
        fields.emplace_back(String(idCounter++), "UseOneWireForTemperature", "bool", "1", "UseOneWireForTemperature", false, true);  // turns off all music except for errors, warnings, time reached and first time desired temperature reached
        // Preferences
        // fields.emplace_back(String(idCounter++), "preference_TimeDisplay", "bool", "1", "preference_TimeDisplay");
        // fields.emplace_back(String(idCounter++), "preference_TemperatureDisplay", "bool", "1", "preference_TemperatureDisplay");
        // fields.emplace_back(String(idCounter++), "preference_TemperatureReached_MusicOn", "bool", "1", "preference_TemperatureReached_MusicOn");
        //
        fields.emplace_back(String(idCounter++), "LowHumidityAlert", "bool", "0", "Alert if LowHumidity detected, works only for sensor DH..",false,true);
        //
        fields.emplace_back(String(idCounter++), "version", "string", "1.1", "Version", true, true);
    }
    static void initializeSample(std::vector<Field>& fields) {
        int idCounter = 0;
        // desired or set values
        fields.emplace_back(String(idCounter++), "a", "float", "37", "desired Temperature");
        fields.emplace_back(String(idCounter++), "b", "int", "0", "b");
        fields.emplace_back(String(idCounter++), "sample version", "string", "sample", "Version", true);
        // todo add currentStartTime
    }

    static String getTime() {  // works only if WiFi is on
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

// some potential probably for my custom pcb:
//  desired or set values AT this point for Tube rotator
// fields.emplace_back(String(idCounter++), "desiredTemperature", "float", "37", "desired Temperature");
// fields.emplace_back(String(idCounter++), "currentTemperature", "float", "-1", "current Temperature", true);
// fields.emplace_back(String(idCounter++), "Rpm", "float ", "300", "current and desired RPM");
// fields.emplace_back(String(idCounter++), "currentHeatingEndDurationInMinutes", "unsigned long ", NOT_PRESENT, "desiredEndTime. Please enter an end time for end time alarm in minutes e 60 for 1 hour from start. You can separately reset the start time to now.", false);
// // on offs
// fields.emplace_back(String(idCounter++), "StepperOn", "bool", "36   ", "turns on off stepper");  //"1"
// fields.emplace_back(String(idCounter++), "FanOn", "bool", "1", "turns on off fan if capability exists");
// fields.emplace_back(String(idCounter++), "HeaterOn", "bool", "1", "turns on off heater");
// // pins
// fields.emplace_back(String(idCounter++), "HeaterPwmPin", "uint8_t", NOT_PRESENT, "Heater Pwm Pin", false);
// fields.emplace_back(String(idCounter++), "StepperPwmStepPin", "uint8_t", String(25), "StepperPwmStepPin");
// fields.emplace_back(String(idCounter++), "StepperEnablePin", "uint8_t", String(26), "StepperEnablePin");
// fields.emplace_back(String(idCounter++), "I2SoClockPin", "uint8_t", NOT_PRESENT, "I2SoClockPin");
// fields.emplace_back(String(idCounter++), "I2SoLatchPin", "uint8_t", NOT_PRESENT, "I2SoLatchPin");
// fields.emplace_back(String(idCounter++), "LedPin", "uint8_t", String(2), "LedPin");
// fields.emplace_back(String(idCounter++), "PotentiometerPin", "uint8_t", NOT_PRESENT, "PotentiometerPin");
// fields.emplace_back(String(idCounter++), "TempSensorPin", "uint8_t", "19", "TempSensorPin");  // we use one main pin for either dh or OneWIre and that is pin GPIO19/LCD_MISO on pin 1 of Expansion 2 connector
// fields.emplace_back(String(idCounter++), "SpeakerPin", "uint8_t", String(32), "SpeakerPin");  // or String(12)
// fields.emplace_back(String(idCounter++), "SpeakerChannel", "uint8_t", String(4), "SpeakerChannel");
// fields.emplace_back(String(idCounter++), "FanPin", "uint8_t", NOT_PRESENT, "FanPin");
// fields.emplace_back(String(idCounter++), "FanPwmChannel", "uint8_t", NOT_PRESENT, "FanPwmChannel");
// fields.emplace_back(String(idCounter++), "MemoryCsPin", "uint8_t", NOT_PRESENT, "MemoryCsPin");
// // misc
// fields.emplace_back(String(idCounter++), "UseStepperOnOffSwitch", "bool", "0", "UseStepperOnOffSwitch");
// fields.emplace_back(String(idCounter++), "maxHeaterDutyCycle", "int", "90", "maxHeaterDutyCycle");
// fields.emplace_back(String(idCounter++), "MKSBoard", "bool", "0", "MKSBoard");
// fields.emplace_back(String(idCounter++), "UseStepperOnOffSwitch", "bool", "0", "UseStepperOnOffSwitch");