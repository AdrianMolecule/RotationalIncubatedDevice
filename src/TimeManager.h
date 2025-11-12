#pragma once
#include <Arduino.h>
#include <time.h>  // Ensure time() is available
class TimeManager {
   private:
    static constexpr size_t TIME_STRING_BUFFER_SIZE = 64;
    // Initialize static members
    static inline char bts[TIME_STRING_BUFFER_SIZE] = {0};
    static int compareCurrentDateTime(const char* targetDateTimeStr, const char* formatStr);
    static time_t dateTimeStringToTimeT(const char* dateTimeStr, const char* format);

   public:
    static const char* getBootTimeAsString();
    static int checkIfHeatingDateTimeWasReached(const char* desiredHeatingEndTime);
};