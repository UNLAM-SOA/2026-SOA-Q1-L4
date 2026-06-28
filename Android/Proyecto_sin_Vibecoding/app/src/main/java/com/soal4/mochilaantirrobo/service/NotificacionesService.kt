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

interface NotificacionesApi {
    @GET("accel")
    suspend fun getNotificaciones(): List<AlertaAcel>
}

object NotificacionesService {
    private const val BASE_URL = "http://192.168.0.250:1880/" // TODO: actualizar URL

    val api: NotificacionesApi by lazy {
        Retrofit.Builder()
            .baseUrl(BASE_URL)
            .addConverterFactory(GsonConverterFactory.create())
            .build()
            .create(NotificacionesApi::class.java)
    }
}