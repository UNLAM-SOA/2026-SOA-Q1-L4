# Embebido — Dispositivo IoT Antirrobos (ESP32-C3)

Firmware del sistema embebido. Vigila golpes (MPU6050) y la apertura del cierre
(sensor de contacto), reacciona con LED + buzzer según una **máquina de estados**
y se comunica por **WiFi/MQTT** con la infraestructura local.

## Cómo compilar (Visual Studio Code + PlatformIO)

1. Instalá **VS Code** y la extensión **PlatformIO IDE**.
2. Abrí esta carpeta (`Embebido/Proyecto_con_Vibecoding`) en VS Code.
3. PlatformIO descargará solo la plataforma y librerías la primera vez.
4. Compilá con el botón **Build** (✓) o `Ctrl+Alt+B`.

`platformio.ini` fija `espressif32@6.9.0` (arduino-esp32 **2.0.17**), una versión
estable cuya API de LEDC coincide con la usada para el buzzer. Así se evita el
error típico de "función no declarada" al mezclar con el core 3.x.

## Cómo simular (Wokwi en VS Code)

1. Instalá la extensión **Wokwi for VS Code** (y activá la licencia gratuita).
2. **Build** del proyecto (paso anterior) para generar `firmware.bin`/`.elf`.
3. `F1` → **Wokwi: Start Simulator**. Se abre el circuito de `diagram.json`.

El WiFi simulado usa la red `Wokwi-GUEST`. El *Wokwi Private IoT Gateway*
(incluido en la extensión) conecta esa red con tu PC, por eso el firmware se
conecta al broker del host como **`host.wokwi.internal:1883`** (ver `config.h`).

## Cómo correr los tests de la máquina de estados (en la PC)

```bash
pio test -e native
```

Compila y ejecuta `test/test_fsm/test_fsm.cpp` (14 casos) en la PC, sin hardware.
Cubren todas las transiciones del diagrama de estados.

## Hardware (pines del ESP32-C3)

| Componente                         | Pin GPIO | Notas                                   |
|------------------------------------|----------|-----------------------------------------|
| Sensor de contacto (potenciómetro) | 3        | Entrada analógica (ADC1_CH3)            |
| Pulsador armar/desarmar            | 4        | `INPUT_PULLUP` + interrupción `FALLING` |
| MPU6050 — SDA                      | 5        | Bus I2C                                 |
| MPU6050 — SCL                      | 6        | Bus I2C                                 |
| LED rojo (indicador)               | 7        | Salida digital                          |
| Buzzer (alarma)                    | 10       | PWM por LEDC (880 Hz)                    |

Se evitaron a propósito los pines de *strapping* (2, 8, 9) para que funcione
igual en Wokwi y en una placa física.

## Arquitectura del firmware

- **Patrón máquina de estados** (`AlarmFSM`, C++ puro y testeable):
  `APAGADO → ACTIVO → ADVERTENCIA → ALERTADO`, según `Diagrama-de-estados.png`.
- **FreeRTOS** con 3 tareas + 1 timer de software (sin `delay()` bloqueante):
  - `tareaSensores`: lee MPU6050 + contacto, genera eventos, publica aceleración.
  - `tareaControl`: corre la FSM y maneja LED/buzzer.
  - `tareaRed`: mantiene WiFi/MQTT y traduce comandos remotos a eventos.
- **Cola de eventos** (`xQueueCreate`) para comunicar tareas e ISR de forma segura.
- **Timer de FreeRTOS** para la ventana de ADVERTENCIA (6 s).
- **Sin números mágicos**: todo en `include/config.h`.

## Para la actividad de métricas (Metrics.h — comisión Martes)

La biblioteca `Metrics.h` de la cátedra **solo compila en el IDE de Arduino /
Wokwi, no en PlatformIO/VS Code**. Por eso no se incluye en este proyecto (que se
compila con VS Code). Para tomar las métricas: abrí el `src/main.cpp` en el IDE de
Arduino o en Wokwi web, agregá `#include "Metrics.h"`, llamá a `initStats()` en el
`setup()` y a `finishStats()` cuando ocurra el evento a medir (p. ej. al recibir
un mensaje MQTT). Ver la guía general en `../../GUIA_DE_PRUEBAS.md`.
