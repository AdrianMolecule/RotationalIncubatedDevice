#pragma once
#include <Melody.h>

extern const uint8_t SPEAKER_CHANNEL;
#define DNS "os"

class MyMusic {
   public:
    MyMusic();
    static void play(Melody melody);
    static void play(Melody melody, bool force);
    static void setLoudness(int loudness);
    static void ErrorAlarm(const char* message, bool setControllerError=true);
    static void FatalErrorAlarm(const char* message, bool setControllerError=true);
    static void WarningAlarm(const char* message, bool setControllerError = true);
};

// extern declarations for melodies
extern Melody scaleLouder;
extern Melody invalidChoice;
extern Melody validChoice;
extern Melody frereJacques;
extern Melody frereJacquesFull;
extern Melody auClairDeLaLune;
extern Melody darthVader;
extern Melody temp30;
extern Melody temp37;
extern Melody fatalErrorAlarm;
extern Melody errorAlarm;
extern Melody warningAlarm;

