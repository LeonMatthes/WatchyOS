package com.example.watchyoscompanionapp

import android.os.ParcelUuid
import java.util.*
import kotlin.experimental.inv

val WATCHYOS_SERVICE_UUID : UUID = UUID.fromString("f9ce43b7-d389-4add-adf7-82811c462ca1")
val WATCHYOS_TIME_CHARACTERISTIC_UUID : UUID = UUID.fromString("e7e3232e-88c0-452f-abd1-003cc2ec24d3")
val WATCHYOS_NOTIFICATIONS_CHARACTERISTIC_UUID : UUID = UUID.fromString("0e207741-1657-4e87-9415-ca2a67af12e5")
val WATCHYOS_STATE_CHARACTERISTIC_UUID : UUID = UUID.fromString("54ea5218-bcc6-4870-baa9-06f25ab86b32")

enum class WatchyBLEState(val value: Byte) {
    REBOOT(1), // should update time and notifications
    FAST_UPDATE(2),
    CONNECTION(3),
    DISCONNECT(-1) // 0xFF
}

enum class WatchyNotificationCommands(val value: Byte) {
    CREATE(0),
    REMOVE(1),
    REMOVE_ALL(2)
}

enum class WatchyAppId(val value: Byte) {
    WHATSAPP(0),
    EMAIL(1),
    UNKNOWN(-1) // 0xFF
}