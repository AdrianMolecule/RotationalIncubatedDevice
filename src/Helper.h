#pragma once
#include <Arduino.h>
#include <time.h>

class Helper {
   public:
    Helper() = default;

    static String getTime() {
        struct tm timeinfo;
        // getLocalTime fills the 'timeinfo' structure.
        // It returns true on success, false on failure (e.g., no Wi-Fi/NTP sync issue)
        if (!getLocalTime(&timeinfo)) {
            return "Failed to obtain time";
        }
        // A buffer is required to hold the formatted C-style string temporarily
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%A, %b %d %Y %I:%M:%S %p", &timeinfo);
        return String(buffer);
    }
};
