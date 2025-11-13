#include "MyMusic.h"

#include <Melody.h>

const uint8_t SPEAKER_CHANNEL = 8;
const int MinHardware_LOUDNESS = 0;
const int MaxHardware_LOUDNESS = 16;

Melody scaleLouder("c>>> d>> e>f g< a<< b<<< c*<<<<", 480);
Melody invalidChoice(" (cg_)__");
Melody validChoice(" (cgc*)**---");
Melody frereJacques("(cdec)x2   (efgr)x2;//   ((gagf)-ec)x2     (c g_ c+)x2");
Melody frereJacquesFull("(cdec)x2   (efgr)x2  ((gagf)-ec)x2     (c g_ c+)x2");
Melody auClairDeLaLune("cccde+dr  ceddc+.r");
Melody darthVader(" (ggg e,-. b,-- | g e,-. b,-- g+ (ddde,-.)* b,--  | g, e,-. b,-- g+ | g* g-.g--  (g g,-. f-- (ed#)-- e-)* r- g#- c#* b#-.b-- |  (b,a)-- b,- r- e,- g, e,-. g,-- | b, g-. b,-- d*+  | g* g-.g--  (g g,-. f-- (ed#)-- e-)* r- g#- c#* b#-.b-- |  (b,a)-- b,- r- e,- g, e,-. b,-- | g e,-. b,-- g+ |)<<_ ");
Melody temp30("(ab)x3");
Melody temp37("(fg)x7");
Melody FatalError("c*64 b64 a64 g64 c*64 b64 a64 g64 (c*16g-16)x4", 320);

// CHANGED: Implementation now uses class scope
void MyMusic::MajorAlarm(const char* message) {
    Serial.print("MAJOR ALARM: ");
    Serial.println(message);
    MyMusic::play(FatalError);
}

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