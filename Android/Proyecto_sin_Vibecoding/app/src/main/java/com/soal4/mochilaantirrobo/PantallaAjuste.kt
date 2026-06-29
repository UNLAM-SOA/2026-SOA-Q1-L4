package com.soal4.mochilaantirrobo

import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.navigation.NavController
import com.soal4.mochilaantirrobo.service.MqttNotifierService
import kotlinx.coroutines.launch

@Preview
@Composable
fun PantallaAjuste(navController: NavController? = null) {

    var valorSlider by rememberSaveable { mutableFloatStateOf(0f) }
    var isChecked by rememberSaveable { mutableStateOf(false) }

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
                scope.launch {
                    println("Enviando nueva sensibilidad via MQTT")
                    val respuesta = MqttNotifierService.enviarSensibilidad(valorSlider.toInt())
                    snackbarHostState.showSnackbar(respuesta)
                }
            }) {
                Text("Enviar a la Mochila")
            }

            Spacer(modifier = Modifier.height(16.dp))

            OutlinedButton(onClick = { navController?.navigate("notif") }) {
                Text("Ver historial del acelerometro")
            }

            Spacer(modifier = Modifier.height(8.dp))
            OutlinedButton(onClick = { navController?.navigate("melodias") }) {
                Text("Configurar Melodías de Alarma")
            }
        }
    }
}
