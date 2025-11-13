#pragma once
#include <ArduinoJson.h>

const char* jsonString = "{\"ssid\":\"ad5\", \"password\":\"S1tormy!\"}";
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