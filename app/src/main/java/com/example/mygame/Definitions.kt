package com.example.mygame

import android.util.Log

class Definitions {
    companion object {
        private const val AppTag = "Kotlin_Applicaition"
        enum class LogLevel{
            Info,
            Error,
            Warning,
        }
        fun log(level: LogLevel,message : String)
        {
            if(level == LogLevel.Info)
                Log.i(AppTag,message)
            else if(level == LogLevel.Error)
                Log.e(AppTag,message)
            else
                Log.w(AppTag,message)
        }
    }
}