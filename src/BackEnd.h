// #pragma once
// #include <Arduino.h>

// #include "DHTesp.h"  //for DHT temp sensor
// // memory card https://www.mischianti.org/2021/03/28/how-to-use-sd-card-with-esp32-2/
// //  include the SD library:
// #include <DallasTemperature.h>
// #include <FS.h>
// #include <MyMusic.h>
// #include <OneWire.h>
// #include <SD.h>
// #include <SPI.h>
#pragma once
#include <Arduino.h>

#include "DHTesp.h"  //for DHT temp sensor
// memory card https://www.mischianti.org/2021/03/28/how-to-use-sd-card-with-esp32-2/
//  include the SD library:
#include <DallasTemperature.h>
#include <FS.h>
#include <MyMusic.h>
#include <OneWire.h>
#include <SD.h>
#include <SPI.h>

#include <cstdio>  // Required for snprintf
#include <vector>
//
#include "Config.h"
#include "Controller.h"
#include "Microstepping.h"
#include "MyMusic.h"
#include "TimeManager.h"
void startStepperIfNotStarted();
void stopStepperIfNotStopped();
void fanSetup();
void fan(bool on);
int readPotentiometer();
void heater(bool on, float duty = 0);
String getFormatedTimeSinceStart();
void desiredEndTimeCheck();
void setupI2SOShiftEnableMotor();
void setupI2SOShiftDisableMotor();
void createDir(fs::FS& fs, const char* path);
void writeFile(fs::FS& fs, const char* path, const String& message);
String formatTime(unsigned long time);
String getFormatedTimeSinceStart();
int readTurnOnStepperSwitch();
void processStepperStartOrStop();
void setupSDCard();
void printDirectory(File dir, int numTabs);
int getTemperature(float& temp, float& humid);
void writeData(byte* bits);
void processCommand();
double rpmToHertz(float rpm);
void setStepsPerRotation(int newStepsPerRotation);
void stepperSetup();
// end INCLUDES.h

// temperature sensor stuff
const float ModerateHeat_POWER = 0.9;
// Temperature either dh or oneWireDallas
// DHTesp dhTempSensor;  // used in setup and readTemperature
// OneWire oneWire(Controller::getI("TempSensorPin"));  // GPIO where the DS18B20 is connected to. Used to be GPIO36 but not anymore
// DallasTemperature tempSensor(&oneWire);
//
int lastTempHumidityReadTime = 0;         // never
unsigned long lastHumidityAlertTime = 0;  // never
float minHumidity = 60.;
bool firstTimeTurnOnHeater = true;
bool firstTimeReachDesiredTemperature = true;
float maxTemperature = 0;
// motor what we want for OS motor
// https://github.com/nenovmy/arduino/blob/master/leds_disco/leds_disco.ino
/* Setting all PWM motor, Heater PWM Properties */
// fan
const int FanFrequency = 40000;
float startTemperature = 0;
namespace Debug {
const bool LOG_HEATER_ON_OFF_STATE = true;
const bool LOG_FAN_ON_OFF_STATE = true;
const bool LOG_STEPPER_ON_OFF_SWITCH_STATE = true;
}  // namespace Debug
uint8_t HEATER_PWM_CHANNEL = 0;
uint8_t STEPPER_PWM_CHANNEL = 2;
uint8_t FAN_PWM_CHANNEL = 8;
uint8_t LED_BLUE_PWM_CHANNEL = 10;
// WeMos D1 esp8266: D8 as standard
const int chipSelect = SS;
unsigned long currentStartTime = millis();  // time since ESP power on in millis
const int32_t SPIfreq = 40000;
const int UNIVERSAL_PWM_RESOLUTION = 10;
const float UNIVERSAL_MAX_DUTY_CYCLE = (int)(pow(2, UNIVERSAL_PWM_RESOLUTION) - 1);
const float MODERATE_HEAT_POWER = 0.89;
const float HALF_DUTY_CYCLE = UNIVERSAL_MAX_DUTY_CYCLE / 2;
//
bool tempIsStepperOn = false;  // the first operation in setup is to stop it
//
enum Preference {  // for preferences
    TimeDisplay = true,
    TemperatureDisplay = true,
    TemperatureReached_MusicOn = true,
};

DHTesp dhTempSensor;          // used in setup and readTemperature
OneWire oneWire = OneWire();  // GPIO where the DS18B20 is connected to. Used to be GPIO36 but not anymore
DallasTemperature tempSensor = DallasTemperature();
class BackEnd {
   public:
    static inline unsigned long lastModelUpdateInSeconds = 0;

    static void setupBackend() {
        Controller::log("setupBackend begin ~~~~~~~");
        MyMusic::play(MyMusic::backend);
        tempSensor.setOneWire(&oneWire);
        stepperSetup();
        //  Declare pins as output:
        pinMode(Controller::getI("LedPwmPin"), OUTPUT);
        ledcSetup(LED_BLUE_PWM_CHANNEL, 40000, UNIVERSAL_PWM_RESOLUTION);  // normal PWM frrequency for MKS is 5000HZ
        delay(20);
        ledcAttachPin(Controller::getI("LedPwmPin"), LED_BLUE_PWM_CHANNEL); /* Attach the Pin PWM Channel to the GPIO Pin */
        delay(20);
        pinMode(Controller::getI("StepperPwmStepPin"), OUTPUT);
        pinMode(Controller::getI("I2SoLatchPin"), OUTPUT);
        pinMode(Controller::getI("I2SoClockPin"), OUTPUT);
        pinMode(Controller::getI("I2SoDataPin"), OUTPUT);
        pinMode(Controller::getI("FanPin"), OUTPUT);
        // //   speaker
        if (Controller::getPresent("SpeakerPin")) {
            ledcSetup(SPEAKER_CHANNEL, 5000, 8);
            ledcAttachPin(Controller::getI("SpeakerPin"), SPEAKER_CHANNEL);
            ledcWrite(SPEAKER_CHANNEL, 0);  // duty Cycle = 0 turns OFF
            Controller::infoAlarm("speaker present");
        }
        // temperature sensor
        if (Controller::getPresent("TempSensorPin")) {
            if (!Controller::getI("UseOneWireForTemperature")) {
                dhTempSensor.setup(Controller::getI("TempSensorPin"), DHTesp::DHT22);
                if (dhTempSensor.getStatus() == DHTesp::ERROR_TIMEOUT) {
                    Controller::warningAlarm("DHTesp::ERROR_TIMEOUT");
                    Controller::log("No DHT22 found or not working properly!");
                }
            } else {
                oneWire.begin(Controller::getI("TempSensorPin"));
                tempSensor.begin();
            }
            Controller::log("Temp sensor initiated");
        } else {
            Controller::log("No temperature sensor pin defined!");
        }
        // heater
        if (Controller::getPresent("HeaterPwmPin")) {
            pinMode(Controller::getI("HeaterPwmPin"), OUTPUT);
            // setup the heater
            ledcSetup(HEATER_PWM_CHANNEL, 40000, UNIVERSAL_PWM_RESOLUTION);  // normal PWM frrequency for MKS is 5000HZ
            delay(20);
            ledcAttachPin(Controller::getI("HeaterPwmPin"), HEATER_PWM_CHANNEL); /* Attach the Pin PWM Channel to the GPIO Pin */
            delay(20);
            Controller::log("Initial disabling of the heater in setup");
            heater(false);
            Controller::setBool("currentHeaterOn", false);
        } else {
            Controller::infoAlarm("No heater pin defined!");
        }
        if (Controller::getPresent("FanPin")) {
            fanSetup();
        } else {
            Controller::log("No fan for this device");
        }
        // setupSDCard();
        Controller::log("maxHeaterDutyCycle %i as percentage\n", Controller::getI("maxHeaterDutyCycle"));
        float temperature;
        float humidity;
        if (Controller::getI("desiredTemperature") < 36) {
            MyMusic::play(MyMusic::tempUnder37);
        } else {
            MyMusic::play(MyMusic::temp37);
        }
        delay(300);
        if (Controller::getPresent("TempSensorPin")) {
            getTemperature(temperature, humidity);
            startTemperature = temperature;
            Controller::log("startTemperature:%.2f, startHumidity %.2f", startTemperature, humidity);
        }
        // stepper
        Controller::log("stepper desiredRPM:%d", Controller::getI("Rpm"));
        Controller::setBool("StepperOn", true);  // the Stepper on is never saved so we manually initialize it
        // Controller::log("Initial disabling of the stepper in setup");
        //  stopStepperIfNotStopped();  // just to init the multiplexing pins
        //  delay(300);
        //  processStepperStartOrStop();
        MyMusic::play(MyMusic::backendend);
        Controller::log("==============================   END Backend Setup. Version:%s ==============================", Controller::getS("version"));
    }
    /////////////
    static inline bool first = true;
    static inline float lastReadTemp = -1;
    static inline bool TEMPERATURE_DISPLAY = true;
    //////
    static inline int i = 0;
    static void loopBackend() {
        if (first) {
            Controller::log("[SYS] loopBackend Started   -----------------------------------------");
            first = false;
        }
        processStepperStartOrStop();
        // Get temperature
        float temperature;
        float humidity;
        char tempCharBuffer[16];  // reused// A size of 16 is often safe for most floats.
        float dT = Controller::getI("desiredTemperature");
        if (TEMPERATURE_DISPLAY) {
            static char buffer[15];
            if (!Controller::getBool("UseOneWireForTemperature")) {
                snprintf(buffer, sizeof(buffer), "humidity=%.2f", humidity);
            } else {
                buffer[0] = '\0';  // ""
            }
            // Controller::log(" Current temp:%.2f, DesiredTemperature:%.2f, max temperature: %.2f, %s\n", temperature, dT, maxTemperature, buffer);
        }
        if (temperature < dT) {  // todo update the heater on off  faster
            if (!Controller::getBool("currentHeaterOn")) {
                Controller::log("Turning heater ON");
                if (firstTimeTurnOnHeater) {
                    firstTimeTurnOnHeater = false;
                }
                if (dT - temperature >= 2) {                                                                // high temp difference
                    heater(true, UNIVERSAL_MAX_DUTY_CYCLE * Controller::getI("maxHeaterDutyCycle") / 100);  // Heater start
                } else {
                    heater(true, UNIVERSAL_MAX_DUTY_CYCLE * MODERATE_HEAT_POWER);
                }
            }
            // else{
            //  do nothing let it heat
            // }
        } else {  // temp is high enough no need to heat
            if (Controller::getBool("currentHeaterOn")) {
                if (firstTimeReachDesiredTemperature) {
                    if (Controller::getBool("TemperatureReachedMusicOn")) {
                        Controller::infoAlarm("firstTimeReachDesiredTemperature");
                    }
                    firstTimeReachDesiredTemperature = false;
                }
                heater(false);  // second arg is ignored when heater is turned off
                Controller::setBool("currentHeaterOn", false);
            }
        }
        if (!Controller::getI("UseOneWireForTemperature") && humidity < minHumidity && ((millis() - lastHumidityAlertTime) / 1000) > 200 /* about 3 minutes*/) {
            unsigned long nowTime = millis();
            if (nowTime > lastHumidityAlertTime + 30000) {  // every half hour
                Controller::warningAlarm("Humidity dropped to less then the minimal humidity of 60% hardcoded value");
                lastHumidityAlertTime = nowTime;
            }
        }
        Controller::setNoLog("time", TimeManager::getCurrentTimeAsString());
        int r = TimeManager::checkIfHeatingDateTimeWasReached(Controller::getS("desiredHeatingEndTime").c_str());
        if (r == 1) {  // 0 means not yet, -1 means not set or errors
            Controller::infoAlarm("HeatingDateTimeWasReached reached", MyMusic::processFinished);
            if (Controller::getS("alarmTurnsHeatingOff")) {
                heater(false);
            }
        }
        Controller::webSocket.textAll(Controller::model.toJsonString());
    }
};
//
// loose functions
int calculateFrequency() {
    int freq = (int)(Controller::getI("Rpm") / 60 * Controller::getI("stepsPerRotation"));
    return freq;
}

void startStepperIfNotStarted() {
    if (tempIsStepperOn) {
        // Serial.println("!!!! stepper already on");
        return;  // already on
    }
    Controller::log("Attempt to start the stepper");
    if (Controller::getI("MKSBoard")) {
        setupI2SOShiftEnableMotor();
    } else {                                                                         // my board
        digitalWrite(Controller::getI("StepperOnOffSoftwareSwitchOutputPin"), LOW);  // enable
    }
    for (int rpm = (Controller::getI("Rpm") > 80 ? 80 : Controller::getI("Rpm")); rpm <= Controller::getI("Rpm"); rpm += 10) {
        if (!Controller::getI("MKSBoard")) setStepsPerRotation(Controller::getI("stepsPerRotation"));
        double f = rpmToHertz(rpm);
        Controller::log("START STEPPER with frequency:%d and RPM:%d", f, rpm);
        // delay(5);
        ledcSetup(STEPPER_PWM_CHANNEL, f, UNIVERSAL_PWM_RESOLUTION);
        // delay(5);
        ledcAttachPin(Controller::getI("StepperPwmStepPin"), STEPPER_PWM_CHANNEL); /* Attach the StepPin PWM Channel to the GPIO Pin */
        // delay(5);
        ledcWrite(STEPPER_PWM_CHANNEL, HALF_DUTY_CYCLE);
        delay(500);
        // Controller::setBool("StepperOn", true);
    }
    // Controller::setBool("StepperOn", 1);
    tempIsStepperOn = true;
}
//
void stopStepperIfNotStopped() {
    if (!tempIsStepperOn) {
        // Serial.println("!!!! stepper already off");
        return;  // already on
    }
    Controller::log("Stop stepper");
    if (Controller::getI("MKSBoard")) {
        setupI2SOShiftDisableMotor();
    } else {                                                                          // custom PCB
        digitalWrite(Controller::getI("StepperOnOffSoftwareSwitchOutputPin"), HIGH);  // dis-engage break
        delay(10);
    }
    ledcDetachPin(Controller::getI("StepperPwmStepPin")); /* Detach the StepPin PWM Channel from the GPIO Pin */
    tempIsStepperOn = false;
    delay(100);
}
//
double rpmToHertz(float rpm) {
    return (int)(rpm / 60 * Controller::getI("stepsPerRotation"));  // in hertz
}
//
void stepperSetup() {
    if (Controller::getI("MKSBoard")) {
        setupI2SOShiftDisableMotor();
    } else {
        if (Controller::getPresent("StepperOnOffSoftwareSwitchOutputPin")) {
            pinMode(Controller::getI("StepperOnOffSoftwareSwitchOutputPin"), OUTPUT);
            digitalWrite(Controller::getI("StepperOnOffSoftwareSwitchOutputPin"), LOW);  // engage
        }
        pinMode(Controller::getI("StepperStepsPerRotationM0Pin"), OUTPUT);
        pinMode(Controller::getI("StepperStepsPerRotationM1Pin"), OUTPUT);
        pinMode(Controller::getI("StepperStepsPerRotationM2Pin"), OUTPUT);
        setStepsPerRotation(Controller::getI("stepsPerRotation"));
    }
    ledcDetachPin(Controller::getI("StepperPwmStepPin")); /* Detach the StepPin PWM Channel from the GPIO Pin */
    if (Controller::getPresent("StepperOnOffSwitchInputPin")) pinMode(Controller::getI("StepperOnOffSwitchInputPin"), INPUT);
    tempIsStepperOn = false;
    delay(200);
}
//
int getTemperature(float& temp, float& humid) {
    if (Controller::getBool("UseOneWireForTemperature")) {
        tempSensor.requestTemperatures();
        // Serial.print("Temperature: ");  // print the temperature in Celsius
        temp = tempSensor.getTempCByIndex(0);
        // Serial.print("In getTemperature temp read is:");
        // Serial.println(temp);
    } else {
        // Reading temperature for humidity takes about 250 milliseconds!. Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
        TempAndHumidity newValues = dhTempSensor.getTempAndHumidity();
        // Check if any reads failed and exit early (to try again).
        if (dhTempSensor.getStatus() != 0) {
            // const char* bu[300];  // todo+ String(dhTempSensor.getStatusString())
            Controller::errorAlarm(" DHT12 error status, probably skipped on temp read");
            return false;
        }
        temp = newValues.temperature;
        if (temp < 0 || temp > 70) {
            for (int i = 0; i < 10; i++) {
                Controller::errorAlarm("temperature out of range 0-70");
            }
        }
        humid = newValues.humidity;
    }
    return true;
}

// current https://reprap.org/wiki/NEMA_17_Stepper_motor 17hs16 20044s1(black ones) rated for 2 A
// adjustment guide https://lastminuteengineers.com/drv8825-stepper-motor-driver-arduino-tutorial/ Vref on Pot = max current /2 =1V
// this is probably the multiplexing of output pins
// xDir is 2 xStep is 1 and beeper is 7
// it seems to me that the signal from Probe is sent to the Controller::getI("StepperPwmStepPin") but for enable we might? use the multiplexor
void setupI2SOShiftEnableMotor() {
    // Serial.println("#################### setupI2SOShift   EnableMotor #####################################");
    byte bits[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  // enable is pin 0 and should be 0 to start stepper, last  is beeper
    writeData(bits);
    delay(100);
}
//
void setupI2SOShiftDisableMotor() {
    // Serial.println("#################### setupI2SOShift    DisableMotor #####################################");
    byte bits[16] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0};  // enable is pin 0 and should be high to stop stepper, last one is the beeper
    writeData(bits);
    delay(10);
}
//
void writeData(byte* bits) {
    // alternate data we can just capture the 2 data values like 2254, 2124
    int data1 = 0;
    for (int i = 0; i < 8; i++) {
        data1 |= bits[i + 8] << i;
    }
    int data2 = 0;
    for (int i = 0; i < 8; i++) {
        data2 |= bits[i] << i;
    }
    // disable update
    digitalWrite(Controller::getI("I2SoLatchPin"), LOW);
    // shift out the data (second shift register must be send first)
    shiftOut(Controller::getI("I2SoDataPin"), Controller::getI("I2SoClockPin"), MSBFIRST, data1);  // NOT NEEDED FOR X axes
                                                                                                   // Serial.print("data2");Serial.println(data2);
    shiftOut(Controller::getI("I2SoDataPin"), Controller::getI("I2SoClockPin"), MSBFIRST, data2);  //*
                                                                                                   // update the shift register output pins
    digitalWrite(Controller::getI("I2SoLatchPin"), HIGH);
}
// Function for reading the Potentiometer
int readPotentiometer() {
    uint16_t customDelay = analogRead(Controller::getI("PotentiometerPin"));  // Reads the potentiometer
    int newRPM = map(customDelay, 0, 1023, 0, 300);                           // read values of the potentiometer from 0 to 1023 into  d0->300
    return 300;
}

void setupSDCard() {
    Serial.print("\nInitializing SD card...");
    SPIClass hspi = SPIClass(HSPI);                                             // HSPI has the 12-15 pins already configured // actually a reference
    if (!SD.begin(Controller::getI("MemoryCsPin"), hspi, SPIfreq, "/sd", 2)) {  // copied from Fluid SDCard.cpp //if (SD.begin(csPin, SPI, SPIfreq, "/sd", 2)) {
        Serial.println("initialization failed. Things to check:");
        Serial.println("* is a card inserted?");
        while (1);
    } else {
        Serial.println("A card is present.");
    }
    // print the type of card
    Serial.println();
    Serial.print("Card type:    ");
    switch (SD.cardType()) {
        case CARD_NONE:
            Serial.println("NONE");
            break;
        case CARD_MMC:
            Serial.println("MMC");
            break;
        case CARD_SD:
            Serial.println("SD");
            break;
        case CARD_SDHC:
            Serial.println("SDHC");  // for me it prints this!
            break;
        default:
            Serial.println("Unknown");
    }
    // print the type and size of the first FAT-type volume
    //  uint32_t volume size;
    //  Serial.print("Volume type is:    FAT");
    //  Serial.println(SDFS.usefatType(), DEC);
    Serial.print("Card size:  ");
    Serial.println((float)SD.cardSize() / 1000);
    Serial.print("Total bytes: ");
    Serial.println(SD.totalBytes());
    Serial.print("Used bytes: ");
    Serial.println(SD.usedBytes());
    File dir = SD.open("/", FILE_READ);
    createDir(SD, "/adriandir");
    printDirectory(dir, 0);
    writeFile(SD, "/lastWrittenTime.txt", formatTime(millis()));
    writeFile(SD, "/hello.txt", "  Hello");
    Serial.println("after writeFile and adrianDir subdirectory ListDir");
}
//
void printDirectory(File dir, int numTabs) {
    while (true) {
        File entry = dir.openNextFile();
        if (!entry) {  // no more files
            break;
        }
        for (uint8_t i = 0; i < numTabs; i++) {
            Serial.print('\t');
        }
        Serial.print(entry.name());
        if (entry.isDirectory()) {
            Serial.println("/");
            printDirectory(entry, numTabs + 1);
        } else {
            // files have sizes, directories do not
            Serial.print("\t\t");
            Serial.print(entry.size(), DEC);
            time_t lw = entry.getLastWrite();
            struct tm* tmstruct = localtime(&lw);
            Serial.printf("\tLAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
        }
        entry.close();
    }
}
//
void writeFile(fs::FS& fs, const char* path, const String& message) {
    Serial.printf("Writing file: %s\n", path);
    File file = fs.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }
    if (file.print(message)) {
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}
//
// de-bounce variables
const int DebounceTime = 200;
unsigned long lastDebounceTime = 0;
int lastSteadyState = LOW;        // the previous steady state from the input pin
int lastFlickerableState = -100;  // the previous flicker-able state from the input pin
int lastStepperOnOffSwitchState;
/** returns 0 for off and 1 for on*/
int readTurnOnStepperSwitch() {
    lastStepperOnOffSwitchState = !digitalRead(Controller::getI("StepperOnOffSwitchInputPin"));
    // lastStepperOnOffSwitchState = !digitalRead(36); //todo
    int ret = -1;  // means unchanged
    // check to see if you just pressed the button  // (i.e. the input went from LOW to HIGH), and you've waited long enough
    // since the last press to ignore any noise:    // If the switch/button changed, due to noise or pressing:
    if (lastStepperOnOffSwitchState != lastFlickerableState) {
        lastDebounceTime = millis();                         // reset the debouncing timer
        lastFlickerableState = lastStepperOnOffSwitchState;  // save the the last flickerable state
    }
    if ((millis() - lastDebounceTime) > DebounceTime) {
        // whatever the reading is at, it's been there for longer than the debounce        // delay, so take it as the actual current state:
        // if the button state has changed:
        if (lastSteadyState == HIGH && lastStepperOnOffSwitchState == LOW) {
            if (Debug::LOG_STEPPER_ON_OFF_SWITCH_STATE) {
                Controller::log("The stepper motor switch was turned off");
            }
            ret = 0;
        } else if (lastSteadyState == LOW && lastStepperOnOffSwitchState == HIGH) {
            if (Debug::LOG_STEPPER_ON_OFF_SWITCH_STATE) {
                Controller::log("The stepper motor switch was turned on");
            }
            ret = 1;
        }
        // save the the last steady state
        lastSteadyState = lastStepperOnOffSwitchState;
    }
    if (ret == -1) {
        ret = lastSteadyState;
    }
    return ret;
}
//
void fanSetup() {
    pinMode(Controller::getI("FanPin"), OUTPUT);
    fan(true);
    delay(2000);
    fan(false);
}
//
void fan(bool on) {
    if (Controller::getI("FanOn") != on) {
        Controller::setBool("FanOn", on);
    }
    if (on) {
        ledcAttachPin(Controller::getI("FanPin"), FAN_PWM_CHANNEL);
        ledcWrite(FAN_PWM_CHANNEL, (int)UNIVERSAL_MAX_DUTY_CYCLE);
    } else {
        ledcDetachPin(Controller::getI("FanPin"));
        ledcWrite(FAN_PWM_CHANNEL, 0);
    }
    if (Debug::LOG_FAN_ON_OFF_STATE) {
        Serial.print("Fan set to: ");
        Serial.println(on);
    }
}
//
void heater(bool on, float duty) {
    // this works only after setup was called to initialize the channel
    if (on) {
        ledcWrite(HEATER_PWM_CHANNEL, duty);  // Heater start
        if (!Controller::getBool("currentHeaterOn"))
            Controller::setBool("currentHeaterOn", true);
    } else {
        ledcWrite(HEATER_PWM_CHANNEL, 0);  // Heater stop
        if (Controller::getBool("currentHeaterOn"))
            Controller::setBool("currentHeaterOn", false);
    }
    if (Debug::LOG_HEATER_ON_OFF_STATE) {
        Controller::log("Heater set to: ");
        Controller::log(on == 0 ? "0" : "1");
        if (on) {
            Controller::log(" with duty at:%.2f%%", ((duty / UNIVERSAL_MAX_DUTY_CYCLE) * 100));
        }
    }
}
//
/** time is in seconds */
String formatTime(unsigned long time) {
    String result = "";
    int hours = (unsigned long)(time / 3600);
    if (hours > 0) {
        result = +hours;
        result += ("h ");
    }
    result += ((unsigned long)(time % 3600) / 60);
    result += ("m ");
    result += (time % 60);
    result += ("s");
    return result;
}
//
void processStepperStartOrStop() {
    bool desired = Controller::getBool("StepperOn");
    if (Controller::getPresent("StepperOnOffSwitchInputPin")) {  // it's an end with the motor on signal so both the physical and soft on for motor to run
        int stepperHardwareSwitchOnOffPosition = readTurnOnStepperSwitch();
        if (stepperHardwareSwitchOnOffPosition == 0) {
            Controller::set("currentStepperOnOffSwitchPosition", "0");
        } else {
            Controller::set("currentStepperOnOffSwitchPosition", "1");
        }
        // Serial.printf("stepperHardwareSwitchOnOffPosition:%d, tempIsStepperOn:%d,desired:%d\n", stepperHardwareSwitchOnOffPosition, tempIsStepperOn, desired);
        if (stepperHardwareSwitchOnOffPosition && desired) {
            startStepperIfNotStarted();
            if (Controller::getPresent("FanPin")) fan(false);
        } else {
            stopStepperIfNotStopped();
            if (Controller::getPresent("FanPin")) fan(true);
        }
    } else {  // NO SWITCH present
        if (desired) {
            startStepperIfNotStarted();
            if (Controller::getPresent("FanPin")) fan(false);
        } else {
            if (!desired) {
                stopStepperIfNotStopped();
                if (Controller::getPresent("FanPin")) fan(true);
            }
        }
    }
}
//
void setStepsPerRotation(int newStepsPerRotation) {
    switch (newStepsPerRotation) {
        case STEPS200:
            digitalWrite(Controller::getI("StepperStepsPerRotationM0Pin"), LOW);
            digitalWrite(Controller::getI("StepperStepsPerRotationM1Pin"), LOW);
            digitalWrite(Controller::getI("StepperStepsPerRotationM2Pin"), LOW);
            break;
        case STEPS400:
            digitalWrite(Controller::getI("StepperStepsPerRotationM0Pin"), HIGH);
            digitalWrite(Controller::getI("StepperStepsPerRotationM1Pin"), LOW);
            digitalWrite(Controller::getI("StepperStepsPerRotationM2Pin"), LOW);
            break;
        case STEPS800:
            digitalWrite(Controller::getI("StepperStepsPerRotationM0Pin"), LOW);
            digitalWrite(Controller::getI("StepperStepsPerRotationM1Pin"), HIGH);
            digitalWrite(Controller::getI("StepperStepsPerRotationM2Pin"), LOW);
            break;
        case STEPS1600:
            digitalWrite(Controller::getI("StepperStepsPerRotationM0Pin"), HIGH);
            digitalWrite(Controller::getI("StepperStepsPerRotationM1Pin"), HIGH);
            digitalWrite(Controller::getI("StepperStepsPerRotationM2Pin"), LOW);
            break;
        case STEPS3200:
            digitalWrite(Controller::getI("StepperStepsPerRotationM0Pin"), LOW);
            digitalWrite(Controller::getI("StepperStepsPerRotationM1Pin"), LOW);
            digitalWrite(Controller::getI("StepperStepsPerRotationM2Pin"), HIGH);
            break;
        case STEPS6400:
            digitalWrite(Controller::getI("StepperStepsPerRotationM0Pin"), HIGH);
            digitalWrite(Controller::getI("StepperStepsPerRotationM1Pin"), HIGH);
            digitalWrite(Controller::getI("StepperStepsPerRotationM2Pin"), HIGH);
            break;
        default:
            Controller::log("BAD micro-stepping. Only 200,400,800,1600,3200,6400 allowed");
            return;
    }
}

//
//
// String getFormatedTimeSinceStart() {
//     unsigned long time = (unsigned long)((millis() - currentStartTime) / 1000);  // finds the time since last print in secs
//     return formatTime(time);
// }
//
// void desiredEndTimeCheck() {  // returns time to alarm
//     int det = Controller::getI("desiredHeatingEndDurationInMinutes");
//     if (det == -1) {
//         return;
//     }
//     unsigned int elapsedTimeInMilliSecs = (millis() - currentStartTime);
//     unsigned int timeToAlarmInSec = det * 60 - (unsigned long)(elapsedTimeInMilliSecs / 1000);
//     Controller::set("timeToAlarmInSec", String(timeToAlarmInSec));
//     if (det != -1 && det * 60 * 1000 <= elapsedTimeInMilliSecs) {
//         Serial.println("Desired end time " + String(det) + " reached. Current elapsed time: " + getFormatedTimeSinceStart());
//         MyMusic::play(scaleLouder, true);
//     }
// }

//
void createDir(fs::FS& fs, const char* path) {
    Serial.printf("Creating Dir: %s\n", path);
    if (fs.mkdir(path)) {
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}
//
// sets the electrical signals for the microswitches half of RPM in x2 stepsPerRotation and then full in no stepsPerRotation
