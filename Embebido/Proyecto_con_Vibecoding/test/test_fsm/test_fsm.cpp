/**
 * test_fsm.cpp
 * -----------------------------------------------------------------------------
 * Tests unitarios de la maquina de estados (AlarmFSM) usando el framework Unity.
 *
 * Se ejecutan en la PC (entorno nativo), sin hardware, con:
 *     pio test -e native
 *
 * Cubren TODAS las transiciones del diagrama de estados, ademas de verificar
 * que los eventos no validos para un estado dado NO cambian el estado.
 * -----------------------------------------------------------------------------
 */
#include <unity.h>
#include "AlarmFSM.h"

// Lleva la FSM hasta un estado de partida concreto de forma deterministica.
static AlarmFSM fsmEn(Estado destino) {
    AlarmFSM fsm; // arranca en APAGADO
    switch (destino) {
        case Estado::APAGADO:
            break;
        case Estado::ACTIVO:
            fsm.procesar(Evento::PRENDER);
            break;
        case Estado::ADVERTENCIA:
            fsm.procesar(Evento::PRENDER);
            fsm.procesar(Evento::MOVIMIENTO);
            break;
        case Estado::ALERTADO:
            fsm.procesar(Evento::PRENDER);
            fsm.procesar(Evento::MAGNETICO);
            break;
    }
    return fsm;
}

void test_estado_inicial_es_apagado(void) {
    AlarmFSM fsm;
    TEST_ASSERT_EQUAL(static_cast<int>(Estado::APAGADO), static_cast<int>(fsm.estado()));
}

// ---- Transiciones desde APAGADO ----
void test_apagado_prender_va_a_activo(void) {
    AlarmFSM fsm = fsmEn(Estado::APAGADO);
    TEST_ASSERT_EQUAL(static_cast<int>(Estado::ACTIVO), static_cast<int>(fsm.procesar(Evento::PRENDER)));
}
void test_apagado_ignora_otros_eventos(void) {
    AlarmFSM fsm = fsmEn(Estado::APAGADO);
    fsm.procesar(Evento::MOVIMIENTO);
    fsm.procesar(Evento::MAGNETICO);
    fsm.procesar(Evento::TIMEOUT_MOV);
    fsm.procesar(Evento::APAGAR);
    TEST_ASSERT_EQUAL(static_cast<int>(Estado::APAGADO), static_cast<int>(fsm.estado()));
}

// ---- Transiciones desde ACTIVO ----
void test_activo_movimiento_va_a_advertencia(void) {
    AlarmFSM fsm = fsmEn(Estado::ACTIVO);
    TEST_ASSERT_EQUAL(static_cast<int>(Estado::ADVERTENCIA), static_cast<int>(fsm.procesar(Evento::MOVIMIENTO)));
}
void test_activo_magnetico_va_a_alertado(void) {
    AlarmFSM fsm = fsmEn(Estado::ACTIVO);
    TEST_ASSERT_EQUAL(static_cast<int>(Estado::ALERTADO), static_cast<int>(fsm.procesar(Evento::MAGNETICO)));
}
void test_activo_apagar_va_a_apagado(void) {
    AlarmFSM fsm = fsmEn(Estado::ACTIVO);
    TEST_ASSERT_EQUAL(static_cast<int>(Estado::APAGADO), static_cast<int>(fsm.procesar(Evento::APAGAR)));
}
void test_activo_ignora_timeout(void) {
    AlarmFSM fsm = fsmEn(Estado::ACTIVO);
    fsm.procesar(Evento::TIMEOUT_MOV);
    TEST_ASSERT_EQUAL(static_cast<int>(Estado::ACTIVO), static_cast<int>(fsm.estado()));
}

// ---- Transiciones desde ADVERTENCIA ----
void test_advertencia_timeout_vuelve_a_activo(void) {
    AlarmFSM fsm = fsmEn(Estado::ADVERTENCIA);
    TEST_ASSERT_EQUAL(static_cast<int>(Estado::ACTIVO), static_cast<int>(fsm.procesar(Evento::TIMEOUT_MOV)));
}
void test_advertencia_movimiento_va_a_alertado(void) {
    AlarmFSM fsm = fsmEn(Estado::ADVERTENCIA);
    TEST_ASSERT_EQUAL(static_cast<int>(Estado::ALERTADO), static_cast<int>(fsm.procesar(Evento::MOVIMIENTO)));
}
void test_advertencia_magnetico_va_a_alertado(void) {
    AlarmFSM fsm = fsmEn(Estado::ADVERTENCIA);
    TEST_ASSERT_EQUAL(static_cast<int>(Estado::ALERTADO), static_cast<int>(fsm.procesar(Evento::MAGNETICO)));
}
void test_advertencia_apagar_va_a_apagado(void) {
    AlarmFSM fsm = fsmEn(Estado::ADVERTENCIA);
    TEST_ASSERT_EQUAL(static_cast<int>(Estado::APAGADO), static_cast<int>(fsm.procesar(Evento::APAGAR)));
}

// ---- Transiciones desde ALERTADO ----
void test_alertado_apagar_va_a_apagado(void) {
    AlarmFSM fsm = fsmEn(Estado::ALERTADO);
    TEST_ASSERT_EQUAL(static_cast<int>(Estado::APAGADO), static_cast<int>(fsm.procesar(Evento::APAGAR)));
}
void test_alertado_no_se_silencia_sin_apagar(void) {
    AlarmFSM fsm = fsmEn(Estado::ALERTADO);
    fsm.procesar(Evento::MOVIMIENTO);
    fsm.procesar(Evento::MAGNETICO);
    fsm.procesar(Evento::TIMEOUT_MOV);
    fsm.procesar(Evento::PRENDER);
    TEST_ASSERT_EQUAL(static_cast<int>(Estado::ALERTADO), static_cast<int>(fsm.estado()));
}

// ---- Escenario completo de uso ----
void test_escenario_completo_robo(void) {
    AlarmFSM fsm; // APAGADO
    TEST_ASSERT_EQUAL(static_cast<int>(Estado::ACTIVO),      static_cast<int>(fsm.procesar(Evento::PRENDER)));
    TEST_ASSERT_EQUAL(static_cast<int>(Estado::ADVERTENCIA), static_cast<int>(fsm.procesar(Evento::MOVIMIENTO)));
    TEST_ASSERT_EQUAL(static_cast<int>(Estado::ALERTADO),    static_cast<int>(fsm.procesar(Evento::MAGNETICO)));
    TEST_ASSERT_EQUAL(static_cast<int>(Estado::APAGADO),     static_cast<int>(fsm.procesar(Evento::APAGAR)));
}

// Punto de entrada del runner de Unity.
int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_estado_inicial_es_apagado);
    RUN_TEST(test_apagado_prender_va_a_activo);
    RUN_TEST(test_apagado_ignora_otros_eventos);
    RUN_TEST(test_activo_movimiento_va_a_advertencia);
    RUN_TEST(test_activo_magnetico_va_a_alertado);
    RUN_TEST(test_activo_apagar_va_a_apagado);
    RUN_TEST(test_activo_ignora_timeout);
    RUN_TEST(test_advertencia_timeout_vuelve_a_activo);
    RUN_TEST(test_advertencia_movimiento_va_a_alertado);
    RUN_TEST(test_advertencia_magnetico_va_a_alertado);
    RUN_TEST(test_advertencia_apagar_va_a_apagado);
    RUN_TEST(test_alertado_apagar_va_a_apagado);
    RUN_TEST(test_alertado_no_se_silencia_sin_apagar);
    RUN_TEST(test_escenario_completo_robo);
    return UNITY_END();
}
