package com.example.myapplication

data class JokeFlags(
    val nsfw: Boolean,
    val religious: Boolean,
    val political: Boolean,
    val racist: Boolean,
    val sexist: Boolean,
    val explicit: Boolean
) {
    fun getFlags(): List<String> {
        val flags = mutableListOf<String>()

        if(nsfw)      flags.add("nsfw")
        if(religious) flags.add("religious")
        if(political) flags.add("political")
        if(racist)    flags.add("racist")
        if(sexist)    flags.add("sexist")
        if(explicit)  flags.add("explicit")

        return flags
    }
}


data class Joke(
    val category: String,
    val type: String,
    val joke: String?,
    val setup: String?,
    val delivery: String?,
    val flags: JokeFlags,
)
