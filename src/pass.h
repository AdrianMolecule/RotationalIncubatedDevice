const char* jsonString = "{\"ssid\":\"ad5\", \"password\":\"S1tormy!\"}";
// const char* ssid= "ad5";
// const char* password = "S1tormy!";
  JsonDocument doc;
const char * readSsi(){

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