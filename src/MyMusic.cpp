#include <Melody.h>
#include <MyMusic.h>

// CHANGED: Definitions moved here from header to prevent multiple definition errors
uint8_t SPEAKER_CHANNEL = 8;
Melody scaleLouder("c>>> d>> e>f g< a<< b<<< c*<<<<", 480);
Melody invalidChoice(" (cg_)__");
Melody validChoice(" (cgc*)**---");
Melody frereJacques("(cdec)x2   (efgr)x2;//   ((gagf)-ec)x2     (c g_ c+)x2");
Melody frereJacquesFull("(cdec)x2   (efgr)x2  ((gagf)-ec)x2     (c g_ c+)x2");
Melody auClairDeLaLune("cccde+dr  ceddc+.r");
Melody darthVader(" (ggg e,-. b,-- | g e,-. b,-- g+ (ddde,-.)* b,--  | g, e,-. b,-- g+ | g* g-.g--  (g g,-. f-- (ed#)-- e-)* r- g#- c#* b#-.b-- |  (b,a)-- b,- r- e,- g, e,-. g,-- | b, g-. b,-- d*+  | g* g-.g--  (g g,-. f-- (ed#)-- e-)* r- g#- c#* b#-.b-- |  (b,a)-- b,- r- e,- g, e,-. b,-- | g e,-. b,-- g+ |)<<_ ");
Melody temp30("(ab)x3");
Melody temp37("(fg)x7");
//
void MyMusic::setLoudness(int loudness) {
    // Loudness could be use with a mapping function, according to your buzzer or sound-producing hardware
    const int MinHardware_LOUDNESS = 0;
    const int MaxHardware_LOUDNESS = 16;
    ledcWrite(SPEAKER_CHANNEL, map(loudness, -4, 4, MinHardware_LOUDNESS, MaxHardware_LOUDNESS));
}
//
void MyMusic::play(Melody melody) {
    // if (Controller::getI("MostMusicOff")) {
    //     return;
    // }
    melody.restart();         // The melody iterator is restarted at the beginning.
    while (melody.hasNext())  // While there is a next note to play.
    {
        melody.next();                                   // Move the melody note iterator to the next one.
        unsigned int frequency = melody.getFrequency();  // Get the frequency in Hz of the curent note.
        unsigned long duration = melody.getDuration();   // Get the duration in ms of the curent note.
        unsigned int loudness = melody.getLoudness();    // Get the loudness of the curent note (in a subjective relative scale from -3 to +3).
                                                         // Common interpretation will be -3 is really soft (ppp), and 3 really loud (fff).
        if (frequency > 0) {
            ledcWriteTone(SPEAKER_CHANNEL, frequency);
            setLoudness(loudness);
        } else {
            ledcWrite(SPEAKER_CHANNEL, 0);
        }
        delay(duration);
        // This 1 ms delay with no tone is added to let a "breathing" time between each note.
        // Without it, identical consecutives notes will sound like just one long note.
        ledcWrite(SPEAKER_CHANNEL, 0);
        delay(1);
    }
    ledcWrite(SPEAKER_CHANNEL, 0);
    delay(1000);
}
//
void MyMusic::play(Melody melody, bool force) {
    if (force) {
        play(melody);
    }
}