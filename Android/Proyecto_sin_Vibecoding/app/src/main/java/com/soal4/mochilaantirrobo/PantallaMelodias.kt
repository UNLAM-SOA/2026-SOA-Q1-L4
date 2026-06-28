package com.soal4.mochilaantirrobo

import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.navigation.NavController
import com.soal4.mochilaantirrobo.service.MqttNotifierService
import kotlinx.coroutines.launch

@Preview
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

                        val respuesta = MqttNotifierService.enviarRingtone(idSeleccionado.toInt())

                        println("Respuesta ringtone: $respuesta")
                    }
                    navController?.popBackStack()
                }, modifier = Modifier.fillMaxWidth()
            ) {
                Text("Sincronizar Melodía")
            }
        }
    }
}
