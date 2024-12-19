package com.example.myapplication

import kotlinx.coroutines.*
import android.os.Bundle
import android.widget.Toast
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import android.view.Menu
import android.view.MenuItem
import android.view.View
import android.widget.ProgressBar
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers

class MainActivity : AppCompatActivity() {
    private lateinit var progressBar: ProgressBar
    private lateinit var jokes: List<Joke>
    private lateinit var menu: Menu
    private var selectedItem = R.id.anyCategoryMenuItem

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContentView(R.layout.activity_main)
        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main)) { v, insets ->
            val systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars())
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom)
            insets
        }

        progressBar = findViewById(R.id.loadingSpinnerProgressBar)

        loadJokes(API::getAny)
    }

    private fun loadJokes(jokeLoader: suspend (Int?) -> List<Joke>) {
        jokes = emptyList()
        renderJokes()

        progressBar.visibility = View.VISIBLE

        CoroutineScope(Dispatchers.Main).launch {
            val jokeLoadingJob = launch {
                jokes = jokeLoader(10)
            }

            delay(2000)
            jokeLoadingJob.join()

            progressBar.visibility = View.GONE
            renderJokes()
        }
    }

    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        this.menu = menu
        menuInflater.inflate(R.menu.jokes_top_menu, menu)
        return true
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        val selectedCategory: suspend (Int?) -> List<Joke> = when (item.itemId) {
            R.id.anyCategoryMenuItem -> API::getAny
            R.id.christmasCategoryMenuItem -> API::getChristmas
            R.id.programmingCategoryMenuItem -> API::getProgramming
            R.id.miscCategoryMenuItem -> API::getMisc
            R.id.darkCategoryMenuItem -> API::getDark
            R.id.punCategoryMenuItem -> API::getPun
            R.id.spookyCategoryMenuItem -> API::getSpooky
            else -> { _: Int? -> emptyList() }
        }

        moveToTop(item)
        loadJokes(selectedCategory)

        return true
    }

    private fun moveToTop(item: MenuItem) {
        val currentActionItem = menu.findItem(selectedItem)
        currentActionItem.setShowAsAction(MenuItem.SHOW_AS_ACTION_NEVER)
        item.setShowAsAction(MenuItem.SHOW_AS_ACTION_ALWAYS)
        selectedItem = item.itemId
    }

    private fun renderJokes() {
        val recyclerView = findViewById<RecyclerView>(R.id.jokeRecyclerView)
        val jokeAdapter = JokeAdapter(jokes,this)
        recyclerView.layoutManager = LinearLayoutManager(this)
        recyclerView.adapter = jokeAdapter
    }
}
