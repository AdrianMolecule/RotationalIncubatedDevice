#pragma once
#include <Arduino.h>

#include "DHTesp.h"  //for DHT temp sensor
// memory card https://www.mischianti.org/2021/03/28/how-to-use-sd-card-with-esp32-2/
//  include the SD library:
#include <DallasTemperature.h>
#include <FS.h>
#include <Melody.h>
#include <OneWire.h>
#include <SD.h>
#include <SPI.h>

#include <vector>

#include "ActualMusic.h"
#include "Controller.h"
#include "Helper.h"
#include "Microstepping.h"
//
void play(Melody melody);
void play(Melody melody, bool force);
void startStepper();
void stopStepper();
void fanSetup();
void fan(bool on);
int readPotentiometer();
void heater(bool on, int duty);
String getFormatedTimeSinceStart();
void desiredEndTimeCheck();
void setupI2SOShiftEnableMotor();
void setupI2SOShiftDisableMotor();
void createDir(fs::FS& fs, const char* path);
void writeFile(fs::FS& fs, const char* path, const String& message);
String formatTime(unsigned long time);
String getFormatedTimeSinceStart();
String getDurationToAlarm();
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
int lastTempHumidityReadTime = 0;  // never
unsigned long lastAlertTime = 0;   // never
int desiredEndTime = -1;           // in minutes
float oldTemperature = 0.;
float minHumidity = 60.;
bool firstTimeTurnOnHeater = true;
bool firstTimeReachDesiredTemperature = true;
float maxTemperature = 0;
// motor what we want for OS motor
// https://github.com/nenovmy/arduino/blob/master/leds_disco/leds_disco.ino
/* Setting all PWM motor, Heater PWM Properties */
// fan
const float fanDutyCyclePercentage = 1;
const int FanFrequency = 40000;
float startTemperature = 0;
namespace Debug {
const bool HEATER = true;
const bool FAN = true;
const bool SWITCH = true;
}  // namespace Debug
uint8_t HEATER_PWM_CHANNEL = 0;
uint8_t STEPPER_PWM_CHANNEL = 2;
uint8_t SPEAKER_CHANNEL = 4;
uint8_t FAN_PWM_CHANNEL = 8;
// WeMos D1 esp8266: D8 as standard
const int chipSelect = SS;
uint8_t maxHeaterDutyCyclePercentage;       // TODO unused
int currentStepsPerRotation = 200;          // TODO unused
unsigned long currentStartTime = millis();  // time since ESP power on in millis TODO unused
// SD memory card
const int32_t SPIfreq = 40000;
const int UNIVERSAL_PWM_RESOLUTION = 10;
const int UNIVERSAL_MAX_DUTY_CYCLE = (int)(pow(2, UNIVERSAL_PWM_RESOLUTION) - 1);
const float MODERATE_HEAT_POWER = 0.9;
const int HALF_DUTY_CYCLE = UNIVERSAL_MAX_DUTY_CYCLE / 2;
//
bool tempIsStepperOn = true;  // need to be true because the first op is to stop it
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

/* OneWire oneWire = OneWire(Controller::getILogged("TempSensorPin"));
DallasTemperature tempSensor(); */
class BackEnd {
   public:
    static inline unsigned long lastModelUpdateInSeconds = 0;

    static void setupBackend() {
        Serial.println("setupBackend begin begins XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
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
        pinMode(Controller::getI("LedPin"), OUTPUT);
        pinMode(Controller::getI("StepperPwmStepPin"), OUTPUT);
        pinMode(Controller::getI("I2SoLatchPin"), OUTPUT);
        pinMode(Controller::getI("I2SoClockPin"), OUTPUT);
        pinMode(Controller::getI("I2SoDataPin"), OUTPUT);
        // pinMode(Controller::getI("FanPin"), OUTPUT);
        // // pinMode(Controller::getI("StepperEnablePin"), INPUT);
        // pinMode(Controller::getI("SpeakerPin"), OUTPUT);
        // // alternate(LedPin, 50, 5);
        // //   speaker
        ledcSetup(SPEAKER_CHANNEL, 5000, 8);
        ledcAttachPin(Controller::getI("SpeakerPin"), SPEAKER_CHANNEL);
        ledcWrite(SPEAKER_CHANNEL, 0);  // duty Cycle = 0
        play(validChoice);
        // temperature sensor
        if (Controller::getPresent("TempSensorPin")) {
            if (!Controller::getI("UseOneWireForTemperature")) {
                dhTempSensor.setup(Controller::getI("TempSensorPin"), DHTesp::DHT22);
                if (dhTempSensor.getStatus() == DHTesp::ERROR_TIMEOUT) {
                    play(darthVader, true);
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
            ledcAttachPin(Controller::getI("HeaterPwmPin"), HEATER_PWM_CHANNEL); /* Attach the StepPin PWM Channel to the GPIO Pin */
            delay(20);
            Serial.println("stepper desiredRPM:");
            Serial.println(Controller::getI("Rpm"));
            // motor
            Serial.println("Initial disabling of the stepper in setup");
            stopStepper();
            Serial.println("Initial disabling of the heater in setup");
            heater(false, -1);
            play(scaleLouder);
        } else {
            Serial.println("No heater pin defined!");
        }
        if (Controller::getPresent("FanPin")) {
            fanSetup();
        } else {
            Serial.println("No fan for this device");
        }
        // setupSDCard();
        Serial.println(Controller::getI("maxHeaterDutyCycle"));
        float temperature;
        float humidity;
        if (Controller::getI("desiredTemperature") == 30) {
            play(temp30);
        } else if (Controller::getI("desiredTemperature") == 37) {
            play(temp37);
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
    //
    static inline bool first = true;
    static void loopBackend() {
        if (first) {
            Serial.println("[SYS] loopBackend Started.");
            first = false;
        }
        processStepperStartOrStop();
        desiredEndTimeCheck();
        // Get temperature
        float temperature;
        float humidity;
        bool TEMPERATURE_DISPLAY = true;
        bool TIME_DISPLAY = true;
        float dT = Controller::getI("desiredTemperature");
        if (((millis() - lastTempHumidityReadTime) / 1000) > 2) {  // every 2 seconds
            getTemperature(temperature, humidity);
            Controller::set("currentTemperature", String(temperature));
            lastTempHumidityReadTime = millis();
            if (temperature > maxTemperature) {
                maxTemperature = temperature;
            }
            if (TEMPERATURE_DISPLAY) {
                Serial.print("Current temp: " + String(temperature) + ", " + (!Controller::getBool("UseOneWireForTemperature") ? "Humidity:" + String(humidity) + " ," : ""));
                Serial.print("DesiredTemperature: " + String(dT));
                Serial.print(" max temperature: " + String(maxTemperature));
                if (TIME_DISPLAY) {
                    Serial.println(" Time since start: " + getFormatedTimeSinceStart() + " ");
                } else {
                    Serial.println(" ");
                }
            }
            if (temperature < dT) {
                if (!Controller::getBool("currentHeaterOn")) {
                    Serial.println("Turning heater ON");
                    if (firstTimeTurnOnHeater) {
                        play(auClairDeLaLune);
                        firstTimeTurnOnHeater = false;
                    }
                    if (dT - temperature >= 2) {
                        heater(true, UNIVERSAL_MAX_DUTY_CYCLE);  // Heater start
                    } else {
                        heater(true, UNIVERSAL_MAX_DUTY_CYCLE * MODERATE_HEAT_POWER);
                    }
                    Controller::setBool("currentHeaterOn", true);
                }
            } else {  // no need to heat
                if (Controller::getBool("currentHeaterOn")) {
                    Serial.println("Turning heater OFF");
                    if (firstTimeReachDesiredTemperature) {
                        if (Controller::getBool("TemperatureReachedMusicOn")) {
                            play(frereJacquesFull, true);
                        }
                        firstTimeReachDesiredTemperature = false;
                    }
                    heater(false, 0);  // second arg is ignored when heater is turned off
                    Controller::setBool("currentHeaterOn", false);
                }
            }
            if (!Controller::getI("UseOneWireForTemperature") && humidity < minHumidity && ((millis() - lastAlertTime) / 1000) > 200 /* about 3 minutes*/) {
                Serial.println("WARNING !!! humidity dropped to less then minimal humidity");
                unsigned long nowTime = millis();
                if (nowTime > lastAlertTime + 2000) {
                    lastAlertTime = nowTime;
                    play(invalidChoice);
                }
            }
        }
    }
}

;
// old loop
//      unsigned long durationSinceRebootInSeconds = millis() / 1000;
//      while (true) {
//          // if Controller::model.
//          durationSinceRebootInSeconds = millis() / 1000;
//          if (durationSinceRebootInSeconds - lastModelUpdateInSeconds >= 5) {
//              lastModelUpdateInSeconds = durationSinceRebootInSeconds;
//              Field* f = Controller::model.getByName("duration");
//              if (!f) {
//                  Field nf("10", "duration", "string", String(durationSinceRebootInSeconds), "time since start", false);
//                  Controller::model.add(nf);
//              } else
//                  f->setValue(String(durationSinceRebootInSeconds));
//              Controller::model.saveToFile();
//              Controller::webSocket.textAll(Controller::model.toJsonString());
//          }
//          Field* f = Controller::model.getByName("delay");
//          String delayAsString;
//          if (!f) {
//              Field nf("11", "delay", "string", "7", "blocking delay in backend", false);
//              Controller::model.add(nf);
//              Controller::model.saveToFile();
//              Controller::webSocket.textAll(Controller::model.toJsonString());
//              delayAsString = nf.getValue();
//          } else {
//              delayAsString = f->getValue();
//          }
//          int delayAsInt = delayAsString.toInt() * 1000;
//          Serial.println("[BACKEND] Blocking delay " + delayAsString + "s");
//          Controller::webSocket.textAll(Controller::model.toJsonString());
//          delay(delayAsInt);
//      }
//      }
//
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
        Controller::setBool("StepperOn", true);
    }
    Controller::setBool("StepperOn", 1);
    tempIsStepperOn = true;
}
//
double rpmToHertz(float rpm) {
    return (int)(rpm / 60 * currentStepsPerRotation);  // in hertz
}
//
void stopStepper() {
    if (!tempIsStepperOn) {
        Serial.println("!!!! stepper already off");
        return;  // already on
    }
    Serial.println("STOP STEPPER");
    tempIsStepperOn = true;
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
            play(auClairDeLaLune);
            return false;
        }
        temp = newValues.temperature;
        if (temp < 0 || temp > 70) {
            for (int i = 0; i < 10; i++) {
                play(darthVader, true);  // temperature out of range
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

//
// void alternate(int pin, int de, int times) {
// //	Serial.println("alternate on pin:");
// //	Serial.println(pin);
// 	for (int var = 0; var < times; ++var) {
// 		digitalWrite(pin, HIGH);
// 		delay(de);
// 		digitalWrite(pin, LOW);
// 		delay(de);
// 	}
// }

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
            if (Debug::SWITCH) {
                Serial.println("The stepper motor button was turned off");
                play(validChoice);
            }
            ret = 0;
        } else if (lastStepperOnOffButtonState == LOW && lastStepperOnOffButtonState == HIGH) {
            if (Debug::SWITCH) {
                Serial.println("The stepper motor button was turned on");
                play(validChoice);
            }
            ret = 1;
        }
        // save the the last steady state
        lastStepperOnOffButtonState = lastStepperOnOffButtonState;
    }
    return ret;
}
//
void setLoudness(int loudness) {
    // Loudness could be use with a mapping function, according to your buzzer or sound-producing hardware
    const int MinHardware_LOUDNESS = 0;
    const int MaxHardware_LOUDNESS = 16;
    ledcWrite(SPEAKER_CHANNEL, map(loudness, -4, 4, MinHardware_LOUDNESS, MaxHardware_LOUDNESS));
}
//
String getFormatedTimeSinceStart() {
    unsigned long time = (unsigned long)((millis() - currentStartTime) / 1000);  // finds the time since last print in secs
    return formatTime(time);
}
// we work in seconds and startTime is in miliseconds
String getDurationToAlarm() {
    if (desiredEndTime == -1) {
        return "No alarm set";
    }
    unsigned long time = desiredEndTime * 60 - (unsigned long)((millis() - currentStartTime) / 1000);
    return formatTime(time);
}
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
    Serial.print(" at duty cycle ");
    Serial.print(UNIVERSAL_MAX_DUTY_CYCLE * fanDutyCyclePercentage);
    Serial.print(" or percentage ");
    Serial.println(fanDutyCyclePercentage);
    delay(3000);
    fan(false);
}
//
void fan(bool on) {
    if (Controller::getI("FanOn") != on) {
        Controller::setBool("FanOn", on);
    }
    if (on) {
        ledcAttachPin(Controller::getI("FanPin"), FAN_PWM_CHANNEL);
        ledcWrite(Controller::getI("FAN_PWM_CHANNEL"), UNIVERSAL_MAX_DUTY_CYCLE * fanDutyCyclePercentage);
    } else {
        ledcDetachPin(Controller::getI("FanPin"));
        ledcWrite(Controller::getI("FAN_PWM_CHANNEL"), 0);
    }
    if (Debug::FAN) {
        Serial.print("Fan set to: ");
        Serial.println(on);
    }
}

void heater(bool on, int duty) {
    // this works only after setup was called to initialize the channel
    if (on) {
        ledcWrite(HEATER_PWM_CHANNEL, duty);  // Heater start
        digitalWrite(Controller::getI("LedPin"), HIGH);
    } else {
        ledcWrite(HEATER_PWM_CHANNEL, 0);  // Heater stop
        digitalWrite(Controller::getI("LedPin"), LOW);
    }
    if (Debug::HEATER) {
        Serial.print("Heater set to: ");
        Serial.print(on);
        if (on) {
            Serial.print(" with duty at:");
            Serial.print(((float)duty / UNIVERSAL_MAX_DUTY_CYCLE) * 100);
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
void play(Melody melody) {
    if (Controller::getI("MostMusicOff")) {
        return;
    }
    melody.restart();         // The melody iterator is restarted at the beginning.
    while (melody.hasNext())  // While there is a next note to play.
    {
        melody.next();                                   // Move the melody note iterator to the next one.
        unsigned int frequency = melody.getFrequency();  // Get the frequency in Hz of the curent note.
        unsigned long duration = melody.getDuration();   // Get the duration in ms of the curent note.
        unsigned int loudness = melody.getLoudness();    // Get the loudness of the curent note (in a subjective relative scale from -3 to +3).
                                                         // Common interpretation will be -3 is really soft (ppp), and 3 really loud (fff).
        if (frequency > 0) {
            ledcWriteTone(SPEAKER_CHANNEL, frequency);
            setLoudness(loudness);
        } else {
            ledcWrite(SPEAKER_CHANNEL, 0);
        }
        delay(duration);
        // This 1 ms delay with no tone is added to let a "breathing" time between each note.
        // Without it, identical consecutives notes will sound like just one long note.
        ledcWrite(SPEAKER_CHANNEL, 0);
        delay(1);
    }
    ledcWrite(SPEAKER_CHANNEL, 0);
    delay(1000);
}

void play(Melody melody, bool force) {
    if (force) {
        play(melody);
    }
}

void desiredEndTimeCheck() {
    if (desiredEndTime != -1 && desiredEndTime * 60 * 1000 <= (millis() - currentStartTime)) {
        Serial.println("Desired end time " + String(desiredEndTime) + " reached. Current elapsed time: " + getFormatedTimeSinceStart());
        play(scaleLouder, true);
    }
}
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
    if (Controller::getPresent("StepperOnOffSwitchInputPin")) {              // it's an end with the motor on signal so both the physical and soft on for motor to run
        int stepperHardwareSwitchOnOffPosition = readTurnOnStepperButton();  //-1 for unchanged
        if (stepperHardwareSwitchOnOffPosition != -1) {
            if (stepperHardwareSwitchOnOffPosition == 1 && !tempIsStepperOn && Controller::getBool("StepperOn")) {
                startStepper();
                fan(false);
            } else {  // hardware swich is off
                if (stepperHardwareSwitchOnOffPosition == 0 && !tempIsStepperOn && !Controller::getBool("StepperOn")) {
                    stopStepper();
                    fan(true);
                }
            }
        }
    } else if (!tempIsStepperOn && Controller::getBool("StepperOn")) {  // viceversa so something changed
        startStepper();
        fan(false);
    } else if (tempIsStepperOn && !Controller::getBool("StepperOn")) {
        stopStepper();
        fan(false);
    }
}