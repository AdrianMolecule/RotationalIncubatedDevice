#pragma once
#include <ArduinoJson.h>

// on router go to Basic/lanSetup/dhcp reservation and click add reservation and choose 192.168.0.201 for bigOS 192.168.0.202 for TR and 192.168.0.203 for small OS
const char* jsonString = "{\"ssid\":\"biohub\", \"password\":\"yourpass\"}";
  JsonDocument doc;
const char * getSsid(){
  DeserializationError error = deserializeJson(doc, jsonString);
  if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return "";
  } else {
      return doc["ssid"];
  }
}
const char * getPass(){
  DeserializationError error = deserializeJson(doc, jsonString);
  if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return "";
  } else {
      return doc["password"];
  }
}