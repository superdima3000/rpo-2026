package com.example.andro25

import android.content.Intent
import android.os.Bundle
import android.view.View
import android.widget.Button
import android.widget.TextView
import androidx.activity.ComponentActivity
import com.example.andro25.nativelibs.NativeLib.randomBytes
import kotlin.experimental.and


class PinpadActivity : ComponentActivity() {

    private lateinit var tvPin: TextView
    private var pin: String = ""

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_pinpad)

        tvPin = findViewById<TextView>(R.id.txtPin)

        shuffleKeys()

        findViewById<Button>(R.id.btnOK).setOnClickListener {
            val it = Intent()
            it.putExtra("pin", pin)
            setResult(RESULT_OK, it)
            finish()
        }

        findViewById<Button>(R.id.btnReset).setOnClickListener {
            pin = ""
            tvPin.text = pin
        }
    }

    fun keyClick(v: View) {
        val key = (v as TextView).text.toString()
        val sz = pin.length + 1

        if (sz <= 4) {
            pin += key
            tvPin.text = "*".repeat(sz)
        }
    }

    private fun shuffleKeys() {
        val keys = arrayOf(
            findViewById<Button>(R.id.btnKey0),
            findViewById<Button>(R.id.btnKey1),
            findViewById<Button>(R.id.btnKey2),
            findViewById<Button>(R.id.btnKey3),
            findViewById<Button>(R.id.btnKey4),
            findViewById<Button>(R.id.btnKey5),
            findViewById<Button>(R.id.btnKey6),
            findViewById<Button>(R.id.btnKey7),
            findViewById<Button>(R.id.btnKey8),
            findViewById<Button>(R.id.btnKey9),
        )

        val rnd = randomBytes(MAX_KEYS)
        for (i in 0 until MAX_KEYS) {
            val idx = (rnd[i].toInt() and 0xFF) % 10
            val text = keys[idx].text

            keys[idx].text = keys[i].text
            keys[i].text = text
        }
    }

    companion object {
        private const val MAX_KEYS = 10
    }
}