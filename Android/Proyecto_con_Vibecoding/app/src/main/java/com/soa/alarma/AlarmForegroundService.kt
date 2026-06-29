package com.soa.alarma

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.content.Intent
import android.os.Build
import android.os.IBinder
import androidx.core.app.NotificationCompat

/**
 * AlarmForegroundService
 * -----------------------------------------------------------------------------
 * Servicio en PRIMER PLANO (foreground service) que mantiene viva la conexion
 * MQTT aunque la app no este en pantalla. Es el ejemplo de "ejecucion en
 * segundo plano" que pide la consigna (concepto de Service de Android).
 *
 * - Muestra una notificacion persistente con el estado de la alarma.
 * - Se suscribe como Listener de MqttManager y actualiza la notificacion cuando
 *   cambia el estado o la conexion.
 */
class AlarmForegroundService : Service(), MqttManager.Listener {

    companion object {
        const val EXTRA_IP = "broker_ip"
        private const val CANAL_ID = "alarma_canal"
        private const val NOTIF_ID = 1
    }

    private var textoEstado = "Conectando..."

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onCreate() {
        super.onCreate()
        crearCanal()
        MqttManager.addListener(this)
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        val ip = intent?.getStringExtra(EXTRA_IP).orEmpty()
        startForeground(NOTIF_ID, construirNotificacion(textoEstado))
        if (ip.isNotBlank()) MqttManager.connect(ip)
        // Si el sistema mata el servicio, que lo reinicie.
        return START_STICKY
    }

    override fun onDestroy() {
        MqttManager.removeListener(this)
        MqttManager.disconnect()
        super.onDestroy()
    }

    // --- Listener de MqttManager ---
    override fun onConnectionChanged(connected: Boolean, info: String) {
        textoEstado = info
        actualizarNotificacion()
    }

    override fun onEstado(estado: String, modo: String) {
        textoEstado = "Estado: $estado ${if (modo.isNotEmpty()) "($modo)" else ""}"
        actualizarNotificacion()
    }

    override fun onAcel(magnitud: Double) { /* no afecta la notificacion */ }
    override fun onEvento(evento: String, detalle: String) { /* opcional */ }

    private fun crearCanal() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val canal = NotificationChannel(
                CANAL_ID,
                "Servicio de alarma",
                NotificationManager.IMPORTANCE_LOW
            ).apply { description = "Mantiene la conexion con el dispositivo antirrobos" }
            getSystemService(NotificationManager::class.java).createNotificationChannel(canal)
        }
    }

    private fun construirNotificacion(texto: String): Notification =
        NotificationCompat.Builder(this, CANAL_ID)
            .setContentTitle("Alarma Antirrobos")
            .setContentText(texto)
            .setSmallIcon(android.R.drawable.ic_lock_idle_alarm)
            .setOngoing(true)
            .setPriority(NotificationCompat.PRIORITY_LOW)
            .build()

    private fun actualizarNotificacion() {
        getSystemService(NotificationManager::class.java)
            .notify(NOTIF_ID, construirNotificacion(textoEstado))
    }
}
