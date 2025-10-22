// FOR MAKERBASE BOARD :           C:\a\diy\eagle\makerBaseDlc32v2.1.002/MksDlc32 _V2.1_002_SCH.pdf
// https://github.com/makerbase-mks/MKS-DLC32/blob/main/MKS-DLC32-main/doc/DLC32%20wiring%20manual.pdf
#include <DallasTemperature.h>
#include <FS.h>
#include <Melody.h>
#include <OneWire.h>
// Suggest SD card: Class4 or Class10; 4~16G memory; Fat32 format. File format support: .NC; .GC; .GCODE
// SD memory card SdD0=I12 ,SdDi=IO13,SdCk=IO14,SdCs=IO15,SdDet=IO39/SensorVn/ based on the C:\a\diy\my machines and tools\my components\makerBaseDlc32v2.1.002.sch
#include <SD.h>  //  include the SD library:memory card https://www.mischianti.org/2021/03/28/how-to-use-sd-card-with-esp32-2/
//
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <WiFi.h>
#include <ArduinoJson.h>
//
#include "AdrianOsPcbPins.h"
#include "Arduino.h"
#include "DHTesp.h"  //for DHT temp sensor
#include "Microstepping.h"
#include "Music.h"
#include "Pins.h"
#include "SerialCommunication.h"
#include "Splitter.h"
/* Pins_Arduino_h */
// music
#define SERIAL_BAUDRATE 115200
//********STEPPER***********
// we use ProbePin=22 for stepper STEP so connect PROBE S to DR8825 so we bend out the STEP pin of the leftmost X driver and put a wire
// The step pin is the second from the left bottom see C:\a\diy\my machines and tools\my components\makerBaseDlc32v2.1.002
// and we connect that to ProbeS Lower pin of connector J12 as
// for temperature sensor use either the dh TempAndHumidity with GND and 5 Volt from Probe connector and
//  DAT to DhtPin=19 which is on extension LcdMiso
//
//*********BUZZER***********
// for speaker we use GND and on + in series with 100  OHM we use (GPIO23 which is the same as LcdMosi).
// That means LcdMosi or Extension X2 pin 6 ---Buzzer with proper polarity --[100 Ohm]---GND
//
//*********HEATER PAD***********
// we need to find an available unused GPIO
//
//*********TEMP Sensor
// we use one main pin for euther dh or OneWIre and that is pin GPIO19 on pin 1 of Expansion 2 connector***********
//
//*********MOTOR ON OFF Switch***********
// REMOVED TODO CHANGE WITH CAPACITIVE  connect switch from GND to TurnOnStepperPin = 36/Sensor_VP SVP/- Xlimit on J9 (last to the right lower pin)) MOTOR starts when pin36 is put to Gnd
//
//  for motor current // Vref = Imax /2
// for m17hs16-2004s1 https://www.omc-stepperonline.com/nema-17-bipolar-45ncm-64oz-in-2a-42x42x40mm-4-wires-w-1m-cable-connector-17hs16-2004s1 and Imax=2 AMp hold torque=Holding Torque: 45Ncm(64oz.in)
// and https://www.youtube.com/watch?v=BV-ouxhZamI
//   to measure on screw-driver=2*8*.068 about 1 volt but I'll go less maybe .9 volts
//
// good doc for pins at https://github.com/diruuu/FluidNC/blob/main/example_configs/MKS-DLC32-v2.0.yaml
// we have 2 io available and we use the one labeled 35
// as in IO35 which is connected to the connector IO pin 6
//(the last pin is 3.3 v and it's in the right top corner when controller key is on the left and board is horizontal - top view
// that is pulled to ground by R29 and serialized by current limiting R28
const int PotentiometerPin = 35;
// web
const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

AdrianOsPcbPins pins = AdrianOsPcbPins();  // pins is a reference to the object
//
enum Preference {  // for preferences
    TimeDisplay = true,
    TemperatureDisplay = true,
    MostMusic_OFF = false,  // turns off all music except for errors, warnings, time reached and first time desired temperature reached
    TemperatureReached_MusicOn = true,

};
namespace Debug {
const bool HEATER = true;
const bool FAN = true;
const bool SWITCH = true;
const bool LowHumidity = true;
}  // namespace Debug
// FAN https://fdossena.com/?p=ArduinoFanControl/i.md
// timer
// temperature sensor stuff
const float ModerateHeat_POWER = 0.9;
// Temperature either dh or oneWireDallas
DHTesp dhTempSensor;                  // used in setup and readTemperature
OneWire oneWire(pins.TempSensorPin);  // GPIO where the DS18B20 is connected to. Used to be GPIO36 but not anymore
DallasTemperature tempSensor(&oneWire);
int lastTempHumidityReadTime = 0;  // never
int lastAlertTime = 0;             // never
//
float currentRPM = 200;  // 30 for demo
//
float oldTemperature = 0.;
float minHumidity = 60.;
bool heaterIsOn = false;
bool firstTimeTurnOnHeater = true;
bool firstTimeReachDesiredTemperature = true;
float maxTemperature = 0;
// motor what we want for OS motor
// https://github.com/nenovmy/arduino/blob/master/leds_disco/leds_disco.ino
/* Setting all PWM motor, Heater PWM Properties */
// stepper motor
const int UNIVERSAL_PWM_RESOLUTION = 10;
const int UNIVERSAL_MAX_DUTY_CYCLE = (int)(pow(2, UNIVERSAL_PWM_RESOLUTION) - 1);
const int STEPPER_HALF_FUTY_CYCLE = UNIVERSAL_MAX_DUTY_CYCLE / 2;
// fan
const float fanDutyCyclePercentage = 1;
const int FanFrequency = 40000;
float startTemperature = 0;
// heater pad
// Oct 2025  for Tube rotator with 2 pads in series we put max duty to 1
// heater pad // 80x100mm 12V DC 20W Silicone Heated Bed Heating Pad 8 Ohm per pad,
//.92 Amps for a 19.5 power adapter//MKS spindle is 24V 40W so 1.66 Amps
// with 2 pads in series and power at 80% we get 19.5 watts
int maxHeaterDutyCyclePercentage = pins.maxHeaterDutyCyclePercentage;  // initial value
int lastStepperOnOffButtonState = LOW;                                 // the previous steady state from the input pin
int lastFlickerableState = -100;                                       // the previous flicker-able state from the input pin
bool lastFanState = false;                                             // last fan state
bool isStepperOn = false;                                              // last isStepperOn state ON or off
// declare some methods because we don't have an include file for this .ino file
void play(Melody melody);
void play(Melody melody, bool force);
void startStepper();
void stopStepper();
void fanSetup();
void fan(bool on);
int readPotentiometer();
void heater(bool on, int duty);
String getFormatedDurationSinceStart();
void desiredHeatingEndTimeCheck();
void setupI2SOShiftEnableMotor();
void setupI2SOShiftDisableMotor();
void createDir(fs::FS& fs, const char* path);
void writeFile(fs::FS& fs, const char* path, const String& message);
String formatTime(unsigned long time);
String getFormatedDurationSinceStart();
unsigned long getSecondsToAlarm();
int readTurnOnStepper();
void processStepperOnOffSwitch();
void setupSDCard();
void printDirectory(File dir, int numTabs);
int getTemperature(float& temp, float& humid);
void writeData(byte* bits);
void processCommand();
double rpmToHertz(float rpm);
void setStepsPerRotation(int newStepsPerRotation);
int calculateHeaterDutyCycle();
void displayHelp();
void displayCurrentState();
void webClientSetup();

    // end INCLUDES.h
    // time

    // SD memory card
    const int32_t SPIfreq = 40000;
SerialCommunication serialCommunication = SerialCommunication();
//
// need to Open Folder C:\a\diy\espProjects\OrbitalShakerAdrianBoard for the libraries

void setup() {
    Serial.begin(115200);
    Serial.println("================================= Setup Starting Now =============================");
    play(scaleLouder, true);
    if (pins.MKSBoard) {
        Serial.println("Using MKS DLC32 v2.1 board specific setup");
        pinMode(pins.StepperEnablePin, INPUT);
        // startStepper();
    } else {  // this is ALL needed for my custom PCB and it uses the mechanical switch
        pinMode(pins.StepperEnablePin, OUTPUT);
        digitalWrite(pins.StepperEnablePin, LOW);
        pinMode(STEPPER_stepsPerRotation_M0, OUTPUT);
        pinMode(STEPPER_stepsPerRotation_M1, OUTPUT);
        pinMode(STEPPER_stepsPerRotation_M2, OUTPUT);
    }  // end just for the pcb
    if (pins.currentStepsPerRotation != Pins::NOT_PRESENT) {
        setStepsPerRotation(pins.currentStepsPerRotation);
    }
    //  Declare pins as output:
    pinMode(pins.LedPin, OUTPUT);
    pinMode(pins.StepperPwmStepPin, OUTPUT);
    pinMode(pins.I2SoLatchPin, OUTPUT);
    pinMode(pins.I2SoClockPin, OUTPUT);
    pinMode(pins.I2SoDataPin, OUTPUT);
    pinMode(pins.FanPin, OUTPUT);
    // pinMode(pins.StepperEnablePin, INPUT);
    pinMode(pins.SpeakerPin, OUTPUT);
    // alternate(LedPin, 50, 5);
    //   speaker
    ledcSetup(pins.SpeakerChannel, 5000, 8);
    ledcAttachPin(pins.SpeakerPin, pins.SpeakerChannel);
    ledcWrite(pins.SpeakerChannel, 0);  // duty Cycle = 0
    play(validChoice);
    // temperature sensor
    if (pins.TempSensorPin != Pins::NOT_PRESENT) {
        if (!pins.UseOneWireForTemperature) {
            dhTempSensor.setup(pins.TempSensorPin, DHTesp::DHT22);
            if (dhTempSensor.getStatus() == DHTesp::ERROR_TIMEOUT) {
                play(darthVader, true);
                Serial.println("No DHT22 found or not working properly!");
            }

        } else {
            tempSensor.begin();
        }
        Serial.println("Temp sensor initiated");
    } else {
        Serial.println("No temperature sensor pin defined!");
    }
    // heater
    if (pins.HeaterPin != Pins::NOT_PRESENT) {
        pinMode(pins.HeaterPin, OUTPUT);
        // setup the heater
        ledcSetup(pins.HeaterPwmChannel, 40000, UNIVERSAL_PWM_RESOLUTION);  // normal PWM frrequency for MKS is 5000HZ
        delay(20);
        ledcAttachPin(pins.HeaterPin, pins.HeaterPwmChannel); /* Attach the StepPin PWM Channel to the GPIO Pin */
        delay(20);
        Serial.println("stepper currentRPM:");
        Serial.println(currentRPM);
    } else {
        Serial.println("No heater pin defined!");
    }
    // motor
    Serial.println("Initial disabling of the stepper in setup");
    stopStepper();
    play(scaleLouder);
    if (pins.FanPin != Pins::NOT_PRESENT) {
        fanSetup();
    } else {
        Serial.println("No fan pin defined!");
    }
    // setupSDCard();
    // oneFullRotation();?

    Serial.println("maxHeaterDutyCyclePercentage" + String(calculateHeaterDutyCycle()));
    float temperature;
    float humidity;
    if (pins.desiredTemperature == 30) {
        play(temp30);
    } else if (pins.desiredTemperature == 37) {
        play(temp37);
    }
    delay(500);
    startStepper();
    if (pins.TempSensorPin != Pins::NOT_PRESENT) {
        getTemperature(temperature, humidity);
        startTemperature = temperature;
        Serial.println("startTemperature:" + String(startTemperature) + " startHumidity:" + String(humidity));
    }
    Serial.println("============END Setup. Version:" + pins.VERSION + "===============");
}
//
void loop() {
    processCommand();
    if (pins.MKSBoard && pins.StepperEnablePin != Pins::NOT_PRESENT) {
        processStepperOnOffSwitch();
    }
    desiredHeatingEndTimeCheck();
    // Get temperature
    float temperature;
    float humidity;
    if (pins.TempSensorPin != Pins::NOT_PRESENT) {
        if (((millis() - lastTempHumidityReadTime) / 1000) > 2) {  // every 2 seconds
            getTemperature(temperature, humidity);
            lastTempHumidityReadTime = millis();
            if (temperature > maxTemperature) {
                maxTemperature = temperature;
            }
            if (TemperatureDisplay) {
                Serial.print("Current temp: " + String(temperature) + ", " + (pins.UseOneWireForTemperature == 0 ? "Humidity:" + String(humidity) + " ," : ""));
                Serial.print("DesiredTemperature: ");
                Serial.print(pins.desiredTemperature);
                Serial.print(" max temperature: " + String(maxTemperature));
                if (TimeDisplay) {
                    Serial.print(" Time since start: " + getFormatedDurationSinceStart() + " ");
                } else {
                    Serial.print(" ");
                }
            }
        }
        if (temperature < pins.desiredTemperature) {
            if (firstTimeTurnOnHeater) {
                play(auClairDeLaLune);
                firstTimeTurnOnHeater = false;
            }
            digitalWrite(pins.LedPin, HIGH);
            if (pins.desiredTemperature - temperature >= 2) {
                heater(true, calculateHeaterDutyCycle());  // Heater start
            } else {
                heater(true, calculateHeaterDutyCycle() * ModerateHeat_POWER);
            }
        } else {               // no need to heat
            if (heaterIsOn) {  // was on before this, used to stop printing too many times
                Serial.println("Turning heater OFF");
                if (firstTimeReachDesiredTemperature) {
                    if (TemperatureReached_MusicOn) {
                        play(frereJacquesFull, true);
                    }
                    firstTimeReachDesiredTemperature = false;
                } else
                    play(frereJacques);
            }
            digitalWrite(pins.LedPin, LOW);
            heater(false, 0);  // second arg is ignored when heater is turned off
            if (!pins.UseOneWireForTemperature && humidity < minHumidity && ((millis() - lastAlertTime) / 1000) > 200 /* about 3 minutes*/) {
                Serial.println("WARNING !!! humidity dropped less then minimal humidity");
                lastAlertTime = millis();
                if (Debug::LowHumidity) {
                    play(invalidChoice);
                }
            }
        }
    }
    server.handleClient();
    webSocket.loop();   
}
//
void processStepperOnOffSwitch() {
    int stepperOnOffPosition = readTurnOnStepper();  //-1 for unchanged
    if (stepperOnOffPosition != -1) {
        if (stepperOnOffPosition == 1) {
            startStepper();
            fan(false);
        } else {
            if (stepperOnOffPosition == 0) {
                stopStepper();
                fan(true);
            }
        }
    }
}
//
void processCommand() {
    String command = "";
    String commandArguments = "";
    serialCommunication.checkForCommand(command, commandArguments);
    if (!command.equals("")) {
        if (command.indexOf("d") == 0) {
            File dir = SD.open("/");
            printDirectory(dir, 0);
        } else if (command.indexOf("r") == 0) {
            // Splitter splitter = Splitter(command);
            // String newRPMAsString = splitter.getItemAtIndex(1);
            if (commandArguments.isEmpty()) {
                Serial.print("Please enter the new desired RPM after r. Like r 80 for RPM=80");
                return;
            }
            currentRPM = commandArguments.toFloat();
            Serial.println("new RPM is: " + commandArguments);
            startStepper();
        } else if (command.indexOf("t") == 0) {
            if (commandArguments.isEmpty()) {
                Serial.print("Please enter the new desired temperature . Like command 29 for temp 29 Celsius");
                return;
            }
            pins.desiredTemperature = commandArguments.toFloat();
            Serial.print("new pins.desiredTemperature is: ");
            Serial.println(pins.desiredTemperature);
            if (pins.desiredTemperature == 30) {
                play(temp30);
            } else if (pins.desiredTemperature == 37) {
                play(temp37);
            }
        } else if (command.indexOf("m") == 0) {
            if (commandArguments.isEmpty()) {
                Serial.print("Please enter m 0 for motor off or m anything else for motor on");
                return;
            }
            int arg = commandArguments.toInt();
            if (arg == 0) {
                stopStepper();
            } else {
                startStepper();
            }
            Serial.print("new steper status is: ");
            Serial.println(isStepperOn);
        } else if (command.indexOf("s") == 0) {
            if (commandArguments.isEmpty()) {
                Serial.print("we reset the start time to now()");
            }
            pins.currentStartTime = millis();
        } else if (command.indexOf("e") == 0) {
            if (commandArguments.isEmpty()) {
                Serial.print("Please enter an end time in minutes");
                return;
            }
            pins.currentHeatingEndDurationInMinutes = commandArguments.toInt();
            Serial.print("new desired Heating End Time (from the very start not from now) is: ");
            Serial.print(pins.currentHeatingEndDurationInMinutes);
            Serial.println(" minutes");
        } else if (command.indexOf("c") == 0) {
            if (commandArguments.isEmpty()) {
                Serial.print("Please enter a new heater duty cycle percentage. Like c 80 for 80 percent of max heater duty cycle");
                return;
            }
            maxHeaterDutyCyclePercentage = commandArguments.toInt();
            Serial.println("new maxHeaterDutyCyclePercentage is: " + String(maxHeaterDutyCyclePercentage));
            Serial.println("new maxHeaterDutyCycle is: " + String(calculateHeaterDutyCycle()));
        } else if (command.indexOf("p") == 0) {
            if (pins.currentStepsPerRotation == Pins::NOT_PRESENT) {
                Serial.println("currentStepsPerRotation change is not supported by this board via code");
                return;
            } else {
                if (commandArguments.isEmpty()) {
                    Serial.print("Please enter a new currentStepsPerRotation. Like p 400");
                    return;
                }
                int newStepsPerRotation = commandArguments.toInt();
                setStepsPerRotation(newStepsPerRotation);
                if (isStepperOn) {
                    startStepper();
                }
                Serial.println("new currentStepsPerRotation is: " + String(pins.currentStepsPerRotation));
            }
        } else if (command.indexOf("?") == 0) {
            displayCurrentState();
            displayHelp();
        } else {
            Serial.print("cannot find command ");
            Serial.println(command);
        }
    }
}
//
//?? not accurate but an idea. we move up  to
// half of RPM in x2 stepsPerRotation and then full in no stepsPerRotation
void setStepsPerRotation(int newStepsPerRotation) {
    switch (newStepsPerRotation) {
        case STEPS200:
            digitalWrite(STEPPER_stepsPerRotation_M0, LOW);
            digitalWrite(STEPPER_stepsPerRotation_M1, LOW);
            digitalWrite(STEPPER_stepsPerRotation_M2, LOW);
            break;
        case STEPS400:
            digitalWrite(STEPPER_stepsPerRotation_M0, HIGH);
            digitalWrite(STEPPER_stepsPerRotation_M1, LOW);
            digitalWrite(STEPPER_stepsPerRotation_M2, LOW);
            break;
        case STEPS800:
            digitalWrite(STEPPER_stepsPerRotation_M0, LOW);
            digitalWrite(STEPPER_stepsPerRotation_M1, HIGH);
            digitalWrite(STEPPER_stepsPerRotation_M2, LOW);
            break;
        case STEPS1600:
            digitalWrite(STEPPER_stepsPerRotation_M0, HIGH);
            digitalWrite(STEPPER_stepsPerRotation_M1, HIGH);
            digitalWrite(STEPPER_stepsPerRotation_M2, LOW);
            break;
        case STEPS3200:
            digitalWrite(STEPPER_stepsPerRotation_M0, LOW);
            digitalWrite(STEPPER_stepsPerRotation_M1, LOW);
            digitalWrite(STEPPER_stepsPerRotation_M2, HIGH);
            break;
        case STEPS6400:
            digitalWrite(STEPPER_stepsPerRotation_M0, HIGH);
            digitalWrite(STEPPER_stepsPerRotation_M1, HIGH);
            digitalWrite(STEPPER_stepsPerRotation_M2, HIGH);
            break;
        default:
            Serial.println("BAD micro-stepping. Only 200,400,800,1600,3200,6400 allowed");
            return;
    }
    pins.currentStepsPerRotation = newStepsPerRotation;
    Serial.println("set currentStepsPerRotation to:" + String(newStepsPerRotation));
}
//
int calculateFrequency() {
    int freq = (int)(currentRPM / 60 * pins.currentStepsPerRotation);
    return freq;
}
void startStepper() {
    if (pins.MKSBoard) {
        Serial.println("START MKS stepper gradually");
        setupI2SOShiftEnableMotor();
    }
    for (int rpm = (currentRPM > 80 ? 80 : currentRPM); rpm <= currentRPM; rpm += 10) {
        double f = rpmToHertz(rpm);
        Serial.println("START STEPPER with RPM:" + String(rpm) + ", and frequency: " + String(f));
        ledcSetup(pins.StepperPwmChannel, f, UNIVERSAL_PWM_RESOLUTION);
        // delay(5);
        ledcAttachPin(pins.StepperPwmStepPin, pins.StepperPwmChannel); /* Attach the StepPin PWM Channel to the GPIO Pin */
        // delay(5);
        ledcWrite(pins.StepperPwmChannel, STEPPER_HALF_FUTY_CYCLE);
        delay(500);
        isStepperOn = true;
    }
}
double rpmToHertz(float rpm) {
    return (int)(rpm / 60 * pins.currentStepsPerRotation);  // in hertz
}
//
void stopStepper() {
    Serial.println("STOP STEPPER");
    if (pins.MKSBoard) {
        Serial.println("STOP MKS STEPPER");
        setupI2SOShiftDisableMotor();
    } else {
        Serial.println("STOP PCB STEPPER");
    }
    ledcDetachPin(pins.StepperPwmStepPin); /* Detach the StepPin PWM Channel to the GPIO Pin */
    isStepperOn = false;
    delay(200);
}

int getTemperature(float& temp, float& humid) {
    Serial.println("getTemperature called and pins..tempPin:" + String(pins.TempSensorPin));
    if (pins.UseOneWireForTemperature) {
        tempSensor.requestTemperatures();
        // Serial.print("Temperature: ");  // print the temperature in Celsius
        temp = tempSensor.getTempCByIndex(0);
        // Serial.println(temp);
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
// it seems to me that the signal from Probe is sent to the pins.StepperPwmStepPin but for enable we might? use the multiplexor
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
    digitalWrite(pins.I2SoLatchPin, LOW);
    // shift out the data (second shift register must be send first)
    shiftOut(pins.I2SoDataPin, pins.I2SoClockPin, MSBFIRST, data1);  // NOT NEEDED FOR X axes
                                                                     // Serial.print("data2");Serial.println(data2);
    shiftOut(pins.I2SoDataPin, pins.I2SoClockPin, MSBFIRST, data2);  //*
                                                                     // update the shift register output pins
    digitalWrite(pins.I2SoLatchPin, HIGH);
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
    uint16_t customDelay = analogRead(pins.PotentiometerPin);  // Reads the potentiometer
    int newRPM = map(customDelay, 0, 1023, 0, 300);            // read values of the potentiometer from 0 to 1023 into  d0->300
    return 300;
}

// WeMos D1 esp8266: D8 as standard
void displayCurrentState() {
    Serial.println("=========== Current State ============");
    Serial.println("Desired Temperature: " + String(pins.desiredTemperature));
    Serial.println("Desired RPM: " + String(currentRPM));
    Serial.println("Motor is :" + String(isStepperOn ? "ON" : "OFF"));
    Serial.println("Duration since power on is :" + getFormatedDurationSinceStart());
    Serial.println("Max Heater Duty Cycle Percentage: " + String(maxHeaterDutyCyclePercentage) + "%");
    if (pins.currentHeatingEndDurationInMinutes != -1) {
        Serial.print("Desired end time: " + String(pins.currentHeatingEndDurationInMinutes) + " minutes, ");
        Serial.println("Duration to alarm: " + String(getSecondsToAlarm() / 60) + " minutes " + String(getSecondsToAlarm() % 60) + " seconds");
    } else {
        Serial.println("No desired end time set");
    }
    Serial.println("currentStepsPerRotation: " + String(pins.currentStepsPerRotation));
}

void displayHelp() {
    Serial.println("=========== Commands help ============");
    Serial.println("t desired temp value. Like t 30 for 30 Celsius");
    Serial.println("r set RPM. Like r 80 for RPM=80");
    Serial.println("m 0 to stop motor m 1 to start motor");
    Serial.println("s reset start time to now");
    Serial.println("c Max Heater Duty Cycle Percentage. Like c 0.8 for 80 percent");
    Serial.println("e Please enter an end time in minutes. Like e 60 for 1 hour from start");
    Serial.println("p set steps per rotation. Like p 200 or 400 or 800 or 1600 or 3200 or 6400");
}
const int chipSelect = SS;

void setupSDCard() {
    Serial.print("\nInitializing SD card...");
    SPIClass hspi = SPIClass(HSPI);                              // HSPI has the 12-15 pins already configured // actually a reference
    if (!SD.begin(pins.MemoryCsPin, hspi, SPIfreq, "/sd", 2)) {  // copied from Fluid SDCard.cpp //if (SD.begin(csPin, SPI, SPIfreq, "/sd", 2)) {
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
int readTurnOnStepper() {
    lastStepperOnOffButtonState = !digitalRead(pins.StepperEnablePin);
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
//copied from their example
void setLoudness(int loudness) {
    // Loudness could be use with a mapping function, according to your buzzer or sound-producing hardware
    const int MinHardware_LOUDNESS = 0;
    const int MaxHardware_LOUDNESS = 16;
    ledcWrite(pins.SpeakerChannel, map(loudness, -4, 4, MinHardware_LOUDNESS, MaxHardware_LOUDNESS));
}
//
String getFormatedDurationSinceStart() {
    unsigned long time = (unsigned long)(millis() - pins.currentStartTime);  // finds the time since last print in milisecs
    return formatTime(time);
}
// pins.currentStartTime is in miliseconds
unsigned long getSecondsToAlarm() {
    unsigned long secondsToAlarm = pins.currentHeatingEndDurationInMinutes * 60 - (unsigned long)((millis() - pins.currentStartTime) / 1000);
    return secondsToAlarm;
}
//
void fanSetup() {
    pinMode(pins.FanPin, OUTPUT);
    ledcSetup(pins.FanPwmChannel, FanFrequency /*Hz*/, 10 /*resolution 2 power=256 values*/);
    fan(true);
    Serial.print("Fan pin ");
    Serial.print(pins.FanPin);
    Serial.print(" on channel ");
    Serial.print(pins.FanPwmChannel);
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
    if (lastFanState != on) {
        lastFanState = on;
    }
    if (on) {
        ledcAttachPin(pins.FanPin, pins.FanPwmChannel);
        ledcWrite(pins.FanPwmChannel, UNIVERSAL_MAX_DUTY_CYCLE * fanDutyCyclePercentage);
    } else {
        ledcDetachPin(pins.FanPin);
        ledcWrite(pins.FanPwmChannel, 0);
    }
    if (Debug::FAN) {
        Serial.print("Fan set to: ");
        Serial.println(on);
    }
}

void heater(bool on, int duty) {
    // this works only after setup was called to initialize the channel
    if (on && !heaterIsOn) {
        ledcWrite(pins.HeaterPwmChannel, duty);  // Heater start
        heaterIsOn = true;
    } else {
        ledcWrite(pins.HeaterPwmChannel, 0);  // Heater stop
        heaterIsOn = false;
    }
    if (Debug::HEATER) {
        Serial.print("Heater set to: ");
        Serial.print(on);
        if (on) {
            Serial.print(" with duty at:");
            Serial.print(((float)duty / UNIVERSAL_MAX_DUTY_CYCLE));
            Serial.print("%");
        }
        Serial.println();
    }
}

/** time is in miliseconds */
String formatTime(unsigned long time) {
    time = (float)time / 1000;  // convert to seconds
    String result = "";
    int hours = (unsigned long)(time / 3600);
    if (hours > 0) {
        result += hours;
        result += ("h ");
    }
    result += ((unsigned long)(time % 3600) / 60);
    result += ("m ");
    result += (time % 60);
    result += ("s");
    return result;
}
//copied from their example
void play(Melody melody) {
    if (MostMusic_OFF) {
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
            ledcWriteTone(pins.SpeakerChannel, frequency);
            setLoudness(loudness);
        } else {
            ledcWrite(pins.SpeakerChannel, 0);
        }
        delay(duration);
        // This 1 ms delay with no tone is added to let a "breathing" time between each note.
        // Without it, identical consecutives notes will sound like just one long note.
        ledcWrite(pins.SpeakerChannel, 0);
        delay(1);
    }
    ledcWrite(pins.SpeakerChannel, 0);
    delay(1000);
}

void play(Melody melody, bool force) {
    if (force) {
        play(melody);
    }
}
int calculateHeaterDutyCycle() {
    int dutyCycle = UNIVERSAL_MAX_DUTY_CYCLE * maxHeaterDutyCyclePercentage / 100;
    return dutyCycle;
}
void desiredHeatingEndTimeCheck() {
    if (pins.currentHeatingEndDurationInMinutes != -1 &&
        pins.currentHeatingEndDurationInMinutes * 60 <= getSecondsToAlarm()) {
        Serial.println("Desired end time " + String(pins.currentHeatingEndDurationInMinutes) + " reached. Current elapsed time: " + getFormatedDurationSinceStart());
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


void handleRoot() {
    String html = "<html><body><h1>Editable Table</h1><table border='1'><tr><th>Label</th><th>Value</th></tr>";
    html += "<tr><td>Item 1</td><td contenteditable='true' class='editable' onblur='updateValue(this, \"item1\")'>Value 1</td></tr>";
    html += "<tr><td>Item 2</td><td contenteditable='true' class='editable' onblur='updateValue(this, \"item2\")'>Value 2</td></tr>";
    html += "</table><script>const connection = new WebSocket('ws://' + window.location.hostname + ':81/'); function updateValue(element, item) { const value = element.innerText; connection.send(JSON.stringify({ command: 'update', item: item, value: value })); }</script></body></html>";
    server.send(200, "text/html", html);
}

void handleCommand(uint8_t num, const char* payload) {
    JsonDocument doc(1024);
    deserializeJson(doc, payload);
    String command = doc["command"];
    if (command == "update") {
        String item = doc["item"];
        String value = doc["value"];
        Serial.printf("Updated %s to %s\n", item.c_str(), value.c_str());
        // Here you can update your backing object as needed
    }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_TEXT:
            handleCommand(num, (char*)payload);
            break;
            // Handle other types if necessary
    }
}
//
void webClientSetup() {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
    server.on("/", handleRoot);
    server.begin();
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
}

//
// void oneFullRotation(){
//	Serial.print("oneFullRotation ");
//	play(validChoice);
//	setupI2SOShiftEnableMotor();
//	for(int i=0;i<(4*200)+1;i++){
//		digitalWrite(pins.StepperPwmStepPin, HIGH);
//		delay(5);
//		digitalWrite(pins.StepperPwmStepPin, LOW);
//		delay(5);
//	}
//	Serial.print("oneFullRotation end");
//	play(validChoice);
//}
