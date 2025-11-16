#include "TimeManager.h"

#include <Controller.h>
#include <MyMusic.h>
#include <WiFi.h>

const long gmtOffset_sec = -14400;  // Adjusted from 3600 to -14400 (4 hours back for EST)
const int daylightOffset_sec = 3600;

void TimeManager::initTime() {
    // If connection is not active, configTime will fail
    if (WiFi.status() != WL_CONNECTED) {
        MyMusic::FatalErrorAlarm("WiFi NOT Connected - Time Sync Failed.");
    }
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

const char* TimeManager::getCurrentTimeAsString() {
    struct tm timeinfo;
    // Check if time is available
    if (!getLocalTime(&timeinfo)) {
        MyMusic::FatalErrorAlarm("NTP Time Sync Failed - No Network Time.");
        const char* errorMsg = "Time Not Synced";
        strncpy(ts, errorMsg, TIME_STRING_BUFFER_SIZE);
        ts[TIME_STRING_BUFFER_SIZE - 1] = '\0';
        return ts;
    }
    size_t result = strftime(ts, TIME_STRING_BUFFER_SIZE, "%a, %b %d %Y, %H:%M:%S", &timeinfo);
    if (result == 0) {
        const char* errorMsg = "-1";
        strncpy(ts, errorMsg, TIME_STRING_BUFFER_SIZE);
        ts[TIME_STRING_BUFFER_SIZE - 1] = '\0';
    }
    return ts;
}

time_t TimeManager::getDesiredEndTime(const String& hours, const String& minutes) {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return 0;
    time_t currentTime_unix = mktime(&timeinfo);
    int h = hours.toInt();
    int m = minutes.toInt();
    if (h == 0 && m == 0) return 0;
    long timeToAdd_sec = (long)h * 3600 + (long)m * 60;
    return currentTime_unix + timeToAdd_sec;
}

String TimeManager::formatUnixTime(time_t unixTime) {
    if (unixTime == 0) return "Not Set";
    struct tm* tm_info;
    tm_info = localtime(&unixTime);
    size_t result = strftime(ts, TIME_STRING_BUFFER_SIZE, "%a, %b %d %Y, %H:%M:%S", tm_info);
    if (result == 0) return "-1";
    return String(ts);
}

int TimeManager::isDesiredEndTimeReached(time_t endTime_unix) {
    if (endTime_unix == 0) return -1;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return -2;
    time_t currentTime_unix = mktime(&timeinfo);
    if (currentTime_unix < endTime_unix) {
        return 0;
    } else
        return 1;
}

const char* TimeManager::getBootTimeAsString() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        const char* errorMsg = "-1";
        strncpy(bts, errorMsg, TIME_STRING_BUFFER_SIZE);
        bts[TIME_STRING_BUFFER_SIZE - 1] = '\0';
        return bts;
    }
    time_t bootTime_unix = mktime(&timeinfo);
    size_t result = strftime(bts, TIME_STRING_BUFFER_SIZE, "%a, %b %d %Y, %H:%M:%S", &timeinfo);
    if (result == 0) {
        const char* errorMsg = "-1";
        strncpy(bts, errorMsg, TIME_STRING_BUFFER_SIZE);
        bts[TIME_STRING_BUFFER_SIZE - 1] = '\0';
    }
    return bts;
}  // In src/TimeManager.cpp (add this function implementation):

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

int TimeManager::checkIfHeatingDateTimeWasReached(const char* desiredHeatingEndTime) {
    if (strcmp(desiredHeatingEndTime, "-1") == 0) {
        return -1;
    }
    time_t endTime_unix = TimeManager::dateTimeStringToTimeT(desiredHeatingEndTime, "%Y-%m-%d %H:%M:%S");
    if (endTime_unix == -1) {
        //Serial.printf("!!!!!! desiredHeatingEndTime:%f in incorrect format, needs %Y-%m-%d %H:%M:%S\n", desiredHeatingEndTime);
        return -1;  // something wrong
    }
    struct tm timeinfo;
    const char* formatStr = "2025-11-12 13:00:00";
    if (!getLocalTime(&timeinfo)) {
        MyMusic::FatalErrorAlarm("cannot get current time from wifi in checkIfHeatingDateTimeWasReached.");
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
