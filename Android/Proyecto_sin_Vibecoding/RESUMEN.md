## Clases importantes del proyecto

Solo hay tres clases que escribimos nosotros. El resto (`Color.kt`, `Theme.kt`, `Type.kt`, `ExampleUnitTest.kt`, `ExampleInstrumentedTest.kt`) se genera automáticamente al crear el proyecto en Android Studio y no es necesario tocarlas.

| Archivo | Qué hace |
|---|---|
| `app/src/main/java/.../MainActivity.kt` | Activity principal + UI en Compose + lógica de shake |
| `app/src/main/java/.../sensors/ShakeDetector.kt` | Detecta el movimiento con el acelerómetro |
| `app/src/main/java/.../service/MqttNotifierService.kt` | Maneja la conexión y publicación MQTT |

### Dónde están las librerías

Las dependencias se declaran en dos archivos:

- **`gradle/libs.versions.toml`** — catálogo central con los nombres y versiones de todas las librerías (MQTT, Compose, Navigation, etc.)
- **`app/build.gradle.kts`** — lista qué librerías usa la app (referencia los alias definidos en el `.toml`)

---

## 1. Sensor — `ShakeDetector`

Implementa `SensorEventListener` para recibir datos del acelerómetro. En cada lectura calcula la magnitud del movimiento con:

```
aceleracion = √(x² + y² + z²)
```

Si supera el umbral (por defecto 40) y pasaron más de 2 segundos desde el último shake, dispara el callback `onShake`. El sensor se activa en `onResume` y se apaga en `onPause`.

---

## 2. MQTT — `MqttNotifierService`

Se usa para notificar a los tópicos mqtt que definimos

| Función | Tópico | Mensaje |
|---|---|---|
| `enviarShake()` | `my/shake` | `"true"` |
| `enviarSensibilidad(nivel)` | `my/topic` | el número como string |

La conexión corre en un hilo de background (`Dispatchers.IO`) para no bloquear la UI.

---

## 3. UI — Jetpack Compose

### Una sola Activity

En Android tradicional cada pantalla era una `Activity` distinta. Con Jetpack Compose hay **una sola `Activity`** (`MainActivity`) que actúa como contenedor. Las pantallas no son Activities — son funciones Kotlin con la anotación `@Composable`.

### Sin XML

Antes la UI se definía en archivos `.xml` arrastrando elementos en un editor visual. En Compose la UI se escribe directamente en código Kotlin:

```kotlin
@Composable
fun PantallaAjuste() {
    Column {
        Text("Hola")
        Button(onClick = { }) {
            Text("Presioname")
        }
    }
}
```

Cada función `@Composable` describe cómo se ve una parte de la pantalla. Se pueden anidar y reutilizar libremente, igual que funciones normales.

### Navegación entre pantallas

Como no hay múltiples Activities, la navegación se maneja con `NavHost` de Navigation Compose. `AppNavigation` registra todas las pantallas disponibles y cuál es la inicial:

```kotlin
NavHost(navController, startDestination = "ajuste") {
    composable("ajuste") { PantallaAjuste(navController) }
    composable("info")   { PantallaInfo(navController) }
}
```

Para navegar se llama `navController.navigate("ruta")` y para volver `navController.popBackStack()`.

### Pantallas de esta app

- **`PantallaAjuste`** — pantalla principal. Tiene un `Slider` (0–100) para elegir la sensibilidad y un botón que la envía por MQTT.
- **`PantallaInfo`** — pantalla de ejemplo que demuestra la navegación.

### Coroutines en la UI

Las operaciones de red no pueden correr en el hilo principal o la app se congela. Dentro de un `@Composable` se usa `rememberCoroutineScope` + `launch` para correrlas en un hilo secundario sin bloquear la interfaz.

`lifecycleScope.launch` por defecto usa `Dispatchers.Main` (el hilo de la UI). El `Dispatchers.IO` es lo que le indica explícitamente que corra en un hilo de background dedicado a operaciones de red/disco.