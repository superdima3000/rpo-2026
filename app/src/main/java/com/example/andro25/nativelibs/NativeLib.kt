package com.example.andro25.nativelibs

import android.util.Log


object NativeLib {
    private const val TAG = "MainActivity"
    external fun stringFromJNI(): String
    external fun initRng(): Int
    external fun randomBytes(no: Int): ByteArray
    external fun encrypt(key: ByteArray, data: ByteArray): ByteArray

    external fun decrypt(key: ByteArray, data: ByteArray): ByteArray

    init {

        System.loadLibrary("mbedcrypto")
        System.loadLibrary("andro25")
        Log.i(TAG, "Libraries loaded successfully")
    }
}