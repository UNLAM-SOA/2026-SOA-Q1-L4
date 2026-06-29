
#include "Melodias.h"
#include "header.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const int* const catalogoMelodias[] = {
  melodiaStarWars,
  melodiaNokia,
  melodiaMario,
  melodiaTetris,
};

static const int catalogoTamanios[] = {
  sizeof(melodiaStarWars) / sizeof(melodiaStarWars[0]),
  sizeof(melodiaNokia)    / sizeof(melodiaNokia[0]),
  sizeof(melodiaMario)    / sizeof(melodiaMario[0]),
  sizeof(melodiaTetris)   / sizeof(melodiaTetris[0]),
};

static const uint8_t TOTAL_MELODIAS = sizeof(catalogoMelodias) / sizeof(catalogoMelodias[0]);

void tareaReproducirMelodia(void* pvParameters) {
  int speakerPin = (int)(intptr_t)pvParameters;

  while (1) {
    uint8_t idx = 0;
    if (xSemaphoreTake(xMelodiaSeleccionadaMutex, portMAX_DELAY) == pdTRUE) {
      idx = melodiaSeleccionada < TOTAL_MELODIAS ? melodiaSeleccionada : 0;
      xSemaphoreGive(xMelodiaSeleccionadaMutex);
    }

    const int* melodiaActiva = catalogoMelodias[idx];
    int numValores = catalogoTamanios[idx];

    for (int i = 0; i < numValores; i += 2) {
      int nota     = melodiaActiva[i];
      int duracion = melodiaActiva[i + 1];

      if (nota == 0) {
        noTone(speakerPin);
      } else {
        tone(speakerPin, nota);
      }

      vTaskDelay(pdMS_TO_TICKS(duracion));
      noTone(speakerPin);
      vTaskDelay(pdMS_TO_TICKS(50));
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}