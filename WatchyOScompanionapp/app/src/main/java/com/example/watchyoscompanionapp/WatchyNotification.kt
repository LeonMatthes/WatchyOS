package com.example.watchyoscompanionapp

import android.app.Notification
import android.service.notification.StatusBarNotification
import android.util.Log
import java.lang.StringBuilder
import java.util.*

data class WatchyNotification(val id: Byte, var sbn: StatusBarNotification) {
    private val extras
        get() = sbn.notification.extras

    fun text() : String {
        val lines = extras.getCharSequenceArray(Notification.EXTRA_TEXT_LINES)
        if(lines != null && lines.isNotEmpty()) {
            val stringBuilder = StringBuilder()
            for(msg in lines) {
                if(msg.isNotEmpty()) {
                    stringBuilder.append(msg.toString())
                    stringBuilder.append('\n')
                }
            }
            return stringBuilder.toString().trim()
        }

        val chars = extras.getCharSequence(Notification.EXTRA_BIG_TEXT)
        if(chars != null && chars.isNotEmpty()) {
            return chars.toString()
        }

        return extras.get(Notification.EXTRA_TEXT)?.toString() ?: ""
    }

    fun title() : String {
        return extras.get(Notification.EXTRA_TITLE)?.toString() ?: ""
    }

    fun removalPayload() : ByteArray {
        return byteArrayOf(WatchyNotificationCommands.REMOVE.value, id)
    }

    private fun appId() : Byte {
        return when(sbn.packageName) {
            "com.whatsapp" -> WatchyAppId.WHATSAPP
            "com.samsung.android.email.provider" -> WatchyAppId.EMAIL
            else -> WatchyAppId.UNKNOWN
        }.value
    }

    fun creationPayload() : ByteArray {
        val title = watchyCompatible(title()).replace(Regex("[\\n]"), " ").take(25)

        val text = watchyCompatible(text()).take(150)

        // Add +1 for null bytes
        val bytes = ByteArray(10 + title.length + 1 + text.length + 1)

        bytes[0] = WatchyNotificationCommands.CREATE.value
        bytes[1] = id
        bytes[2] = appId()

        val date = Date(sbn.notification.`when`)
        bytes[3] = (date.year - 70).toByte()
        bytes[4] = (date.month + 1).toByte()
        bytes[5] = date.date.toByte()
        bytes[6] = (date.day + 1).toByte() // Sunday == 1, Saturday == 7
        bytes[7] = date.hours.toByte()
        bytes[8] = date.minutes.toByte()
        bytes[9] = date.seconds.toByte()

        var i = 10
        for(byte in title.toByteArray()) {
            bytes[i++] = byte
        }

        bytes[i++] = '\u0000'.toByte()
        for(byte in text.toByteArray()) {
            bytes[i++] = byte
        }

        bytes[bytes.lastIndex] = '\u0000'.toByte()

        return bytes
    }
}
