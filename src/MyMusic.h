#pragma once
#include <Melody.h>

// CHANGED: extern declaration only
extern uint8_t SPEAKER_CHANNEL;

class MyMusic {
   public:
    MyMusic();
    static void play(Melody melody);
    static void play(Melody melody, bool force);
    static void setLoudness(int loudness);
};

// CHANGED: extern declarations for melodies
extern Melody scaleLouder;
extern Melody invalidChoice;
extern Melody validChoice;
extern Melody frereJacques;
extern Melody frereJacquesFull;
extern Melody auClairDeLaLune;
extern Melody darthVader;
extern Melody temp30;
extern Melody temp37;