package com.example.myapplication

import android.content.Context
import android.graphics.Rect
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import android.widget.Toast
import androidx.recyclerview.widget.RecyclerView
import com.google.android.flexbox.FlexDirection
import com.google.android.flexbox.FlexWrap
import com.google.android.flexbox.FlexboxLayoutManager
import com.google.android.flexbox.JustifyContent

class JokeViewHolder(itemView: View, private val activityContext: Context) : RecyclerView.ViewHolder(itemView) {
    private lateinit var joke: Joke

    val categoryTextView = itemView.findViewById<TextView>(R.id.jokeOneCategoryTextView)
    val typeTextView = itemView.findViewById<TextView>(R.id.jokeOneTypeTextView)
    val textTextView = itemView.findViewById<TextView>(R.id.jokeOneTextTextView)
    val deliveryTextView = itemView.findViewById<TextView>(R.id.jokeOneDeliveryTextView)
    val flagRecyclerView = itemView.findViewById<RecyclerView>(R.id.jokeOneFlagRecyclerView)

    init {
        itemView.setOnClickListener {
            onItemClick()
        }
    }

    fun bind(joke: Joke) {
        this.joke = joke

        categoryTextView.text = joke.category

        typeTextView.text = joke.type

        if (joke.type == "single") {
            textTextView.text = joke.joke ?: "No joke available"
        } else {
            textTextView.text= joke.setup ?: "No setup available"
        }

        deliveryTextView.text = joke.delivery

        flagRecyclerView.layoutManager = FlexboxLayoutManager(activityContext).apply {
            flexDirection = FlexDirection.ROW
            flexWrap = FlexWrap.WRAP
            justifyContent = JustifyContent.FLEX_START
        }

        flagRecyclerView.addItemDecoration(object : RecyclerView.ItemDecoration() {
            override fun getItemOffsets(
                outRect: Rect,
                view: View,
                parent: RecyclerView,
                state: RecyclerView.State
            ) {
                outRect.set(0,0,16,0)
            }
        })

        flagRecyclerView.adapter = FlagAdapter(joke.flags.getFlags())
    }

    private fun toggleDeliveryField() {
        if(deliveryTextView.visibility == View.VISIBLE)
            deliveryTextView.visibility = View.GONE
        else
            deliveryTextView.visibility = View.VISIBLE
    }

    private fun onItemClick() {
        if(joke.type == "single")
            Toast.makeText(activityContext, "This joke is of type SINGLE", Toast.LENGTH_SHORT).show()
        else
            toggleDeliveryField()
    }
}

class JokeAdapter(private val jokes: List<Joke>, private val activityContext: Context): RecyclerView.Adapter<JokeViewHolder>() {
    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): JokeViewHolder {
        val view = LayoutInflater.from(parent.context)
            .inflate(R.layout.joke_one, parent, false)
        return JokeViewHolder(view, activityContext)
    }

    override fun onBindViewHolder(holder: JokeViewHolder, position: Int) {
        val joke = jokes[position]
        holder.bind(joke)
    }

    override fun getItemCount(): Int = jokes.size
}