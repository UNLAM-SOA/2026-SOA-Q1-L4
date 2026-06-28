package com.soal4.mochilaantirrobo

import android.hardware.SensorManager
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.tooling.preview.Preview
import androidx.lifecycle.lifecycleScope
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

        MqttNotifierService.conectar()
        MqttNotifierService.suscribirAlertas()

        enableEdgeToEdge()
        setContent {
            MochilaAntirroboTheme {
                AppNavigation()
            }
        }
    }

    override fun onResume() {
        super.onResume()
        shakeDetector.iniciar()
    }

    override fun onPause() {
        super.onPause()
        shakeDetector.detener()
    }
}

@Preview
@Composable
fun AppNavigation() {
    val navController = rememberNavController()
    val mensaje by MqttNotifierService.mensajeEntrante.collectAsState()

    if (mensaje != null) {
        AlertDialog(
            onDismissRequest = { MqttNotifierService.limpiarMensaje() },
            title = { Text("Estado de la mochila") },
            text = { Text(mensaje!!) },
            confirmButton = {
                Button(onClick = { MqttNotifierService.limpiarMensaje() }) {
                    Text("OK")
                }
            }
        )
    }

    NavHost(navController = navController, startDestination = "splash") {
        composable("splash") { PantallaSplash(navController) }
        composable("ajuste") { PantallaAjuste(navController = navController) }
        composable("notif") { PantallaHistorial(navController = navController) }
        composable("melodias") { PantallaMelodias(navController = navController) }
    }
}