package com.soa.alarma

import android.content.Context
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import android.os.Build
import android.os.Bundle
import android.os.VibrationEffect
import android.os.Vibrator
import android.os.VibratorManager
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import kotlin.math.sqrt

/**
 * SensorActivity (Activity 2)
 * -----------------------------------------------------------------------------
 * Usa el ACELEROMETRO del telefono (un sensor del dispositivo movil) para
 * detectar una sacudida ("shake"). Al detectarla:
 *   - Realiza una accion en el SMARTPHONE: hace vibrar el telefono.
 *   - Realiza una accion en el SISTEMA EMBEBIDO: envia el comando PANIC por MQTT,
 *     lo que dispara la alarma del ESP32.
 *
 * Asi se ejercita el uso de sensores + permisos (VIBRATE) e integra el telefono
 * con el embebido, tal como pide la consigna.
 */
class SensorActivity : AppCompatActivity(), SensorEventListener {

    private lateinit var sensorManager: SensorManager
    private var acelerometro: Sensor? = null
    private lateinit var vibrator: Vibrator

    private lateinit var txtValores: TextView
    private lateinit var txtSacudidas: TextView
    private lateinit var txtUltima: TextView

    private var contadorSacudidas = 0
    private var ultimaSacudidaMs = 0L

    companion object {
        // Umbral de fuerza (en "g") para considerar que hubo una sacudida.
        private const val UMBRAL_SHAKE_G = 2.7f
        // Tiempo minimo entre sacudidas para no contar la misma dos veces.
        private const val INTERVALO_MIN_MS = 1000L
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_sensor)

        txtValores = findViewById(R.id.txtValores)
        txtSacudidas = findViewById(R.id.txtSacudidas)
        txtUltima = findViewById(R.id.txtUltima)

        sensorManager = getSystemService(Context.SENSOR_SERVICE) as SensorManager
        acelerometro = sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER)
        vibrator = obtenerVibrador()

        if (acelerometro == null) {
            Toast.makeText(this, "Este dispositivo no tiene acelerometro", Toast.LENGTH_LONG).show()
        }
    }

    override fun onResume() {
        super.onResume()
        acelerometro?.let {
            sensorManager.registerListener(this, it, SensorManager.SENSOR_DELAY_GAME)
        }
    }

    override fun onPause() {
        super.onPause()
        sensorManager.unregisterListener(this)
    }

    override fun onSensorChanged(event: SensorEvent) {
        if (event.sensor.type != Sensor.TYPE_ACCELEROMETER) return

        val x = event.values[0]
        val y = event.values[1]
        val z = event.values[2]
        txtValores.text = getString(R.string.fmt_xyz, x, y, z)

        // Fuerza total normalizada respecto de la gravedad terrestre.
        val gFuerza = sqrt(x * x + y * y + z * z) / SensorManager.GRAVITY_EARTH

        val ahora = System.currentTimeMillis()
        if (gFuerza > UMBRAL_SHAKE_G && ahora - ultimaSacudidaMs > INTERVALO_MIN_MS) {
            ultimaSacudidaMs = ahora
            onSacudidaDetectada()
        }
    }

    override fun onAccuracyChanged(sensor: Sensor?, accuracy: Int) { /* no usado */ }

    private fun onSacudidaDetectada() {
        contadorSacudidas++
        txtSacudidas.text = getString(R.string.fmt_sacudidas, contadorSacudidas)
        txtUltima.text = getString(R.string.shake_accion)

        vibrar()

        // Accion en el embebido: disparar la alarma (PANIC) por MQTT.
        if (MqttManager.connected) {
            MqttManager.publicarComando("PANIC")
            Toast.makeText(this, "Sacudida! Se envio PANIC al ESP32", Toast.LENGTH_SHORT).show()
        } else {
            Toast.makeText(this, "Sacudida detectada (sin conexion MQTT)", Toast.LENGTH_SHORT).show()
        }
    }

    private fun obtenerVibrador(): Vibrator =
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            val vm = getSystemService(Context.VIBRATOR_MANAGER_SERVICE) as VibratorManager
            vm.defaultVibrator
        } else {
            @Suppress("DEPRECATION")
            getSystemService(Context.VIBRATOR_SERVICE) as Vibrator
        }

    private fun vibrar() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            vibrator.vibrate(VibrationEffect.createOneShot(400, VibrationEffect.DEFAULT_AMPLITUDE))
        } else {
            @Suppress("DEPRECATION")
            vibrator.vibrate(400)
        }
    }
}
