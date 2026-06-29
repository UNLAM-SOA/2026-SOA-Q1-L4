package com.soa.alarma

import android.Manifest
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.widget.Button
import android.widget.EditText
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat

/**
 * MainActivity (Activity 1)
 * -----------------------------------------------------------------------------
 * Pantalla principal. Cumple lo pedido por la consigna:
 *   - Un CAMPO DE TEXTO (IP del broker) y un BOTON (Conectar).
 *   - Envia COMANDOS por WiFi/MQTT al embebido (ARMAR / DESARMAR).
 *   - MUESTRA por pantalla el valor de un sensor que envia el embebido
 *     (estado de la alarma + magnitud de aceleracion del MPU6050).
 *
 * Implementa MqttManager.Listener para refrescar la UI cuando llegan mensajes.
 */
class MainActivity : AppCompatActivity(), MqttManager.Listener {

    private lateinit var prefs: android.content.SharedPreferences
    private lateinit var inputIp: EditText
    private lateinit var txtConexion: TextView
    private lateinit var txtEstado: TextView
    private lateinit var txtAcel: TextView

    private val pedirNotificaciones =
        registerForActivityResult(androidx.activity.result.contract.ActivityResultContracts.RequestPermission()) { }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        prefs = getSharedPreferences("config", Context.MODE_PRIVATE)

        inputIp = findViewById(R.id.inputIp)
        txtConexion = findViewById(R.id.txtConexion)
        txtEstado = findViewById(R.id.txtEstado)
        txtAcel = findViewById(R.id.txtAcel)

        inputIp.setText(prefs.getString("broker_ip", "192.168.0.10"))

        findViewById<Button>(R.id.btnConectar).setOnClickListener { conectar() }
        findViewById<Button>(R.id.btnArmar).setOnClickListener { enviarComando("ARM") }
        findViewById<Button>(R.id.btnDesarmar).setOnClickListener { enviarComando("DISARM") }
        findViewById<Button>(R.id.btnSensor).setOnClickListener {
            startActivity(Intent(this, SensorActivity::class.java))
        }

        pedirPermisoNotificaciones()
    }

    override fun onStart() {
        super.onStart()
        MqttManager.addListener(this)
    }

    override fun onStop() {
        super.onStop()
        MqttManager.removeListener(this)
    }

    private fun pedirPermisoNotificaciones() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU &&
            ContextCompat.checkSelfPermission(this, Manifest.permission.POST_NOTIFICATIONS)
            != PackageManager.PERMISSION_GRANTED
        ) {
            pedirNotificaciones.launch(Manifest.permission.POST_NOTIFICATIONS)
        }
    }

    private fun conectar() {
        val ip = inputIp.text.toString().trim()
        if (ip.isEmpty()) {
            Toast.makeText(this, "Ingresa la IP del broker", Toast.LENGTH_SHORT).show()
            return
        }
        prefs.edit().putString("broker_ip", ip).apply()

        // Arranca (o reinicia) el servicio en primer plano que mantiene la conexion.
        val intent = Intent(this, AlarmForegroundService::class.java)
            .putExtra(AlarmForegroundService.EXTRA_IP, ip)
        ContextCompat.startForegroundService(this, intent)

        txtConexion.text = "Conectando a $ip ..."
    }

    private fun enviarComando(cmd: String) {
        if (!MqttManager.connected) {
            Toast.makeText(this, "Conectate primero al broker", Toast.LENGTH_SHORT).show()
            return
        }
        MqttManager.publicarComando(cmd)
        Toast.makeText(this, "Comando enviado: $cmd", Toast.LENGTH_SHORT).show()
    }

    // --- Callbacks de MqttManager (corren en el hilo principal) ---
    override fun onConnectionChanged(connected: Boolean, info: String) {
        txtConexion.text = info
    }

    override fun onEstado(estado: String, modo: String) {
        txtEstado.text = if (modo.isNotEmpty()) "$estado\n($modo)" else estado
    }

    override fun onAcel(magnitud: Double) {
        txtAcel.text = getString(R.string.fmt_acel, magnitud)
    }

    override fun onEvento(evento: String, detalle: String) {
        if (evento.isNotEmpty()) {
            Toast.makeText(this, "Evento: $evento", Toast.LENGTH_SHORT).show()
        }
    }
}
