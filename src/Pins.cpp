#include "Pins.h"
#include "Arduino.h"

Pins::Pins() {
    HeaterPin =NOT_PRESENT;
    HeaterPwmChannel =NOT_PRESENT;
    StepperPwmStepPin =NOT_PRESENT;
    StepperPwmChannel =NOT_PRESENT;
    StepperEnablePin =NOT_PRESENT;
    I2SoClockPin =NOT_PRESENT;
    I2SoLatchPin =NOT_PRESENT;
    LedPin =NOT_PRESENT;
    PotentiometerPin =NOT_PRESENT;
    TempSensorPin =NOT_PRESENT;
    SpeakerPin =NOT_PRESENT;
    SpeakerChannel =NOT_PRESENT;
    FanPin =NOT_PRESENT;
    FanPwmChannel =NOT_PRESENT;
    MemoryCsPin =NOT_PRESENT;
    SpindleEnablePin =255;  // do we need this? Yes, it would be stepperOnOffEnablePin
    UseOneWireForTemperature = false;
    MKSBoard = true;
    //
    desiredTemperature = 37;  // seed it todo change back to 36.5
    currentRPM = 300; 
    currentHeatingEndDurationInMinutes=-1;
    //
    currentStepsPerRotation = NOT_PRESENT;
    maxHeaterDutyCyclePercentage = 80;
    currentStartTime = millis();
    VERSION = "RotationalIncubatedDevice 1.0";
}


