package com.soal4.mochilaantirrobo.sensors

import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import kotlin.math.sqrt

class ShakeDetector(
    private val sensorManager: SensorManager,
    private val umbralFuerzaShake: Int = 40,
    private val cooldownMs: Long = 2000
) : SensorEventListener {

    var onShake: (() -> Unit)? = null

    private val accelerometer: Sensor? =
        sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER)

    private var ultimoShake = 0L

    fun iniciar() {
        sensorManager.registerListener(this, accelerometer, SensorManager.SENSOR_DELAY_NORMAL)
    }

    fun detener() {
        sensorManager.unregisterListener(this)
    }

    override fun onSensorChanged(event: SensorEvent?) {
        if (event?.sensor?.type != Sensor.TYPE_ACCELEROMETER) return

        val (x, y, z) = event.values
        val aceleracion = sqrt((x * x + y * y + z * z).toDouble())

        if (aceleracion > umbralFuerzaShake) {
            println("Se detectó un shake")
            val ahora = System.currentTimeMillis()
            if (ahora - ultimoShake > cooldownMs) {
                println("Enviando notificación de shake por mqtt")
                ultimoShake = ahora
                onShake?.invoke()
            }
            else {
                println("El nuevo shake fue muy seguido al anterior, ignorandolo")
            }
        }
    }

    override fun onAccuracyChanged(sensor: Sensor?, accuracy: Int) = Unit
}
