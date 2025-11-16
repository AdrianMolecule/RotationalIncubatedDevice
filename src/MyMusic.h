#pragma once
#include <Melody.h>

extern const uint8_t SPEAKER_CHANNEL;
#define DNS "os"

class MyMusic {
    public:
    static inline Melody scaleLouder = Melody("c>>> d>> e>f g< a<< b<<< c*<<<<", 480);
    static inline Melody invalidChoice = Melody(" (cg_)__");
    static inline Melody validChoice = Melody(" (cgc*)**---");
    static inline Melody frereJacques = Melody("(cdec)x2   (efgr)x2;//   ((gagf)-ec)x2     (c g_ c+)x2");
    static inline Melody frereJacquesFull = Melody("(cdec)x2   (efgr)x2  ((gagf)-ec)x2     (c g_ c+)x2");
    static inline Melody auClairDeLaLune = Melody("cccde+dr  ceddc+.r");
    static inline Melody darthVader = Melody(" (ggg e,-. b,-- | g e,-. b,-- g+ (ddde,-.)* b,--  | g, e,-. b,-- g+ | g* g-.g--  (g g,-. f-- (ed#)-- e-)* r- g#- c#* b#-.b-- |  (b,a)-- b,- r- e,- g, e,-. g,-- | b, g-. b,-- d*+  | g* g-.g--  (g g,-. f-- (ed#)-- e-)* r- g#- c#* b#-.b-- |  (b,a)-- b,- r- e,- g, e,-. b,-- | g e,-. b,-- g+ |)<<_ ");
    static inline Melody temp30 = Melody("(ab)x3");
    static inline Melody temp37 = Melody("(fg)x7");
    static inline Melody fatalErrorAlarmMusic = Melody("c*64 b64 a64 g64 c*64 b64 a64 g64 (c*16g-16)x4", 320);
    static inline Melody errorAlarmMusic = Melody("(g-32 e-32)x4", 380);
    static inline Melody warningAlarmMusic = Melody("(e16 g16)x2 r8", 480);
    static inline Melody infoAlarmMusic = Melody("(c8 e8 g8) r8", 600);

    MyMusic();
    static void play(Melody melody);
    static void play(Melody melody, bool force);
    static void setLoudness(int loudness);
};
