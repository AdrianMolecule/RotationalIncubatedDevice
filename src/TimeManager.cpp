#include "TimeManager.h"

#include <Arduino.h>  // For Serial.printf (assuming this context)

#include <cstring>
// #include <Controller.h>

// Helper to convert a string (e.g., "2025-12-01 14:30:00") and its format to a numeric time_t value.
time_t TimeManager::dateTimeStringToTimeT(const char* dateTimeStr, const char* format) {
    struct tm timeinfo = {0};
    // strptime reads the string into the tm structure based on the format.
    if (strptime(dateTimeStr, format, &timeinfo) == NULL) {
        Serial.printf("Error parsing date-time string: %s with format %s\n", dateTimeStr, format);
        return (time_t)-1;
    }
    // MODIFICATION: Set seconds to zero to normalize the comparison time.
    timeinfo.tm_sec = 0;
    // mktime converts the tm structure (local time) into a numeric time_t timestamp.
    return mktime(&timeinfo);
}
//
int TimeManager::checkIfHeatingDateTimeWasReached(const char* desiredHeatingEndTime) {
    if (std::strcmp(desiredHeatingEndTime, "-1") == 0) {
        return -1;
    }
    time_t endTime_unix = TimeManager::dateTimeStringToTimeT(desiredHeatingEndTime, "%Y-%m-%d %H:%M:%S");
    if (endTime_unix == -1) {
        Serial.printf("!!!!!! desiredHeatingEndTime:%f in incorrect format, needs %Y-%m-%d %H:%M:%S\n", desiredHeatingEndTime);
        return -1;  // something wrong
    }
    struct tm timeinfo;
    const char* formatStr = "2025-11-12 13:00:00";
    if (!getLocalTime(&timeinfo)) {
        Serial.println("!!!! Error: Failed to get current time.");
        return -1;
    }
    // Convert the normalized current time structure to a numeric time_t timestamp.
    time_t currentTime_normalized = mktime(&timeinfo);
    if (currentTime_normalized < 100000) {
        Serial.println("Error: Current time not yet synchronized.");
        return -1;
    }
    // 3. Comparison Logic (now comparing normalized timestamps)
    if (currentTime_normalized < endTime_unix) {
        return 0;  // Current time is before target time
    } else
        return 1;  // Current time is equal or after target time}
}
//
const char* TimeManager::getBootTimeAsString() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        const char* errorMsg = "-1";
        strncpy(bts, errorMsg, TIME_STRING_BUFFER_SIZE);
        bts[TIME_STRING_BUFFER_SIZE - 1] = '\0';
        return bts;
    }
    time_t bootTime_unix = mktime(&timeinfo);  // time_t is long
    size_t result = strftime(bts, TIME_STRING_BUFFER_SIZE, "%a, %b %d %Y, %H:%M:%S", &timeinfo);
    if (result == 0) {
        const char* errorMsg = "-1";
        strncpy(bts, errorMsg, TIME_STRING_BUFFER_SIZE);
        bts[TIME_STRING_BUFFER_SIZE - 1] = '\0';
    }
    return bts;
}