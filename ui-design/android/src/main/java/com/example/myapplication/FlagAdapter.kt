package com.example.myapplication

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView

class FlagViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
    val text = itemView.findViewById<TextView>(R.id.flagOneTextView)

    fun bind(flag: String) {
        this.text.text = flag
    }
}

class FlagAdapter(private val flags: List<String>): RecyclerView.Adapter<FlagViewHolder>() {
    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): FlagViewHolder {
        val view = LayoutInflater.from(parent.context)
            .inflate(R.layout.flag_one, parent, false)
        return FlagViewHolder(view)
    }

    override fun onBindViewHolder(holder: FlagViewHolder, position: Int) {
        val flag = flags[position]
        holder.bind(flag)
    }

    override fun getItemCount(): Int = flags.size
}
