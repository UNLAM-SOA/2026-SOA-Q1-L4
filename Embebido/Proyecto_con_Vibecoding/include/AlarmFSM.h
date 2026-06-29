/**
 * AlarmFSM.h
 * -----------------------------------------------------------------------------
 * Maquina de Estados Finitos (FSM) del dispositivo antirrobos.
 *
 * IMPORTANTE: esta clase es C++ PURO, sin ninguna dependencia de Arduino ni de
 * FreeRTOS. Eso permite:
 *   1) Compilarla y testearla en la PC (ver carpeta test/).
 *   2) Mantener la logica de transiciones separada del hardware.
 *
 * El firmware (main.cpp) traduce las entradas fisicas (boton, sensores, MQTT)
 * a eventos de la FSM y reacciona ante los cambios de estado encendiendo el
 * LED, el buzzer y publicando por MQTT.
 *
 * Estados y transiciones se corresponden EXACTAMENTE con el diagrama de estados
 * final del proyecto (Diagrama-de-estados.png):
 *
 *   [*] -> APAGADO
 *   APAGADO     --PRENDER(1)--------> ACTIVO
 *   ACTIVO      --MOVIMIENTO(3)-----> ADVERTENCIA
 *   ACTIVO      --MAGNETICO(2)------> ALERTADO
 *   ACTIVO      --APAGAR(5)---------> APAGADO
 *   ADVERTENCIA --TIMEOUT_MOV(4)----> ACTIVO
 *   ADVERTENCIA --MOVIMIENTO(3)-----> ALERTADO
 *   ADVERTENCIA --MAGNETICO(2)------> ALERTADO
 *   ADVERTENCIA --APAGAR(5)---------> APAGADO
 *   ALERTADO    --APAGAR(5)---------> APAGADO
 * -----------------------------------------------------------------------------
 */
#ifndef ALARM_FSM_H
#define ALARM_FSM_H

#include <cstdint>

// Estados posibles del sistema (segun el diagrama de estados).
enum class Estado : uint8_t {
    APAGADO = 0,   // Reposo / desarmado
    ACTIVO,        // Armado y vigilando
    ADVERTENCIA,   // Movimiento detectado: alta vigilancia por una ventana de tiempo
    ALERTADO       // Alarma disparada
};

// Eventos que la FSM sabe procesar. El numero entre parentesis corresponde a
// la etiqueta del diagrama de estados.
enum class Evento : uint8_t {
    PRENDER = 1,       // (1) Armar el sistema
    MAGNETICO = 2,     // (2) Sensor magnetico/contacto detectado
    MOVIMIENTO = 3,    // (3) Movimiento brusco detectado
    TIMEOUT_MOV = 4,   // (4) Venció la ventana de advertencia sin novedad
    APAGAR = 5         // (5) Desarmar / apagar
};

class AlarmFSM {
public:
    AlarmFSM() : estado_(Estado::APAGADO) {}

    // Estado actual.
    Estado estado() const { return estado_; }

    // Procesa un evento y aplica la transicion correspondiente.
    // Devuelve el estado resultante. Si el evento no aplica al estado actual,
    // el estado no cambia (transicion ignorada).
    Estado procesar(Evento evento);

    // Devuelve true si el evento provoca un cambio de estado desde el estado
    // actual (util para tests y para decidir acciones). No modifica la FSM.
    bool provocaTransicion(Evento evento) const;

    // Reinicia la FSM a APAGADO (por ejemplo, al reiniciar el equipo).
    void reiniciar() { estado_ = Estado::APAGADO; }

private:
    Estado estado_;

    // Calcula el estado destino para (estado actual, evento) sin modificar nada.
    Estado siguiente(Estado actual, Evento evento) const;
};

// Helpers de presentacion (utiles para logs y para los mensajes MQTT).
const char* nombreEstado(Estado estado);
const char* nombreEvento(Evento evento);

#endif // ALARM_FSM_H
