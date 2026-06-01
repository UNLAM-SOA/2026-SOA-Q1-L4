#ifndef MELODIES_H
#define MELODIES_H

#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_E6  1319
#define NOTE_G6  1568
#define NOTE_E7  2637
#define NOTE_C7  2093
#define NOTE_D7  2349
#define NOTE_G7  3136

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

void tareaReproducirMelodia(void* pvParameters);

#endif