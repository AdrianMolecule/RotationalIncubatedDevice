#include "TimeManager.h"

#include <MyMusic.h>
#include <WiFi.h>

const long gmtOffset_sec = -14400;  // Adjusted from 3600 to -14400 (4 hours back for EST)
const int daylightOffset_sec = 3600;

void TimeManager::initTime() {
    // If connection is not active, configTime will fail
    if (WiFi.status() != WL_CONNECTED) {
        MyMusic::MajorAlarm("WiFi NOT Connected - Time Sync Failed.");
    }
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

const char* TimeManager::getCurrentTimeAsString() {
    struct tm timeinfo;
    // Check if time is available
    if (!getLocalTime(&timeinfo)) {
        MyMusic::MajorAlarm("NTP Time Sync Failed - No Network Time.");
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

/** * Checks if the current time has reached or passed the time specified in dateTimeStr (HH:MM).
 * @param dateTimeStr The desired time in "HH:MM" format.
 * @return 1 if reached, 0 if not yet reached, -1 on error/not set.
 */
int TimeManager::checkIfHeatingDateTimeWasReached(const char* dateTimeStr) {
    if (dateTimeStr == nullptr || dateTimeStr[0] == '\0') {
        Serial.println(" checkIfHeatingDateTimeWasReached 1");
        return -1;
    }
    struct tm timeinfo;
    // Get the current local time. Returns false if time hasn't been synced (e.g., no WiFi/NTP error).
    if (!getLocalTime(&timeinfo)) {
        Serial.println(" checkIfHeatingDateTimeWasReached 2");
        return -1;
    }
    // Attempt to parse HH and MM from the string (e.g., "15:30")
    int desiredHour, desiredMinute;
    if (sscanf(dateTimeStr, "%d:%d", &desiredHour, &desiredMinute) != 2) {
        return -1;  // Format error
    }
    // Create a time structure for the desired end time, using TODAY's date/time
    // fields from the current time, but replacing the hour and minute.
    struct tm desiredTimeinfo = timeinfo;
    desiredTimeinfo.tm_hour = desiredHour;
    desiredTimeinfo.tm_min = desiredMinute;
    desiredTimeinfo.tm_sec = 0;
    // Convert both structures to UNIX timestamps (seconds since epoch) for comparison
    time_t currentTime_unix = mktime(&timeinfo);
    time_t desiredTime_unix = mktime(&desiredTimeinfo);
    // Check if the current time is greater than or equal to the desired time
    Serial.println(" checkIfHeatingDateTimeWasReached 3");
    if (currentTime_unix >= desiredTime_unix) {
        Serial.println(" checkIfHeatingDateTimeWasReached 4");
        return 1;  // Heating end time reached
    } else {
        Serial.println(" checkIfHeatingDateTimeWasReached 5");
        return 0;  // Not yet reached
    }
}