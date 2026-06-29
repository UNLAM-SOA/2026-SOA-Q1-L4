package com.soa.alarma

import android.os.Handler
import android.os.Looper
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken
import org.eclipse.paho.client.mqttv3.MqttCallbackExtended
import org.eclipse.paho.client.mqttv3.MqttClient
import org.eclipse.paho.client.mqttv3.MqttConnectOptions
import org.eclipse.paho.client.mqttv3.MqttMessage
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence
import org.json.JSONObject
import java.util.concurrent.Executors

/**
 * MqttManager
 * -----------------------------------------------------------------------------
 * Punto unico de acceso a MQTT para toda la app (patron singleton).
 *
 * - Usa la libreria Java de Eclipse Paho.
 * - Todas las operaciones de red (connect/publish/disconnect) se ejecutan en un
 *   hilo de fondo (Executor), NUNCA en el hilo principal: de lo contrario Android
 *   lanzaria NetworkOnMainThreadException.
 * - Los callbacks de Paho llegan en su propio hilo y se reenvian al hilo principal
 *   (Handler del Looper main) antes de avisar a la UI, para que las Activities
 *   puedan tocar las vistas sin problemas.
 *
 * La conexion la mantiene viva AlarmForegroundService (servicio en primer plano),
 * lo que permite que siga funcionando aunque la app pase a segundo plano.
 */
object MqttManager {

    // Topicos (coinciden con el ESP32, Node-RED y Telegraf)
    const val TOPIC_ESTADO = "alarma/estado"
    const val TOPIC_ACEL = "alarma/acel"
    const val TOPIC_EVENTO = "alarma/evento"
    const val TOPIC_COMANDO = "alarma/comando"

    private const val PUERTO = 1883
    private const val USUARIO = "claromio"
    private const val PASSWORD = "RiverBest"

    /** La UI implementa esta interfaz para enterarse de lo que pasa. */
    interface Listener {
        fun onConnectionChanged(connected: Boolean, info: String)
        fun onEstado(estado: String, modo: String)
        fun onAcel(magnitud: Double)
        fun onEvento(evento: String, detalle: String)
    }

    private val io = Executors.newSingleThreadExecutor()
    private val main = Handler(Looper.getMainLooper())
    private val listeners = mutableSetOf<Listener>()

    private var client: MqttClient? = null

    // Ultimos valores conocidos (para refrescar la UI ni bien se suscribe).
    @Volatile var connected = false; private set
    @Volatile var lastEstado = "—"; private set
    @Volatile var lastModo = ""; private set
    @Volatile var lastAcel = 0.0; private set

    fun addListener(l: Listener) {
        listeners.add(l)
        // Estado inicial inmediato para el nuevo oyente.
        main.post {
            l.onConnectionChanged(connected, if (connected) "Conectado" else "Desconectado")
            l.onEstado(lastEstado, lastModo)
            l.onAcel(lastAcel)
        }
    }

    fun removeListener(l: Listener) = listeners.remove(l)

    /** Conecta al broker en la IP indicada (puerto 1883). Reintenta solo. */
    fun connect(brokerIp: String) {
        io.execute {
            try {
                cerrarCliente()
                val uri = "tcp://$brokerIp:$PUERTO"
                val c = MqttClient(uri, "android-" + System.currentTimeMillis(), MemoryPersistence())
                val opts = MqttConnectOptions().apply {
                    userName = USUARIO
                    password = PASSWORD.toCharArray()
                    isCleanSession = true
                    isAutomaticReconnect = true
                    connectionTimeout = 10
                    keepAliveInterval = 30
                }
                c.setCallback(object : MqttCallbackExtended {
                    override fun connectComplete(reconnect: Boolean, serverURI: String?) {
                        try {
                            c.subscribe(
                                arrayOf(TOPIC_ESTADO, TOPIC_ACEL, TOPIC_EVENTO),
                                intArrayOf(0, 0, 0)
                            )
                        } catch (_: Exception) { }
                        notificarConexion(true, "Conectado a $uri")
                    }
                    override fun connectionLost(cause: Throwable?) {
                        notificarConexion(false, "Conexion perdida (se reintenta)")
                    }
                    override fun messageArrived(topic: String, message: MqttMessage) {
                        procesarMensaje(topic, String(message.payload))
                    }
                    override fun deliveryComplete(token: IMqttDeliveryToken?) { }
                })
                c.connect(opts)
                client = c
            } catch (e: Exception) {
                notificarConexion(false, "Error: ${e.message}")
            }
        }
    }

    /** Publica un comando ("ARM", "DISARM", "PANIC") hacia el ESP32. */
    fun publicarComando(cmd: String) {
        io.execute {
            try {
                client?.takeIf { it.isConnected }
                    ?.publish(TOPIC_COMANDO, MqttMessage(cmd.toByteArray()))
            } catch (_: Exception) { }
        }
    }

    fun disconnect() {
        io.execute { cerrarCliente() }
    }

    private fun cerrarCliente() {
        try {
            client?.let {
                if (it.isConnected) it.disconnect()
                it.close()
            }
        } catch (_: Exception) { }
        client = null
        if (connected) notificarConexion(false, "Desconectado")
    }

    private fun procesarMensaje(topic: String, payload: String) {
        try {
            when (topic) {
                TOPIC_ESTADO -> {
                    val j = JSONObject(payload)
                    lastEstado = j.optString("estado", lastEstado)
                    lastModo = j.optString("modo", "")
                    val e = lastEstado; val m = lastModo
                    main.post { listeners.forEach { it.onEstado(e, m) } }
                }
                TOPIC_ACEL -> {
                    val j = JSONObject(payload)
                    lastAcel = j.optDouble("magnitud", 0.0)
                    val a = lastAcel
                    main.post { listeners.forEach { it.onAcel(a) } }
                }
                TOPIC_EVENTO -> {
                    val j = JSONObject(payload)
                    val ev = j.optString("evento", "")
                    val d = j.optString("detalle", "")
                    main.post { listeners.forEach { it.onEvento(ev, d) } }
                }
            }
        } catch (_: Exception) { }
    }

    private fun notificarConexion(c: Boolean, info: String) {
        connected = c
        main.post { listeners.forEach { it.onConnectionChanged(c, info) } }
    }
}
