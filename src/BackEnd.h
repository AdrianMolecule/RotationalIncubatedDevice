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
#include "Controller.h"
#include "Helper.h"
#include "Microstepping.h"
#include "MyMusic.h"
#include "TimeManager.h"

void startStepper();
void stopStepper();
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
int readTurnOnStepperButton();
void processStepperStartOrStop();
void setupSDCard();
void printDirectory(File dir, int numTabs);
int getTemperature(float& temp, float& humid);
void writeData(byte* bits);
void processCommand();
double rpmToHertz(float rpm);
void setStepsPerRotation(int newStepsPerRotation);
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
int currentStepsPerRotation = 200;          // TODO unused
unsigned long currentStartTime = millis();  // time since ESP power on in millis
const int32_t SPIfreq = 40000;
const int UNIVERSAL_PWM_RESOLUTION = 10;
const float UNIVERSAL_MAX_DUTY_CYCLE = (int)(pow(2, UNIVERSAL_PWM_RESOLUTION) - 1);
const float MODERATE_HEAT_POWER = 0.89;
const float HALF_DUTY_CYCLE = UNIVERSAL_MAX_DUTY_CYCLE / 2;
//
bool tempIsStepperOn = false;  // TODO might need to be true because the first operation is to stop it
//
int lastStepperOnOffButtonState = LOW;  // this is use for debouncing the previous steady state from the input pin
int lastFlickerableState = -100;
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
        Serial.println("setupBackend begin ~~~~~~~");
        tempSensor.setOneWire(&oneWire);
        if (Controller::getPresent("StepperOnOffSwitchInputPin"))
            pinMode(Controller::getI("StepperOnOffSwitchInputPin"), INPUT);
        if (Controller::getI("MKSBoard")) {
            Serial.println("Using MKS DLC32 v2.1 board specific setup");
        } else {  // this is ALL needed for my custom PCB
            if (Controller::getPresent("StepperOnOffSoftwareSwitchOutputPin")) {
                pinMode(Controller::getI("StepperOnOffSoftwareSwitchOutputPin"), OUTPUT);
                digitalWrite(Controller::getI("StepperOnOffSoftwareSwitchOutputPin"), LOW);
            }
            // pinMode(STEPPER_stepsPerRotation_M0, OUTPUT);
            // pinMode(STEPPER_stepsPerRotation_M1, OUTPUT);
            // pinMode(STEPPER_stepsPerRotation_M2, OUTPUT);
            // setStepsPerRotation(currentStepsPerRotation);
        }  // end just for the pcb
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
        // pinMode(Controller::getI("FanPin"), OUTPUT);
        // // pinMode(Controller::getI("StepperEnablePin"), INPUT);
        // pinMode(Controller::getI("SpeakerPin"), OUTPUT);
        // // alternate(LedPwmPin, 50, 5);
        // //   speaker
        ledcSetup(SPEAKER_CHANNEL, 5000, 8);
        ledcAttachPin(Controller::getI("SpeakerPin"), SPEAKER_CHANNEL);
        ledcWrite(SPEAKER_CHANNEL, 0);  // duty Cycle = 0
        MyMusic::play(validChoice);
        // temperature sensor
        if (Controller::getPresent("TempSensorPin")) {
            if (!Controller::getI("UseOneWireForTemperature")) {
                dhTempSensor.setup(Controller::getI("TempSensorPin"), DHTesp::DHT22);
                if (dhTempSensor.getStatus() == DHTesp::ERROR_TIMEOUT) {
                    MyMusic::play(darthVader, true);
                    Serial.println("No DHT22 found or not working properly!");
                }
            } else {
                oneWire.begin(Controller::getI("TempSensorPin"));
                tempSensor.begin();
            }
            Serial.println("Temp sensor initiated");
        } else {
            Serial.println("No temperature sensor pin defined!");
        }
        // heater
        if (Controller::getPresent("HeaterPwmPin")) {
            pinMode(Controller::getI("HeaterPwmPin"), OUTPUT);
            // setup the heater
            ledcSetup(HEATER_PWM_CHANNEL, 40000, UNIVERSAL_PWM_RESOLUTION);  // normal PWM frrequency for MKS is 5000HZ
            delay(20);
            ledcAttachPin(Controller::getI("HeaterPwmPin"), HEATER_PWM_CHANNEL); /* Attach the Pin PWM Channel to the GPIO Pin */
            delay(20);
            Serial.print("stepper desiredRPM:");
            Serial.println(Controller::getI("Rpm"));
            // motor
            Serial.println("Initial disabling of the stepper in setup");
            stopStepper();
            Serial.println("Initial disabling of the heater in setup");
            heater(false);
            Controller::setBool("currentHeaterOn", false);
            MyMusic::play(scaleLouder);
        } else {
            Serial.println("No heater pin defined!");
        }
        if (Controller::getPresent("FanPin")) {
            fanSetup();
        } else {
            Serial.println("No fan for this device");
        }
        // setupSDCard();
        Serial.printf("maxHeaterDutyCycle %i as percentage\n", Controller::getI("maxHeaterDutyCycle"));
        float temperature;
        float humidity;
        if (Controller::getI("desiredTemperature") == 30) {
            MyMusic::play(temp30);
        } else if (Controller::getI("desiredTemperature") == 37) {
            MyMusic::play(temp37);
        }
        delay(300);
        startStepper();
        if (Controller::getPresent("TempSensorPin")) {
            getTemperature(temperature, humidity);
            startTemperature = temperature;
            Serial.println("startTemperature:" + String(startTemperature) + " startHumidity:" + String(humidity));
        }
        Serial.println("=================================END Backend Setup. Version:" + Controller::getS("version") + "================================");
    }
    /////////////
    static inline bool first = true;
    static inline float lastReadTemp = -1;
    static inline bool TEMPERATURE_DISPLAY = true;
    static inline bool TIME_DISPLAY = true;
    //////
    static void loopBackend() {
        if (first) {
            Serial.println("[SYS] loopBackend Started.");
            first = false;
        }
        processStepperStartOrStop();
        // Get temperature
        float temperature;
        float humidity;
        char tempCharBuffer[16];  // reused// A size of 16 is often safe for most floats.
        float dT = Controller::getI("desiredTemperature");
        int ledBlueDuty = dT > 36 ? UNIVERSAL_MAX_DUTY_CYCLE / 10 : UNIVERSAL_MAX_DUTY_CYCLE;
        ledcWrite(LED_BLUE_PWM_CHANNEL, ledBlueDuty);              // BLUE LED start
        if (((millis() - lastTempHumidityReadTime) / 1000) > 1) {  // every 1 seconds
            getTemperature(temperature, humidity);
            if (lastReadTemp != temperature) {  // avoid setting values that did not change
                std::snprintf(tempCharBuffer, sizeof(tempCharBuffer), /* Maximum bytes to write*/ "%.2f", temperature);
                Controller::set("currentTemperature", tempCharBuffer);
                // Controller::webSocket.textAll(Controller::model.toJsonString());//immediate UI update
                lastReadTemp = temperature;
            }
            lastTempHumidityReadTime = millis();
            if (temperature > maxTemperature) {  // todo
                maxTemperature = temperature;
            }
            if (TEMPERATURE_DISPLAY) {
                Serial.printf("Current temp: %.2f, %s", temperature, !Controller::getBool("UseOneWireForTemperature") ? Serial.printf("Humidity:%.2f ,", humidity), "" : "");
                Serial.printf("DesiredTemperature:%.2f max temperature: %.2f\n", dT, maxTemperature);
                // if (TIME_DISPLAY) {
                //     Serial.println(" Time since start: " + getFormatedTimeSinceStart() + " ");
                // } else {
                //     Serial.println(" ");
                // }
            }
            if (temperature < dT) {  // todo update the heater on off  faster
                if (!Controller::getBool("currentHeaterOn")) {
                    Serial.println("Turning heater ON");
                    if (firstTimeTurnOnHeater) {
                        Serial.println("First time heater ON");
                        MyMusic::play(auClairDeLaLune);
                        firstTimeTurnOnHeater = false;
                    }
                    if (dT - temperature >= 2) {                                                                // high temp difference
                        heater(true, UNIVERSAL_MAX_DUTY_CYCLE * Controller::getI("maxHeaterDutyCycle") / 100);  // Heater start
                    } else {
                        heater(true, UNIVERSAL_MAX_DUTY_CYCLE * MODERATE_HEAT_POWER);
                    }
                    Controller::setBool("currentHeaterOn", true);
                }
                // else{
                //  do nothing let it heat
                // }
            } else {  // temp is high enough no need to heat
                if (Controller::getBool("currentHeaterOn")) {
                    if (firstTimeReachDesiredTemperature) {
                        if (Controller::getBool("TemperatureReachedMusicOn")) {
                            MyMusic::play(frereJacquesFull, true);
                        }
                        firstTimeReachDesiredTemperature = false;
                    }
                    heater(false);  // second arg is ignored when heater is turned off
                    Controller::setBool("currentHeaterOn", false);
                }
            }
            if (!Controller::getI("UseOneWireForTemperature") && humidity < minHumidity && ((millis() - lastHumidityAlertTime) / 1000) > 200 /* about 3 minutes*/) {
                Serial.println("WARNING !!! humidity dropped to less then minimal humidity");
                unsigned long nowTime = millis();
                if (nowTime > lastHumidityAlertTime + 2000) {
                    lastHumidityAlertTime = nowTime;
                    MyMusic::play(invalidChoice);
                }
            }
            Controller::setNoLog("time", TimeManager::getCurrentTimeAsString());
            int r = TimeManager::checkIfHeatingDateTimeWasReached(Controller::getS("desiredHeatingEndTime").c_str());
            if (r == 1) {  // 0 means not yet, -1 means not set or errors
                Serial.println("HeatingDateTimeWasReached reached");
                MyMusic::play(scaleLouder, true);
            }
            // todo alarm
        }
        Controller::webSocket.textAll(Controller::model.toJsonString());
    }
};

// loose functions
int calculateFrequency() {
    int freq = (int)(Controller::getI("Rpm") / 60 * currentStepsPerRotation);
    return freq;
}

void startStepper() {
    Serial.println("Attempt to start the stepper");
    if (tempIsStepperOn) {
        Serial.println("!!!! stepper already on");
        return;  // already on
    }
    if (Controller::getI("MKSBoard")) {
        Serial.println("START MKS stepper gradually");
        setupI2SOShiftEnableMotor();
    }
    for (int rpm = (Controller::getI("Rpm") > 80 ? 80 : Controller::getI("Rpm")); rpm <= Controller::getI("Rpm"); rpm += 10) {
        double f = rpmToHertz(rpm);
        Serial.println("START STEPPER with frequency:" + String(f) + " and RPM:" + String(rpm));
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
double rpmToHertz(float rpm) {
    return (int)(rpm / 60 * currentStepsPerRotation);  // in hertz
}
//
void initialSetupStepper(bool attemptToStopAnyway = false) {
    if (Controller::getI("MKSBoard")) {
        Serial.println("initialSetupStepper STOP MKS STEPPER");
        setupI2SOShiftDisableMotor();
    } else {
        Serial.println("initialSetupStepper STOP PCB STEPPER");
    }
    ledcDetachPin(Controller::getI("StepperPwmStepPin")); /* Detach the StepPin PWM Channel to the GPIO Pin */
    tempIsStepperOn = false;
    delay(200);
}
//
void stopStepper() {
    if (!tempIsStepperOn) {
        Serial.println("!!!! stepper already off");
        return;  // already on
    }
    Serial.println("STOP STEPPER");
    if (Controller::getI("MKSBoard")) {
        Serial.println("STOP MKS STEPPER");
        setupI2SOShiftDisableMotor();
    } else {
        Serial.println("STOP PCB STEPPER");
    }
    ledcDetachPin(Controller::getI("StepperPwmStepPin")); /* Detach the StepPin PWM Channel to the GPIO Pin */
    tempIsStepperOn = false;
    delay(200);
}

int getTemperature(float& temp, float& humid) {
    // Serial.println("getTemperature called and pins..tempPin:" + String(Controller::getI("TempSensorPin")));
    if (Controller::getBool("UseOneWireForTemperature")) {
        tempSensor.requestTemperatures();
        // Serial.print("Temperature: ");  // print the temperature in Celsius
        temp = tempSensor.getTempCByIndex(0);
        Serial.print("In getTemperature temp read is:");
        Serial.println(temp);
    } else {
        // Reading temperature for humidity takes about 250 milliseconds!
        // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
        TempAndHumidity newValues = dhTempSensor.getTempAndHumidity();
        // Check if any reads failed and exit early (to try again).
        if (dhTempSensor.getStatus() != 0) {
            Serial.println("my DHT12 error status: " + String(dhTempSensor.getStatusString()));
            MyMusic::play(auClairDeLaLune);
            return false;
        }
        temp = newValues.temperature;
        if (temp < 0 || temp > 70) {
            for (int i = 0; i < 10; i++) {
                MyMusic::play(darthVader, true);  // temperature out of range
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
/** return -1 if unchanged and 0 for off and 1 for on*/
int readTurnOnStepperButton() {
    lastStepperOnOffButtonState = !digitalRead(Controller::getI("StepperOnOffSwitchInputPin"));
    int ret = -1;  // means unchanged
    // check to see if you just pressed the button
    // (i.e. the input went from LOW to HIGH), and you've waited long enough
    // since the last press to ignore any noise:
    // If the switch/button changed, due to noise or pressing:
    if (lastStepperOnOffButtonState != lastFlickerableState) {
        // reset the debouncing timer
        lastDebounceTime = millis();
        // save the the last flickerable state
        lastFlickerableState = lastStepperOnOffButtonState;
    }
    if ((millis() - lastDebounceTime) > DebounceTime) {
        // whatever the reading is at, it's been there for longer than the debounce
        // delay, so take it as the actual current state:
        // if the button state has changed:
        if (lastStepperOnOffButtonState == HIGH && lastStepperOnOffButtonState == LOW) {
            if (Debug::LOG_STEPPER_ON_OFF_SWITCH_STATE) {
                Serial.println("The stepper motor button was turned off");
                MyMusic::play(validChoice);
            }
            ret = 0;
        } else if (lastStepperOnOffButtonState == LOW && lastStepperOnOffButtonState == HIGH) {
            if (Debug::LOG_STEPPER_ON_OFF_SWITCH_STATE) {
                Serial.println("The stepper motor button was turned on");
                MyMusic::play(validChoice);
            }
            ret = 1;
        }
        // save the the last steady state
        lastStepperOnOffButtonState = lastStepperOnOffButtonState;
    }
    return ret;
}
//
// String getFormatedTimeSinceStart() {
//     unsigned long time = (unsigned long)((millis() - currentStartTime) / 1000);  // finds the time since last print in secs
//     return formatTime(time);
// }
//
void fanSetup() {
    pinMode(Controller::getI("FanPin"), OUTPUT);
    fan(true);
    Serial.print("Fan pin ");
    Serial.print(Controller::getI("FanPin"));
    Serial.print(" on channel ");
    Serial.print(FAN_PWM_CHANNEL);
    Serial.print(" at frequency ");
    Serial.print(FanFrequency);
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

void heater(bool on, float duty) {
    // this works only after setup was called to initialize the channel
    if (on) {
        ledcWrite(HEATER_PWM_CHANNEL, duty);  // Heater start
    } else {
        ledcWrite(HEATER_PWM_CHANNEL, 0);  // Heater stop
    }
    if (Debug::LOG_HEATER_ON_OFF_STATE) {
        Serial.print("Heater set to: ");
        Serial.print(on);
        if (on) {
            Serial.print(" with duty at:");
            Serial.print((duty / UNIVERSAL_MAX_DUTY_CYCLE) * 100);
            Serial.print("%");
        }
        Serial.println();
    }
}

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

//
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
void processStepperStartOrStop() {
    bool desired = Controller::getBool("StepperOn");
    if (Controller::getPresent("StepperOnOffSwitchInputPin")) {              // it's an end with the motor on signal so both the physical and soft on for motor to run
        int stepperHardwareSwitchOnOffPosition = readTurnOnStepperButton();  //-1 for unchanged
        if (stepperHardwareSwitchOnOffPosition != -1) {                      // todo revisit logic
            if (stepperHardwareSwitchOnOffPosition == 1 && !tempIsStepperOn && desired) {
                startStepper();
                if (Controller::getPresent("FanPin")) fan(false);
            } else {  // hardware swich is off
                if (stepperHardwareSwitchOnOffPosition == 0 && !tempIsStepperOn && !desired) {
                    stopStepper();
                    if (Controller::getPresent("FanPin")) fan(true);
                }
            }
        }
    } else if (!tempIsStepperOn && desired) {  // viceversa so something changed
        startStepper();
        if (Controller::getPresent("FanPin")) fan(false);
    } else if (tempIsStepperOn && !desired) {
        stopStepper();
        if (Controller::getPresent("FanPin")) fan(true);
    }
}
