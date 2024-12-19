package com.example.myapplication

import retrofit2.Retrofit
import retrofit2.http.GET
import retrofit2.http.Path
import retrofit2.http.Query
import retrofit2.converter.gson.GsonConverterFactory

private data class SingleJokeResponse(
    var error: Boolean,
    val category: String,
    val type: String,
    val joke: String?,
    val setup: String?,
    val delivery: String?,
    val flags: JokeFlags
) {
    fun getJoke(): Joke {
        return Joke(category, type, joke, setup, delivery, flags)
    }
}

private data class MultiJokeResponse(
    var error: Boolean,
    val amount: Int,
    val jokes: List<Joke>
)

private interface JokeApiService {
    @GET("joke/{category}")
    suspend fun getJokes(
        @Path("category") category: String,
        @Query("amount") amount: Int
    ): MultiJokeResponse

    @GET("joke/{category}")
    suspend fun getJoke(
        @Path("category") category: String,
    ): SingleJokeResponse
}

class API {
    companion object {
       private val retrofit: Retrofit by lazy {
           Retrofit.Builder()
               .baseUrl("https://v2.jokeapi.dev/")
               .addConverterFactory(GsonConverterFactory.create())
               .build()
       }

       private val api: JokeApiService by lazy {
           retrofit.create(JokeApiService::class.java)
       }

       private suspend fun getCategory(category: String, amount: Int?): List<Joke> {
           return try {
               if (amount == null || amount == 1) {
                   val response = api.getJoke(category)
                   listOf(response.getJoke())
               } else {
                   val response = api.getJokes(category, amount)
                   response.jokes
               }
           } catch (e: Exception) {
               emptyList()
           }
       }

       suspend fun getAny         (amount: Int?): List<Joke> = getCategory("Any", amount)
       suspend fun getMisc        (amount: Int?): List<Joke> = getCategory("Misc", amount)
       suspend fun getProgramming (amount: Int?): List<Joke> = getCategory("Programming", amount)
       suspend fun getDark        (amount: Int?): List<Joke> = getCategory("Dark", amount)
       suspend fun getPun         (amount: Int?): List<Joke> = getCategory("Pun", amount)
       suspend fun getSpooky      (amount: Int?): List<Joke> = getCategory("Spooky", amount)
       suspend fun getChristmas   (amount: Int?): List<Joke> = getCategory("Christmas", amount)
   }
}