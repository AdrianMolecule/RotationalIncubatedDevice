#pragma once
#include <Arduino.h>
#include <time.h>  // Ensure time() is available

class TimeManager {
   private:
    static constexpr const char* ntpServer = "pool.ntp.org";
    static constexpr size_t TIME_STRING_BUFFER_SIZE = 64;
    // The use of 'static inline' means these members are fully defined here,
    // which resolves the 'is not a static data member' and 'redefinition' errors
    // from TimeManager.cpp.
    static inline char ts[TIME_STRING_BUFFER_SIZE] = {0};
    static inline char bts[TIME_STRING_BUFFER_SIZE] = {0};

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