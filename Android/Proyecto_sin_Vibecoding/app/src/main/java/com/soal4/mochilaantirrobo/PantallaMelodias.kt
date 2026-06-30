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
            "STARWARS",
            "NOKIA",
            "MARIO",
            "TETRIS"
        )
    }
    var nombreSeleccionado by remember { mutableStateOf(listaMelodias.first()) }
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
                            selected = (nombreSeleccionado == melodia),
                            onClick = { nombreSeleccionado = melodia })
                        Text(text = melodia, modifier = Modifier.padding(start = 8.dp))
                    }
                }
            }

            Button(
                onClick = {
                    scope.launch {
                        val respuesta = MqttNotifierService.enviarRingtone(nombreSeleccionado)
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
