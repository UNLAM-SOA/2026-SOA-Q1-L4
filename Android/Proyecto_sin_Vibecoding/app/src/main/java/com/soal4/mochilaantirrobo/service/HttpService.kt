package com.soal4.mochilaantirrobo.service

import retrofit2.Retrofit
import retrofit2.converter.gson.GsonConverterFactory
import retrofit2.http.GET

data class AlertaAcel(
    val time: String,
    val x: Double,
    val y: Double,
    val z: Double
)

data class Estado(
    val estado: String
)


interface NotificacionesApi {
    @GET("accel")
    suspend fun getNotificaciones(): List<AlertaAcel>

    @GET("state")
    suspend fun getEstado(): List<Estado>
}

object HttpService {
    private const val BASE_URL = "https://meg-extrapolative-susana.ngrok-free.dev/" // TODO: actualizar URL

    val api: NotificacionesApi by lazy {
        Retrofit.Builder()
            .baseUrl(BASE_URL)
            .addConverterFactory(GsonConverterFactory.create())
            .build()
            .create(NotificacionesApi::class.java)
    }
}