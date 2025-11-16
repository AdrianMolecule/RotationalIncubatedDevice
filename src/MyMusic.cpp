#include <Melody.h>
//
#include "MyMusic.h"

const uint8_t SPEAKER_CHANNEL = 8;
const int MinHardware_LOUDNESS = 0;
const int MaxHardware_LOUDNESS = 16;


//
void MyMusic::setLoudness(int loudness) {
    ledcWrite(SPEAKER_CHANNEL, map(loudness, -4, 4, MinHardware_LOUDNESS, MaxHardware_LOUDNESS));
}
void MyMusic::play(Melody melody) {
    melody.restart();
    while (melody.hasNext()) {
        melody.next();
        unsigned int frequency = melody.getFrequency();
        unsigned long duration = melody.getDuration();
        unsigned int loudness = melody.getLoudness();
        if (frequency > 0) {
            ledcWriteTone(SPEAKER_CHANNEL, frequency);
            setLoudness(loudness);
        } else {
            ledcWrite(SPEAKER_CHANNEL, 0);
        }
        delay(duration);
        ledcWrite(SPEAKER_CHANNEL, 0);
        delay(1);
    }
    ledcWrite(SPEAKER_CHANNEL, 0);
    delay(1000);
}
void MyMusic::play(Melody melody, bool force) {
    if (force) {
        play(melody);
    }
}