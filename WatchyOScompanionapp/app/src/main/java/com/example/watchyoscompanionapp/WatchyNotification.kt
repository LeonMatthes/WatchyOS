package com.example.watchyoscompanionapp

import android.app.Notification
import android.service.notification.StatusBarNotification
import android.util.Log
import java.util.*

data class WatchyNotification(val id: Byte, var sbn: StatusBarNotification) {
    private val extras
        get() = sbn.notification.extras

    private fun text() : String {
        return extras.get(Notification.EXTRA_TEXT_LINES)?.toString() ?:
                extras.get(Notification.EXTRA_TEXT)?.toString() ?:
                ""
    }

    private fun title() : String {
        return extras.get(Notification.EXTRA_TITLE)?.toString() ?: "---"
    }

    fun removalPayload() : ByteArray {
        return byteArrayOf(WatchyNotificationCommands.REMOVE.value, id)
    }

    private fun appId() : Byte {
        return when(sbn.packageName) {
            "com.whatsapp" -> WatchyAppId.WHATSAPP
            else -> WatchyAppId.UNKNOWN
        }.value
    }

    fun creationPayload() : ByteArray {
        val cal = Calendar.getInstance()

        val title = title().replace(Regex("[^A-Za-z0-9,.!? \\n]"), "").take(25)

        val text = text().replace(Regex("[^A-Za-z0-9,.! \\n]"), "").take(200)

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