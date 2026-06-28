package com.soal4.mochilaantirrobo.service

import android.util.Log
import kotlinx.coroutines.flow.MutableStateFlow
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken
import org.eclipse.paho.client.mqttv3.MqttCallback
import org.eclipse.paho.client.mqttv3.MqttClient
import org.eclipse.paho.client.mqttv3.MqttConnectOptions
import org.eclipse.paho.client.mqttv3.MqttException
import org.eclipse.paho.client.mqttv3.MqttMessage
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence
import kotlin.coroutines.resume
import kotlin.coroutines.suspendCoroutine

object MqttNotifierService {

    private lateinit var mqttClient: MqttClient

    // TODO Actualizar
    private const val BROKER_URL = "tcp://192.168.0.250:1883"
    private const val CLIENT_ID = "MochilaAndroidClient"

    private const val TOPIC_SHAKE = "alarm/arm_dsrm"
    private const val TOPIC_SENSIBILIDAD = "alarm/acelerometer_sens"

    private const val TOPIC_RINGTONE = "alarm/ringtone_id"
    private const val TOPIC_ALERTAS = "alarm/state"


    val mensajeEntrante = MutableStateFlow<String?>(null)

    fun conectar() {

            try {

                mqttClient = MqttClient(BROKER_URL, CLIENT_ID, MemoryPersistence())

                val options = MqttConnectOptions().apply {
                    isAutomaticReconnect = true
                    isCleanSession = false
                    userName = "claromio"
                    password = "RiverBest".toCharArray()
                }

                mqttClient.connect(options)

                if (!mqttClient.isConnected) {
                    println("Error al conectarse por mqtt")
                }


            } catch (e: Exception) {
                println("Error al conectase por mqtt")
                e.printStackTrace()
            }
        }

    suspend fun enviarArmDsrm(): Void =
        suspendCoroutine {
            val topic = TOPIC_SHAKE

            val message = MqttMessage("true".toByteArray()).apply {
                qos = 1
            }

            try {

                mqttClient.publish(topic, message)

            } catch (e: MqttException) {
                Log.e("MQTT", "Error notificando shake por MQTT")
                e.printStackTrace()
            }
        }

    suspend fun enviarSensibilidad(nivel: Int): String =
        suspendCoroutine { continuation ->

            val topic = TOPIC_SENSIBILIDAD

            val payload = nivel.toString()

            val message = MqttMessage(payload.toByteArray()).apply {
                qos = 1
            }

            try {

                mqttClient.publish(topic, message)

                continuation.resume(
                    "MQTT: Nivel $nivel enviado con éxito"
                )

            } catch (e: MqttException) {

                continuation.resume(
                    "MQTT Error: ${e.message}"
                )
            }
        }
    suspend fun enviarRingtone(idMelodia: Int): String =
        suspendCoroutine { continuation ->

            val topic = TOPIC_RINGTONE
            val payload = idMelodia.toString() // Convierte el 1, 2, 3 o 4 a Texto

            val message = MqttMessage(payload.toByteArray()).apply {
                qos = 1
            }

            try {
                mqttClient.publish(topic, message)

                continuation.resume(
                    "MQTT: Melodía $idMelodia sincronizada con éxito"
                )
            } catch (e: MqttException) {
                continuation.resume(
                    "MQTT Error al cambiar melodía: ${e.message}"
                )
            }
        }

    fun suscribirAlertas() {
        mqttClient.setCallback(object : MqttCallback {
            override fun messageArrived(topic: String, message: MqttMessage) {
                mensajeEntrante.value = message.toString()
            }
            override fun connectionLost(cause: Throwable?) {}
            override fun deliveryComplete(token: IMqttDeliveryToken?) {}
        })
        try {
            mqttClient.subscribe(TOPIC_ALERTAS, 1)
        } catch (e: MqttException) {
            Log.e("MQTT", "Error al suscribirse: ${e.message}")
        }
    }

    fun limpiarMensaje() {
        mensajeEntrante.value = null
    }
}