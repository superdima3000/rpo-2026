package com.example.andro25.util

import org.apache.commons.codec.binary.Hex
import org.apache.commons.codec.DecoderException

fun checkDecoding(original: ByteArray, decoded: ByteArray): Boolean{
    if (original.size != decoded.size) {
        return false
    }

    for (i in 0 until original.size) {
        if (original[i] != decoded[i]){
            return false
        }
    }

    return true
}

fun stringToHex(s: String): ByteArray{
    val hex = try {
        Hex.decodeHex(s.toCharArray())
    } catch (e: DecoderException) {
        ByteArray(0)
    }
    return hex
}

fun hexToString(h: ByteArray) = String(Hex.encodeHex(h)).uppercase()
