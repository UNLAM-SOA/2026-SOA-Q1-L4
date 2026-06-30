package com.soal4.mochilaantirrobo

import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.navigation.NavController
import com.soal4.mochilaantirrobo.service.HttpService
import com.soal4.mochilaantirrobo.service.MqttNotifierService
import kotlinx.coroutines.launch

@Preview
@Composable
fun PantallaAjuste(navController: NavController? = null) {

    var valorSlider by rememberSaveable { mutableFloatStateOf(0f) }
    val isActivo by MqttNotifierService.estadoAlarma.collectAsState()
    var estadoCargado by remember { mutableStateOf(false) }

    val scope = rememberCoroutineScope()
    val snackbarHostState = remember { SnackbarHostState() }

    LaunchedEffect(Unit) {
        try {
            val resultado = HttpService.api.getEstado()
            val estado = resultado.firstOrNull()?.estado ?: "APAGADO"
            MqttNotifierService.estadoAlarma.value = estado == "ACTIVO"
        } catch (e: Exception) {
            // mantiene el valor actual del flow
        } finally {
            estadoCargado = true
        }
    }

    Scaffold(
        snackbarHost = { SnackbarHost(snackbarHostState) }) { padding ->
        Column(
            modifier = Modifier
                .padding(padding)
                .fillMaxSize(),
            verticalArrangement = Arrangement.Center,
            horizontalAlignment = Alignment.CenterHorizontally
        ) {

            Text(
                text = if (!estadoCargado) "Cargando..." else if (isActivo) "ACTIVO" else "APAGADO",
                fontSize = 28.sp,
                fontWeight = FontWeight.Bold,
                color = if (isActivo) Color(0xFF4CAF50) else Color(0xFFF44336)
            )

            Spacer(modifier = Modifier.height(8.dp))

            Text("Prender o apagar la alarma")

            Switch(
                checked = isActivo,
                onCheckedChange = {
                    scope.launch {
                        MqttNotifierService.enviarArmDsrm()
                    }
                })

            Text(
                "Nivel de Sensibilidad: ${valorSlider.toInt()}",
                modifier = Modifier.padding(top = 30.dp)
            )

            Slider(
                enabled = isActivo,
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
