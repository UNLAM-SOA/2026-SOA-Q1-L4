package com.soal4.mochilaantirrobo.service

import android.content.Context
import android.util.Log
import androidx.credentials.CredentialManager
import androidx.credentials.GetCredentialRequest
import androidx.credentials.exceptions.GetCredentialCancellationException
import androidx.credentials.exceptions.GetCredentialException
import androidx.credentials.exceptions.NoCredentialException
import com.google.android.libraries.identity.googleid.GetGoogleIdOption
import com.google.android.libraries.identity.googleid.GoogleIdTokenCredential
import com.google.firebase.auth.FirebaseAuth
import com.google.firebase.auth.FirebaseAuthException
import com.google.firebase.auth.FirebaseUser
import com.google.firebase.auth.GoogleAuthProvider
import com.soal4.mochilaantirrobo.R
import kotlinx.coroutines.tasks.await

/**
 * Resultado de una operación de autenticación, para que la UI muestre errores.
 */
sealed interface ResultadoAuth {
    data object Exito : ResultadoAuth
    data class Error(val mensaje: String) : ResultadoAuth
}

/**
 * Envuelve FirebaseAuth. Singleton al estilo de [MqttNotifierService].
 * Las pantallas llaman a estas funciones suspend desde un coroutine scope.
 */
object AuthService {

    private val auth: FirebaseAuth by lazy { FirebaseAuth.getInstance() }

    /** Usuario con sesión activa, o null. Lo usa el splash para decidir la ruta. */
    fun usuarioActual(): FirebaseUser? = auth.currentUser

    suspend fun iniciarSesionEmail(email: String, pass: String): ResultadoAuth =
        try {
            auth.signInWithEmailAndPassword(email.trim(), pass).await()
            ResultadoAuth.Exito
        } catch (e: FirebaseAuthException) {
            Log.e("AUTH", "Error al iniciar sesión: ${e.message}")
            ResultadoAuth.Error("No se pudo iniciar sesión. Verificá tus datos.")
        } catch (e: Exception) {
            Log.e("AUTH", "Error de red al iniciar sesión: ${e.message}")
            ResultadoAuth.Error("Error de conexión. Intentá de nuevo.")
        }

    suspend fun registrarEmail(email: String, pass: String): ResultadoAuth =
        try {
            auth.createUserWithEmailAndPassword(email.trim(), pass).await()
            ResultadoAuth.Exito
        } catch (e: FirebaseAuthException) {
            Log.e("AUTH", "Error al registrar: ${e.message}")
            ResultadoAuth.Error("No se pudo crear la cuenta. Puede que el email ya esté en uso.")
        } catch (e: Exception) {
            Log.e("AUTH", "Error de red al registrar: ${e.message}")
            ResultadoAuth.Error("Error de conexión. Intentá de nuevo.")
        }

    /**
     * Inicia sesión con Google mediante Credential Manager.
     * [context] debe ser el contexto de la Activity (para poder mostrar el selector).
     */
    suspend fun iniciarSesionGoogle(context: Context): ResultadoAuth =
        try {
            val credentialManager = CredentialManager.create(context)

            val googleIdOption = GetGoogleIdOption.Builder()
                .setFilterByAuthorizedAccounts(false)
                .setServerClientId(context.getString(R.string.default_web_client_id))
                .build()

            val request = GetCredentialRequest.Builder()
                .addCredentialOption(googleIdOption)
                .build()

            val resultado = credentialManager.getCredential(context, request)
            val googleCredential = GoogleIdTokenCredential.createFrom(resultado.credential.data)

            val firebaseCredential = GoogleAuthProvider.getCredential(googleCredential.idToken, null)
            auth.signInWithCredential(firebaseCredential).await()

            ResultadoAuth.Exito
        } catch (e: GetCredentialCancellationException) {
            // El usuario cerró el selector: no es un error a mostrar de forma ruidosa.
            ResultadoAuth.Error("")
        } catch (e: NoCredentialException) {
            Log.e("AUTH", "Sin cuentas de Google disponibles: ${e.message}")
            ResultadoAuth.Error("No hay cuentas de Google en el dispositivo.")
        } catch (e: GetCredentialException) {
            Log.e("AUTH", "Error de Credential Manager: ${e.message}")
            ResultadoAuth.Error("No se pudo iniciar sesión con Google.")
        } catch (e: Exception) {
            Log.e("AUTH", "Error al iniciar sesión con Google: ${e.message}")
            ResultadoAuth.Error("No se pudo iniciar sesión con Google.")
        }

    /** Cierra la sesión de Firebase y limpia el estado de credenciales. */
    suspend fun cerrarSesion(context: Context) {
        auth.signOut()
        try {
            CredentialManager.create(context)
                .clearCredentialState(androidx.credentials.ClearCredentialStateRequest())
        } catch (e: Exception) {
            Log.e("AUTH", "Error al limpiar credenciales: ${e.message}")
        }
    }
}
