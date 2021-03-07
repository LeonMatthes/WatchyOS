package com.example.watchyoscompanionapp

import java.util.*

abstract class BLECommand

data class WriteGattCommand(val uuid: UUID,
                            val writeType: Int,
                            val data: ByteArray,
                            val callback: (Boolean) -> Unit = {}) : BLECommand()

data class ReadGattCommand(val uuid: UUID) : BLECommand()