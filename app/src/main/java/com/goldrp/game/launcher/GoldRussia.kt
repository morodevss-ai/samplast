package com.goldrp.game.launcher

import android.app.Application
import android.os.Process
import android.util.Log
import com.google.firebase.FirebaseApp
import com.google.firebase.analytics.FirebaseAnalytics

class GoldRussia : Application() {
    private var mFirebaseAnalytics: FirebaseAnalytics? = null

    override fun onCreate() {
        super.onCreate()

        Thread.setDefaultUncaughtExceptionHandler { thread, throwable ->
            Log.e("CRASH", "Uncaught exception in thread ${thread.name}", throwable)
            throwable.printStackTrace()
            Process.killProcess(Process.myPid())
            System.exit(1)
        }

        try {
            FirebaseApp.initializeApp(this)
            mFirebaseAnalytics = FirebaseAnalytics.getInstance(this)

            Log.d("SProject", "Firebase initialized successfully")

            mFirebaseAnalytics?.let {
                setUserProperties()
            } ?: run {
                Log.w("SProject", "FirebaseAnalytics instance is null")
            }

        } catch (e: Exception) {
            Log.e("SProject", "Error initializing Firebase: ${e.message}", e)
        }
    }

    private fun setUserProperties() {
        try {
            mFirebaseAnalytics?.apply {
                setUserProperty("app_version", "0.8.2.1")
                setUserProperty("device_architecture", "ARMx64")
                setUserProperty("build_type", "release")
            }
            Log.d("SProject", "User properties set successfully")
        } catch (e: Exception) {
            Log.e("SProject", "Error setting user properties: ${e.message}", e)
        }
    }

    fun getFirebaseAnalytics(): FirebaseAnalytics? = mFirebaseAnalytics
}