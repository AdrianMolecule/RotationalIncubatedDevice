#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <WiFi.h>

#include "DHTesp.h"  //for DHT temp sensor
#include "pass.h"  //for DHT temp sensor



// WebSocket server
WebSocketsServer webSocket = WebSocketsServer(81);  // From Display to Board motors and new desired values
// AsyncWebServer server(80);                          // From Board sensors to Display
DHTesp dhTempSensor;
// Motor control pins
const int motor1Pin = 5;
const int motor2Pin = 6;

// Function declarations
float readPH();
void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);

// HTML page to serve
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Temperature Display</title>
    <script>
        let socket;
        function setupWebSocket() {
            socket = new WebSocket('ws://ESP32_IP_ADDRESS:81');
            socket.onmessage = function(event) {
                document.getElementById('temperature').innerText = 'Temperature: ' + event.data + ' °C';
            };
            socket.onopen = function() {
                console.log('WebSocket connection established');
            };
            socket.onclose = function() {
                console.log('WebSocket connection closed');
            };
        }
        window.onload = setupWebSocket;
    </script>
</head>
<body>
    <h1>ESP32 Temperature Display</h1>
    <div id="temperature">Temperature: -- °C</div>
</body>
</html>
)rawliteral";

void setup() {
    Serial.begin(115200);
    delay(1000);
    // JsonDocument doc;
    // DeserializationError error = deserializeJson(doc, jsonString);
    // if (error) {
    //     Serial.print(F("deserializeJson() failed: "));
    //     Serial.println(error.f_str());
    // } else {
    //     ssid = doc["ssid"].as<const char*>();
    //     password = doc["password"].as<const char*>();
    //     ;
    // }
    Serial.print("ssid: " + String(ssid));
    Serial.println("password: "+ String(password));
    // WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
    }
    Serial.println("Connected to WiFi");
    Serial.println("local IP: " + WiFi.localIP().toString());
    Serial.println("ws://" + WiFi.localIP().toString() + ":81");
    // WebSocket setup
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
    // htmlPage.replace("ESP32_IP_ADDRESS", WiFi.localIP().toString().c_str());
    /*     server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
            request->send(200, "text/html", htmlPage);  // from board to display sends updated HTML to the display
        }); */

    // Start server
    // server.begin(); todo
    // // DHT sensor setup
    // dhTempSensor.setup(255, DHTesp::DHT22);
    // if (dhTempSensor.getStatus() == DHTesp::ERROR_TIMEOUT) {
    //     Serial.println("No DHT22 found or not working properly!");
    // }
    // // Motor pin setup
    // pinMode(motor1Pin, OUTPUT);
    // pinMode(motor2Pin, OUTPUT);
}

String oldData = "nothing";

void loop() {
    webSocket.loop();
    // // Read temperature and pH
    // // float temperature = dhTempSensor.getTemperature();
    // // float pHValue = readPH();  // Implement this function based on your pH sensor
    // // Prepare data string
    // //String data = String(temperature) + "," + String(pHValue);
    // String data = String(77) + "," + String(88);
    // // Broadcast data
    // if(data.equalsIgnoreCase(oldData)) {
    //     return;
    // }
    // webSocket.broadcastTXT(data);
    // delay(1000);  // Send data every 1 seconds
}

// WebSocket event handler so data from display goes to the board
void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    String message = String((char*)payload);
    Serial.println(message);
    /*  // Motor control commands
     if (message == "motor1_on") {
         digitalWrite(motor1Pin, HIGH);
     } else if (message == "motor1_off") {
         digitalWrite(motor1Pin, LOW);
     } else if (message == "motor2_on") {
         digitalWrite(motor2Pin, HIGH);
     } else if (message == "motor2_off") {
         digitalWrite(motor2Pin, LOW);
     } */
}

// Placeholder for pH reading
float readPH() {
    // Implement your pH reading logic here
    return 7.0;  // Placeholder value
}
