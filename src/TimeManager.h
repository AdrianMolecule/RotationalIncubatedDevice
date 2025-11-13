#pragma once
#include <Arduino.h>
#include <time.h>  // Ensure time() is available

class TimeManager {
   private:
    static constexpr const char* ntpServer = "pool.ntp.org";
    static constexpr size_t TIME_STRING_BUFFER_SIZE = 64;
    static inline char ts[TIME_STRING_BUFFER_SIZE] = {0};
    static inline char bts[TIME_STRING_BUFFER_SIZE] = {0};
    static time_t dateTimeStringToTimeT(const char* dateTimeStr, const char* format);

   public:
    // Added function declarations to match the definitions in TimeManager.cpp
    static void initTime();
    static const char* getCurrentTimeAsString();
    static time_t getDesiredEndTime(const String& hours, const String& minutes);
    static String formatUnixTime(time_t unixTime);
    static int isDesiredEndTimeReached(time_t endTime_unix);
    static const char* getBootTimeAsString();
    static int checkIfHeatingDateTimeWasReached(const char* dateTimeStr);
};