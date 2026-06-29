/**
 * AlarmFSM.cpp
 * -----------------------------------------------------------------------------
 * Implementacion de la maquina de estados. Las transiciones estan escritas de
 * forma explicita (un switch por estado) para que sea facil contrastarlas con
 * el diagrama de estados del informe.
 * -----------------------------------------------------------------------------
 */
#include "AlarmFSM.h"

Estado AlarmFSM::siguiente(Estado actual, Evento evento) const {
    switch (actual) {

        case Estado::APAGADO:
            // Solo se puede pasar a ACTIVO al "prender".
            if (evento == Evento::PRENDER) return Estado::ACTIVO;
            return actual;

        case Estado::ACTIVO:
            // Armado y vigilando.
            if (evento == Evento::MOVIMIENTO) return Estado::ADVERTENCIA; // (3)
            if (evento == Evento::MAGNETICO)  return Estado::ALERTADO;    // (2)
            if (evento == Evento::APAGAR)     return Estado::APAGADO;     // (5)
            return actual;

        case Estado::ADVERTENCIA:
            // Alta vigilancia. Un nuevo estimulo dispara la alarma; si vence la
            // ventana de tiempo sin novedad, vuelve a ACTIVO.
            if (evento == Evento::MOVIMIENTO)  return Estado::ALERTADO;    // (3)
            if (evento == Evento::MAGNETICO)   return Estado::ALERTADO;    // (2)
            if (evento == Evento::TIMEOUT_MOV) return Estado::ACTIVO;      // (4)
            if (evento == Evento::APAGAR)      return Estado::APAGADO;     // (5)
            return actual;

        case Estado::ALERTADO:
            // La alarma solo se silencia desarmando el sistema.
            if (evento == Evento::APAGAR) return Estado::APAGADO;          // (5)
            return actual;
    }
    return actual;
}

bool AlarmFSM::provocaTransicion(Evento evento) const {
    return siguiente(estado_, evento) != estado_;
}

Estado AlarmFSM::procesar(Evento evento) {
    estado_ = siguiente(estado_, evento);
    return estado_;
}

const char* nombreEstado(Estado estado) {
    switch (estado) {
        case Estado::APAGADO:     return "APAGADO";
        case Estado::ACTIVO:      return "ACTIVO";
        case Estado::ADVERTENCIA: return "ADVERTENCIA";
        case Estado::ALERTADO:    return "ALERTADO";
    }
    return "DESCONOCIDO";
}

const char* nombreEvento(Evento evento) {
    switch (evento) {
        case Evento::PRENDER:     return "PRENDER";
        case Evento::MAGNETICO:   return "MAGNETICO";
        case Evento::MOVIMIENTO:  return "MOVIMIENTO";
        case Evento::TIMEOUT_MOV: return "TIMEOUT_MOV";
        case Evento::APAGAR:      return "APAGAR";
    }
    return "DESCONOCIDO";
}
