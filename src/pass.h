#ifndef Pass_H_
#define Pass_H_
#include <ArduinoJson.h>

const char* jsonString = "{\"ssid\":\"ad5\", \"password\":\"S1tormy!\"}";
  JsonDocument doc;
const char * readSsid(){
  DeserializationError error = deserializeJson(doc, jsonString);
  if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return "";
  } else {
      return doc["ssid"];
  }
}
const char * readPassword(){
  DeserializationError error = deserializeJson(doc, jsonString);
  if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return "";
  } else {
      return doc["password"];
  }
}
#endif /* Pass_H_ */
#pragma once