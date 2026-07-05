package com.soal4.mochilaantirrobo

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.text.input.PasswordVisualTransformation
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.navigation.NavController
import com.soal4.mochilaantirrobo.service.AuthService
import com.soal4.mochilaantirrobo.service.ResultadoAuth
import kotlinx.coroutines.launch

@Preview
@Composable
fun PantallaLogin(navController: NavController? = null) {

    val context = LocalContext.current
    val scope = rememberCoroutineScope()

    var email by rememberSaveable { mutableStateOf("") }
    var password by rememberSaveable { mutableStateOf("") }
    var modoRegistro by rememberSaveable { mutableStateOf(false) }
    var cargando by rememberSaveable { mutableStateOf(false) }
    var error by rememberSaveable { mutableStateOf<String?>(null) }

    fun manejarResultado(resultado: ResultadoAuth) {
        cargando = false
        when (resultado) {
            is ResultadoAuth.Exito -> {
                navController?.navigate("ajuste") {
                    popUpTo("login") { inclusive = true }
                }
            }
            is ResultadoAuth.Error -> {
                error = resultado.mensaje.ifBlank { null }
            }
        }
    }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(horizontal = 32.dp),
        verticalArrangement = Arrangement.Center,
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Text(
            text = if (modoRegistro) "Crear cuenta" else "Iniciar sesión",
            fontSize = 28.sp,
            fontWeight = FontWeight.Bold
        )

        Spacer(modifier = Modifier.height(24.dp))

        OutlinedTextField(
            value = email,
            onValueChange = { email = it },
            label = { Text("Email") },
            singleLine = true,
            enabled = !cargando,
            keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Email),
            modifier = Modifier.fillMaxWidth()
        )

        Spacer(modifier = Modifier.height(12.dp))

        OutlinedTextField(
            value = password,
            onValueChange = { password = it },
            label = { Text("Contraseña") },
            singleLine = true,
            enabled = !cargando,
            visualTransformation = PasswordVisualTransformation(),
            keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Password),
            modifier = Modifier.fillMaxWidth()
        )

        if (error != null) {
            Spacer(modifier = Modifier.height(8.dp))
            Text(text = error!!, color = Color(0xFFF44336))
        }

        Spacer(modifier = Modifier.height(20.dp))

        Button(
            enabled = !cargando && email.isNotBlank() && password.isNotBlank(),
            onClick = {
                error = null
                cargando = true
                scope.launch {
                    val resultado = if (modoRegistro) {
                        AuthService.registrarEmail(email, password)
                    } else {
                        AuthService.iniciarSesionEmail(email, password)
                    }
                    manejarResultado(resultado)
                }
            },
            modifier = Modifier.fillMaxWidth()
        ) {
            Text(if (modoRegistro) "Registrarse" else "Iniciar sesión")
        }

        Spacer(modifier = Modifier.height(8.dp))

        OutlinedButton(
            enabled = !cargando,
            onClick = {
                error = null
                cargando = true
                scope.launch {
                    manejarResultado(AuthService.iniciarSesionGoogle(context))
                }
            },
            modifier = Modifier.fillMaxWidth()
        ) {
            Text("Continuar con Google")
        }

        Spacer(modifier = Modifier.height(16.dp))

        TextButton(
            enabled = !cargando,
            onClick = {
                error = null
                modoRegistro = !modoRegistro
            }
        ) {
            Text(
                if (modoRegistro) "¿Ya tenés cuenta? Iniciá sesión"
                else "¿No tenés cuenta? Registrate"
            )
        }

        if (cargando) {
            Spacer(modifier = Modifier.height(16.dp))
            CircularProgressIndicator()
        }
    }
}
