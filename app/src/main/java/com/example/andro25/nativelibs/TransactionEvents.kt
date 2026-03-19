package com.example.andro25.nativelibs

interface TransactionEvents {
    fun enterPin(ptc: Int, amount: String)
    fun transactionResult(result: Boolean)
}