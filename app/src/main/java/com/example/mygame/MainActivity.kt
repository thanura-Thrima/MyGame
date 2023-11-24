package com.example.mygame

import android.os.Bundle

import android.util.Log
import android.view.View
import android.widget.FrameLayout
import android.widget.TextView
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.core.view.ViewCompat
import com.google.androidgamesdk.GameActivity

class MainActivity : GameActivity() {
    companion object {
        init {
            System.loadLibrary("mygame")
        }
    }

    override fun onCreateSurfaceView() {
        this.setContentView(R.layout.main_activity)
        Log.e("raveen", "calling self")
        mSurfaceView = InputEnabledSurfaceView(this)
        val frameLayout = findViewById<FrameLayout>(R.id.frame_layout)
        frameLayout.addView(mSurfaceView)
        val mTextView = TextView(this)
        mTextView.text = "hello"
        frameLayout.addView(mTextView)
        frameLayout.requestFocus()
        mSurfaceView.holder.addCallback(this)
        ViewCompat.setOnApplyWindowInsetsListener(mSurfaceView, this)
    }

    override fun onCreate(savedInstanceState: Bundle?)  {
        Log.e("raveen", "calling onCreate")
        super.onCreate(savedInstanceState)
    }

    @Composable
    fun Greeting(name: String, modifier: Modifier = Modifier) {
        Surface() {

                    Text(
                        text = name,
                        modifier = modifier
                    )
        }
    }
    override fun onWindowFocusChanged(hasFocus: Boolean) {
        super.onWindowFocusChanged(hasFocus)
        if (hasFocus) {
            hideSystemUi()
        }
    }

    private fun hideSystemUi() {
        val decorView = window.decorView
        decorView.systemUiVisibility = (View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                or View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                or View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                or View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                or View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                or View.SYSTEM_UI_FLAG_FULLSCREEN)
    }
}