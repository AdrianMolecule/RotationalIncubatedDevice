#include "Helper.h"

// Default fields with all data members: name, type, value, description, id, readOnly
std::vector<Field> Helper::defaultFields() {
    return std::vector<Field>{
        Field("Speed", "float", "0.0", "Motor speed in RPM", "f1", false),
        Field("Duration", "int", "10", "Operation duration in seconds", "f2", false),
        Field("Enabled", "bool", "true", "Enable the system", "f3", false),
        Field("Mode", "string", "Auto", "Operation mode", "f4", false)
    };
}

// Generate a simple top menu for navigation
String Helper::generateMenu() {
    return "<nav>"
           "<a href='/'>Home</a> | "
           "<a href='/info'>Info</a> | "
           "<a href='/metadata'>Metadata</a> | "
           "<a href='/debug'>Debug</a>"
           "</nav><hr/>";
}

// Standard HTML header with a title
String Helper::htmlHeader(const String& title) {
    return "<!DOCTYPE html><html><head>"
           "<meta charset='UTF-8'>"
           "<title>" + title + "</title>"
           "<style>"
           "body { font-family: Arial, sans-serif; }"
           "table { border-collapse: collapse; width: 100%; }"
           "th, td { border: 1px solid #ccc; padding: 8px; text-align: left; }"
           "th { background-color: #f2f2f2; }"
           "input, select { width: 90%; }"
           "</style>"
           "</head><body>";
}

// Standard HTML footer
String Helper::htmlFooter() {
    return "<hr/><footer><p>ESP32 Web Controller</p></footer></body></html>";
}
