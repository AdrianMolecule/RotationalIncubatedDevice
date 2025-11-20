#pragma once
#include <Melody.h>

extern const uint8_t SPEAKER_CHANNEL;
#define DNS "custom os"

class MyMusic {
    public:
    static inline Melody scaleLouder = Melody("c>>> d>> e>f g< a<< b<<< c*<<<<", 480);
    static inline Melody invalidChoice = Melody(" (cg_)__");
    static inline Melody validChoice = Melody(" (cgc*)**---");
    static inline Melody darthVader = Melody(" (ggg e,-. b,-- | g e,-. b,-- g+ (ddde,-.)* b,--  | g, e,-. b,-- g+ | g* g-.g--  (g g,-. f-- (ed#)-- e-)* r- g#- c#* b#-.b-- |  (b,a)-- b,- r- e,- g, e,-. g,-- | b, g-. b,-- d*+  | g* g-.g--  (g g,-. f-- (ed#)-- e-)* r- g#- c#* b#-.b-- |  (b,a)-- b,- r- e,- g, e,-. b,-- | g e,-. b,-- g+ |)<<_ ");
    static inline Melody wakeUp = Melody("c<<<<");
    static inline Melody wifi = Melody("ag<<<<");
    static inline Melody backend = Melody("ff<<<<");
    static inline Melody backendend = Melody("g<<<<");
    static inline Melody loop = Melody("abf<<<<");
    static inline Melody tempUnder37 = Melody("(ga<<<<)x3");
    static inline Melody temp37 = Melody("(fg<<<<)x3");
    static inline Melody processFinished = Melody("c<<<<8 e<<<<8", 600);
     static inline Melody fatalErrorAlarmMusic = Melody("c*64 b64 a64 g64 c*64 b64 a64 g64 (c*16g-16)x4", 320);
    static inline Melody errorAlarmMusic = Melody("(g-32 e-32)x4", 380);
    static inline Melody warningAlarmMusic = Melody("(e16 g16)x2 r8", 480);
    static inline Melody infoAlarmMusic = Melody("(c8 e8 g8) r8", 600);

    MyMusic();
    static void play(Melody melody);
    static void play(Melody melody, bool force);
    static void setLoudness(int loudness);
};
