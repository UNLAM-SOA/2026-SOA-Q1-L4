# App Android — Alarma Antirrobos

Aplicación cliente del dispositivo IoT antirrobos. Se comunica por **WiFi/MQTT**
con el ESP32 (a través del broker Mosquitto local).

## Requisitos

- **Android Studio** (Koala 2024.1 o más nuevo).
- JDK 17 (viene incluido en Android Studio).
- Un teléfono Android **real** (API 26 / Android 8.0 o superior) con acelerómetro
  y vibración, conectado a la **misma red WiFi** que la PC donde corre el broker.
  > El emulador también funciona para MQTT, pero no tiene acelerómetro físico.

## Versiones (combinación estable y verificada)

| Herramienta            | Versión |
|------------------------|---------|
| Gradle                 | 8.7     |
| Android Gradle Plugin  | 8.5.2   |
| Kotlin                 | 1.9.24  |
| compileSdk / targetSdk | 34      |
| minSdk                 | 26      |
| Cliente MQTT (Paho)    | org.eclipse.paho.client.mqttv3:1.2.5 |

> El proyecto **no incluye binarios** (ni APK ni `gradle-wrapper.jar`). Al abrirlo,
> Android Studio genera el wrapper automáticamente durante el primer *Gradle Sync*.

## Estructura y conceptos (alineados con la materia)

- **MainActivity** (Activity 1): campo de texto (IP del broker) + botón Conectar;
  botones ARMAR/DESARMAR que envían comandos por MQTT; muestra el estado y la
  aceleración que llegan **desde** el ESP32.
- **SensorActivity** (Activity 2): usa el **acelerómetro** del teléfono para
  detectar una sacudida y, al detectarla, **vibra** (acción en el smartphone) y
  envía `PANIC` al ESP32 (acción en el embebido).
- **AlarmForegroundService**: *Service* en primer plano que mantiene la conexión
  MQTT viva en **segundo plano** (notificación persistente).
- **MqttManager**: cliente Paho ejecutado en un **hilo de fondo** (Executor); los
  callbacks se reenvían al hilo principal para actualizar la UI.

Navegación: `MainActivity → (botón) → SensorActivity`.

## Cómo ejecutarla

1. Levantá la infraestructura (carpeta `Infraestructura`, ver su README).
2. Averiguá la IP de tu PC en la red WiFi (Windows: `ipconfig` → IPv4).
3. Abrí esta carpeta en Android Studio y esperá el *Gradle Sync*.
4. Conectá el teléfono por USB con **Depuración USB** activada y presioná *Run*.
5. En la app, escribí la IP de la PC en el campo de texto y tocá **Conectar**.
6. Probá ARMAR/DESARMAR y abrí la pantalla del sensor para sacudir el teléfono.

## Permisos que utiliza

`INTERNET`, `ACCESS_NETWORK_STATE` (MQTT), `FOREGROUND_SERVICE` +
`FOREGROUND_SERVICE_DATA_SYNC` (servicio en segundo plano), `POST_NOTIFICATIONS`
(notificación, se pide en tiempo de ejecución en Android 13+) y `VIBRATE`.
