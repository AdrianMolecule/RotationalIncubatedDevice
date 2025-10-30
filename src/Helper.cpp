#include "Helper.h"

std::vector<Field> Helper::defaultFields() {
    return {
        Field("1","Temp", "float", "23.4","d", false),
        Field("2","Hum", "float", "45.0", "d",false),
        Field("3","DeviceName", "string", "ESP32", "d",false),
        Field("4","Enabled", "bool", "true", "d",false),
        Field("5","LEDState", "bool", "false","d", false)
    };
}
