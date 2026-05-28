#ifndef MELODIES_H
#define MELODIES_H

#include "pitches.h"

// 1. Star Wars (Marcha Imperial)
const int melodiaStarWars[] = {
  NOTE_A4, 500, NOTE_A4, 500, NOTE_A4, 500, NOTE_F4, 350, NOTE_C5, 150,
  NOTE_A4, 500, NOTE_F4, 350, NOTE_C5, 150, NOTE_A4, 1000
};

// 2. Ringtone clásico de Nokia
const int melodiaNokia[] = {
  NOTE_E5, 125, NOTE_D5, 125, NOTE_FS4, 250, NOTE_GS4, 250,
  NOTE_CS5, 125, NOTE_B4, 125, NOTE_D4, 250, NOTE_E4, 250
};

// 3. Super Mario Bros (Sonido de 1-Up)
const int melodiaMario[] = {
  NOTE_E6, 125, NOTE_G6, 125, NOTE_E7, 125, NOTE_C7, 125, NOTE_D7, 125, NOTE_G7, 125
};

// 4. Tetris (Intro Theme)
const int melodiaTetris[] = {
  NOTE_E5, 250, NOTE_B4, 125, NOTE_C5, 125, NOTE_D5, 250,
  NOTE_C5, 125, NOTE_B4, 125, NOTE_A4, 250, NOTE_A4, 125, NOTE_C5, 125, NOTE_E5, 250
};

#endif