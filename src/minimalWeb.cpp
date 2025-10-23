 #include <WiFi.h>
/* this works with
lib_deps =
        arduinoJson
        dualb/Melody@^1.2.0
*/
const char* password = "S1tormy!";

void setup() {
    Serial.begin(115200);
    delay(1000);

    // WiFi.mode(WIFI_STA);  // Optional
    char ssid[4];
    strcpy(ssid, "ad5");  // Requires <cstring>
    WiFi.begin(ssid, password);
    Serial.println("\nConnecting");

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
    }

    Serial.println("\nConnected to the WiFi network");
    Serial.print("Local ESP32 IP: ");
    Serial.println(WiFi.localIP());
}

void loop() {}