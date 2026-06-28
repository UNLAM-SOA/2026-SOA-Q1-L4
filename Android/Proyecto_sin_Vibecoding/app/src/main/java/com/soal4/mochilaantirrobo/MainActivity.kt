package com.soal4.mochilaantirrobo

import android.hardware.SensorManager
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.Image
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.lifecycle.lifecycleScope
import androidx.navigation.NavController
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import com.soal4.mochilaantirrobo.sensors.ShakeDetector
import com.soal4.mochilaantirrobo.service.MqttNotifierService
import com.soal4.mochilaantirrobo.ui.theme.MochilaAntirroboTheme
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch

class MainActivity : ComponentActivity() {

    private lateinit var shakeDetector: ShakeDetector

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val sensorManager = getSystemService(SENSOR_SERVICE) as SensorManager
        shakeDetector = ShakeDetector(sensorManager)
        shakeDetector.onShake = {
            lifecycleScope.launch(Dispatchers.IO) {
                MqttNotifierService.enviarArmDsrm()
            }
        }

        // TODO que no corra en una corutina
        MqttNotifierService.conectar()

        enableEdgeToEdge()
        setContent {
            MochilaAntirroboTheme {
                AppNavigation()
            }
        }
    }

    // Cuando se entra en la activity
    override fun onResume() {
        super.onResume()
        shakeDetector.iniciar()
    }

    // Cuando sale de la activity
    override fun onPause() {
        super.onPause()
        shakeDetector.detener()
    }
}

// Solo dice que otros composables (pantallas) existen y cual es el principal.
@Preview
@Composable
fun AppNavigation() {
    val navController = rememberNavController()
    NavHost(navController = navController, startDestination = "splash") {
        composable("splash") { PantallaSplash(navController) }
        composable("ajuste") {
            PantallaAjuste(navController = navController)
        }
        composable("info") {
            PantallaInfo(navController = navController)
        }
        composable("notif") {
            PantallaNotificaciones(navController = navController)
        }
        composable("melodias") {
            PantallaMelodias(navController = navController)
        }
    }
}


// Las pantallas acá se definen así, no con un nuevo activity.
// Se definene con @composable
@Preview
@Composable
fun PantallaAjuste(navController: NavController? = null) {

    var valorSlider by rememberSaveable { mutableFloatStateOf(0f) }
    var isChecked by rememberSaveable { mutableStateOf(false) }

    // Define una corutina, corre las cosas en otro hilo que no es el principal
    // Es como crear un nuevo thread
    val scope = rememberCoroutineScope()

    val snackbarHostState = remember { SnackbarHostState() }

    Scaffold(
        snackbarHost = { SnackbarHost(snackbarHostState) }) { padding ->
        Column(
            modifier = Modifier
                .padding(padding)
                .fillMaxSize(),
            verticalArrangement = Arrangement.Center,
            horizontalAlignment = Alignment.CenterHorizontally
        ) {

            Text("Prender o apagar la alarma")

            Switch(

                checked = isChecked, onCheckedChange = {
                    isChecked = it
                    scope.launch {
                        println("Enviando prendido/apagado via MQTT")
                        val respuesta = MqttNotifierService.enviarArmDsrm()
                        println("Respuesta: $respuesta")
                    }
                })

            Text(
                "Nivel de Sensibilidad: ${valorSlider.toInt()}",
                modifier = Modifier.padding(top = 30.dp)
            )

            Slider(
                enabled = isChecked,
                value = valorSlider,
                onValueChange = { valorSlider = it },
                valueRange = 0f..100f,
                modifier = Modifier.padding(horizontal = 40.dp)
            )

            Button(onClick = {
                // Con launch le indicas que lo corra en ese nuevo hilo, en lugar del principal.
                scope.launch {
                    println("Enviando nueva sensibilidad via MQTT")
                    val respuesta = MqttNotifierService.enviarSensibilidad(valorSlider.toInt())
                    snackbarHostState.showSnackbar(respuesta)
                }
            }) {
                Text("Enviar a la Mochila")
            }

            Spacer(modifier = Modifier.height(16.dp))

            // Con navigate le indicas que vaya a esa otra pantalla
            OutlinedButton(onClick = { navController?.navigate("info") }) {
                Text("Ir a pantalla info")
            }
            Spacer(modifier = Modifier.height(8.dp))

            OutlinedButton(onClick = { navController?.navigate("notif") }) {
                Text("Ver historial de alertas")
            }

            Spacer(modifier = Modifier.height(8.dp))
            OutlinedButton(onClick = { navController?.navigate("melodias") }) {
                Text("Configurar Melodías de Alarma")
            }
        }
    }
}


@Composable
fun PantallaSplash(navController: NavController) {
    LaunchedEffect(Unit) {
        kotlinx.coroutines.delay(2000)
        navController.navigate("ajuste") {
            popUpTo("splash") { inclusive = true }
        }
    }

    Box(
        modifier = Modifier
            .fillMaxSize()
            .padding(32.dp), contentAlignment = Alignment.Center
    ) {
        Column(horizontalAlignment = Alignment.CenterHorizontally) {
            Image(
                painter = painterResource(id = R.drawable.logo_mochila),
                contentDescription = "Logo de la mochila",
                modifier = Modifier.size(180.dp)
            )
            Spacer(modifier = Modifier.height(16.dp))
            Text(
                text = "Mochila Antirrobo", style = MaterialTheme.typography.headlineMedium
            )
        }
    }
}


@Composable
fun PantallaNotificaciones(navController: NavController? = null) {
    // Datos de prueba: después los reemplazás con lo que venga de la BBDD
    val notificaciones = listOf(
        Notificacion("2026-06-05 21:30", "Sensor activado en la mochila"),
        Notificacion("2026-06-05 20:15", "Sensibilidad ajustada a 80"),
        Notificacion("2026-06-04 18:00", "Shake detectado")
    )

    Scaffold { padding ->
        Column(
            modifier = Modifier
                .padding(padding)
                .fillMaxSize()
        ) {
            Text(
                text = "Registro de Notificaciones",
                style = MaterialTheme.typography.headlineMedium,
                modifier = Modifier.padding(16.dp)
            )

            // Lista de notificaciones
            LazyColumn(
                modifier = Modifier.fillMaxSize(),
                contentPadding = PaddingValues(16.dp),
                verticalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                items(notificaciones) { notif ->
                    Card(
                        modifier = Modifier.fillMaxWidth(),
                        elevation = CardDefaults.cardElevation(defaultElevation = 4.dp)
                    ) {
                        Row(
                            modifier = Modifier
                                .padding(12.dp)
                                .fillMaxWidth(),
                            horizontalArrangement = Arrangement.SpaceBetween
                        ) {
                            Text(
                                text = notif.fechaHora, style = MaterialTheme.typography.bodyMedium
                            )
                            Spacer(modifier = Modifier.width(16.dp))
                            Text(
                                text = notif.descripcion, style = MaterialTheme.typography.bodyLarge
                            )
                        }
                    }
                }
            }

            Spacer(modifier = Modifier.height(16.dp))

            Button(
                onClick = { navController?.popBackStack() },
                modifier = Modifier.align(Alignment.CenterHorizontally)
            ) {
                Text("Volver")
            }
        }
    }
}

// Modelo simple para la notificación
data class Notificacion(val fechaHora: String, val descripcion: String)


// Esta es una pantalla de ejemplo para ver como navegar desde una pantalla a otra
// Después se puede borrar
@Preview
@Composable
fun PantallaInfo(navController: NavController? = null) {
    Scaffold { padding ->
        Column(
            modifier = Modifier
                .padding(padding)
                .fillMaxSize(),
            verticalArrangement = Arrangement.Center,
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            Text(
                text = "Proyecto SOA grupo 4", style = MaterialTheme.typography.headlineMedium
            )
            Spacer(modifier = Modifier.height(8.dp))
            Text("Hola! Soy un ejemplo.")
            Spacer(modifier = Modifier.height(24.dp))
            Button(onClick = { navController?.popBackStack() }) {
                Text("Volver")
            }
        }
    }
}

@Composable
fun PantallaMelodias(navController: NavController? = null) {
    val listaMelodias = remember {
        listOf(
            Pair("1", "Star Wars - Marcha Imperial"),
            Pair("2", "Nokia Clásico"),
            Pair("3", "Super Mario Bros (1-Up)"),
            Pair("4", "Tetris Theme")
        )
    }
    var idSeleccionado by remember { mutableStateOf("1") }
    val scope = rememberCoroutineScope()

    Scaffold { padding ->
        Column(
            modifier = Modifier
                .padding(padding)
                .fillMaxSize()
                .padding(16.dp),
            verticalArrangement = Arrangement.SpaceBetween,
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            Column(horizontalAlignment = Alignment.CenterHorizontally) {
                Text(
                    "Seleccioná la melodía para la mochila",
                    style = MaterialTheme.typography.titleMedium
                )
                Spacer(modifier = Modifier.height(16.dp))

                listaMelodias.forEach { melodia ->
                    Row(
                        verticalAlignment = Alignment.CenterVertically,
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(8.dp)
                    ) {
                        RadioButton(
                            selected = (idSeleccionado == melodia.first),
                            onClick = { idSeleccionado = melodia.first })
                        Text(text = melodia.second, modifier = Modifier.padding(start = 8.dp))
                    }
                }
            }

            Button(
                onClick = {
                    scope.launch {
                        println("Enviando melodía ID $idSeleccionado via MQTT")

                        // LLAMADA CORREGIDA: Apunta a la nueva función dedicada
                        val respuesta = MqttNotifierService.enviarRingtone(idSeleccionado.toInt())

                        println(respuesta) // Esto te va a imprimir el éxito o falla en consola
                    }
                    navController?.popBackStack() // Vuelve a la pantalla del Slider
                }, modifier = Modifier.fillMaxWidth()
            ) {
                Text("Sincronizar Melodía")
            }
        }
    }
}