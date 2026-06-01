#include "FSM.h"
#include <Melodias.h>
#include <Sensores.h>

// ── Acciones de la FSM ────────────────────────────────────────────────────
static void guardaPosicion() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  lastX = a.acceleration.x;
  lastY = a.acceleration.y;
  lastZ = a.acceleration.z;
}

static void prender() {
  Serial.println(">>> SISTEMA ARMADO");
  guardaPosicion();
  estadoActual = ACTIVO;
}

static void advertirMovimiento() {
  Serial.println(">>> ADVERTENCIA: Movimiento detectado");
  contadorAdvertencias++;
  timerReiniciadoEnAdvertencia = false;
  digitalWrite(LED_PIN, HIGH);
  iniciarTimeoutAdvertencia();
  estadoActual = ADVERTENCIA_MOVIMIENTO;
}

static void alertar() {
  Serial.println("¡ALERTA! Movimiento Y contacto detectados");
  cancelarTimeoutAdvertencia();
  digitalWrite(LED_PIN, HIGH);

  if (xAlarmaTask == nullptr) {
    xTaskCreate(tareaReproducirMelodia, "MelodiaAlarma", TAM_PILA, (void*)(intptr_t)SPEAKER_PIN, PRIORIDAD, &xAlarmaTask);
  }

  estadoActual = ALERTA;
}

static void apagar() {
  Serial.println(">>> SISTEMA DESARMADO");
  contadorAdvertencias = 0;
  cancelarTimeoutAdvertencia();

  if (estadoActual == ALERTA) {
    if (xAlarmaTask != nullptr) {
      vTaskDelete(xAlarmaTask);
      xAlarmaTask = nullptr;
    }
    noTone(SPEAKER_PIN);
  }

  digitalWrite(LED_PIN, LOW);
  estadoActual = APAGADO;
}

static void manejarMovDuranteAdvertencia() {
  if (!timerReiniciadoEnAdvertencia) {
    Serial.println(">>> Movimiento reiterado: extendiendo advertencia");
    timerReiniciadoEnAdvertencia = true;
    iniciarTimeoutAdvertencia();
  } else {
    Serial.println(">>> Movimiento persistente: escalando a ALERTA");
    alertar();
  }
}

static void reiniciarAdvertencia() {
  if (contadorAdvertencias >= UMBRAL_ADVERTENCIAS) {
    Serial.println(">>> Advertencias repetidas: escalando a ALERTA");
    alertar();
    return;
  }
  Serial.println(">>> Timeout: volviendo a ACTIVO");

  int hallActual = analogRead(PIN_HALL);
  Serial.println("Valor hall: ");
  Serial.println(hallActual);

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  Serial.println("Valor x");
  Serial.println(a.acceleration.x);
  Serial.println("Valor y");
  Serial.println(a.acceleration.y);
  Serial.println("Valor z");
  Serial.println(a.acceleration.z);

  contadorAdvertencias = 0;
  digitalWrite(LED_PIN, LOW);
  estadoActual = ACTIVO;
}

static void errorTransicion() {
  Serial.println("Transicion invalida");
}

// ── Matriz de transición ──────────────────────────────────────────────────
// Eventos: APAGAR, PRENDER, MOV_DETECTADO, TOUCH_DETECTADO, TIMEOUT_ADVERTENCIA
static const Accion MATRIZ_TRANSICION[TOTAL_ESTADOS][TOTAL_EVENTOS] = {
  { errorTransicion, prender,                       errorTransicion,              errorTransicion, errorTransicion      }, // APAGADO
  { apagar,          errorTransicion,               advertirMovimiento,           alertar,         errorTransicion      }, // ACTIVO
  { apagar,          errorTransicion,               manejarMovDuranteAdvertencia, alertar,         reiniciarAdvertencia }, // ADVERTENCIA_MOVIMIENTO
  { apagar,          errorTransicion,               errorTransicion,              errorTransicion, errorTransicion      }  // ALERTA
};

// ── Núcleo FSM ────────────────────────────────────────────────────────────
extern EventDetector eventType[MAX_TYPE_EVENTS];

static bool getNuevoEvento() {
  short index = lastIndexTypeSensor % MAX_TYPE_EVENTS;
  lastIndexTypeSensor++;
  return eventType[index]();
}

void procesarEvento() {
  if (getNuevoEvento()) {
    if ((newEvent >= 0) && (newEvent < TOTAL_EVENTOS) &&
        (estadoActual >= 0) && (estadoActual < TOTAL_ESTADOS)) {
      MATRIZ_TRANSICION[estadoActual][newEvent]();
    }
  }
}
