package com.soal4.mochilaantirrobo.service

import io.ktor.client.HttpClient
import io.ktor.client.engine.android.Android
import io.ktor.client.plugins.contentnegotiation.ContentNegotiation
import io.ktor.client.request.get
import io.ktor.client.request.post
import io.ktor.client.request.setBody
import io.ktor.client.statement.HttpResponse
import io.ktor.http.ContentType
import io.ktor.http.contentType
import io.ktor.serialization.kotlinx.json.json

object HttpService {

    private val client = HttpClient(Android) {
        install(ContentNegotiation) {
            json()
        }
    }

    private const val BASE_URL = "http://192.168.01.01:8080" // TODO Actualizar

    suspend fun get(endpoint: String): HttpResponse =
        client.get("$BASE_URL/$endpoint")

    suspend fun post(endpoint: String, body: Any): HttpResponse =
        client.post("$BASE_URL/$endpoint") {
            contentType(ContentType.Application.Json)
            setBody(body)
        }

    fun cerrar() = client.close()
}