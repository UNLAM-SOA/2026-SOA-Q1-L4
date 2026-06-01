
#include "Melodias.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/////////////////////////////////////////////////////////////////////////////////////////
//////////// Tarea para hacer que suene la melodia, una vez se active la alarma./////////
/////////////////////////////////////////////////////////////////////////////////////////
void tareaReproducirMelodia(void* pvParameters) {
  int speakerPin = (int)(intptr_t)pvParameters;

  // Aca se puede elegir qué ringtone queremos que suene (ej: melodiaStarWars, melodiaNokia, melodiaMario, melodiaTetris)
  const int* melodiaActiva = melodiaTetris;
  int numValores = sizeof(melodiaTetris) / sizeof(melodiaTetris[0]);

  while (1) {
    for (int i = 0; i < numValores; i += 2) {
      int nota = melodiaActiva[i];
      int duracion = melodiaActiva[i + 1];

      if (nota == 0) {
        noTone(speakerPin);
      } else {
        tone(speakerPin, nota);
      }

      // Cedemos el procesador mientras suena la nota
      vTaskDelay(pdMS_TO_TICKS(duracion));
      noTone(speakerPin);
      vTaskDelay(pdMS_TO_TICKS(50));
    }
    vTaskDelay(pdMS_TO_TICKS(1000)); // Pausa antes de repetir
  }
}