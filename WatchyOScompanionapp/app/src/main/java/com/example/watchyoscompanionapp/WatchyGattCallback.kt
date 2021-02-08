package com.example.watchyoscompanionapp

import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCallback
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothProfile
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.Handler
import android.os.Looper
import android.os.SystemClock
import android.util.Log
import java.util.*

private const val TAG = "WatchyGattCallback"

class WatchyGattCallback() : BluetoothGattCallback() {

    var gatt: BluetoothGatt? = null

    var notification_bits : Byte = 0x00
    var notifications_changed = false

    init {
        val gattCallback = this

    }

    private fun writeTimeCharacteristic(gatt: BluetoothGatt) {
        if (gatt.services.isEmpty()) {
            Log.w(TAG, "No service and characteristic available, call discoverServices() first?")
            return
        }

        val characteristic = gatt
                .getService(UUID.fromString(WATCHYOS_SERVICE_UUID))
                .getCharacteristic(UUID.fromString(WATCHYOS_TIME_CHARACTERISTIC_UUID))

        characteristic.writeType = BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
        val cal = Calendar.getInstance()
        val payload = byteArrayOf(
                (cal.get(Calendar.YEAR) - 1970).toByte(),
                cal.get(Calendar.MONTH).toByte(),
                cal.get(Calendar.DAY_OF_MONTH).toByte(),
                cal.get(Calendar.DAY_OF_WEEK).toByte(), // Sunday == 1
                cal.get(Calendar.HOUR_OF_DAY).toByte(),
                cal.get(Calendar.MINUTE).toByte(),
                cal.get(Calendar.SECOND).toByte()
        )
        characteristic.value = payload
        gatt.writeCharacteristic(characteristic)
        Log.d(TAG, "Writing date characteristic to value ${payload.joinToString()}")
    }

    private fun writeNotificationCharacteristic(gatt: BluetoothGatt) {
        if(gatt.services.isEmpty()) {
            Log.w(TAG, "No service and characteristic available, call discoverServices() first?")
            return
        }

        val characteristic = gatt
                .getService(UUID.fromString(WATCHYOS_SERVICE_UUID))
                .getCharacteristic(UUID.fromString(WATCHYOS_NOTIFICATIONS_CHARACTERISTIC_UUID))

        characteristic.writeType = BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
        characteristic.value = byteArrayOf(notification_bits)
        gatt.writeCharacteristic(characteristic)
    }

    override fun onCharacteristicWrite(gatt: BluetoothGatt, characteristic: BluetoothGattCharacteristic, status: Int) {
        super.onCharacteristicWrite(gatt, characteristic, status)
        if(status == BluetoothGatt.GATT_SUCCESS) {
            Log.d(TAG, "Successfully wrote characteristic")
            gatt.disconnect()

            if(characteristic.uuid.equals(UUID.fromString(WATCHYOS_NOTIFICATIONS_CHARACTERISTIC_UUID))) {
                notifications_changed = false
                Log.d(TAG, "Successfully wrote characteristic")
            }
        }
        else {
            Log.d(TAG, "Characteristic write failed with status: $status! Retrying")
            writeTimeCharacteristic(gatt)
        }
    }

    private fun BluetoothGatt.printGattTable() {
        if (services.isEmpty()) {
            Log.i(TAG, "No service and characteristic available, call discoverServices() first?")
            return
        }
        services.forEach { service ->
            val characteristicsTable = service.characteristics.joinToString(
                    separator = "\n|--",
                    prefix = "|--"
            ) { it.uuid.toString() }
            Log.i(TAG, "\nService ${service.uuid}\nCharacteristics:\n$characteristicsTable"
            )
        }
    }

    override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
        with(gatt) {
            Log.w(TAG, "Discovered ${services.size} services for ${device.address}")
            printGattTable()
        }
        writeTimeCharacteristic(gatt)
    }

    override fun onConnectionStateChange(gatt: BluetoothGatt, status: Int, newState: Int) {
        this.gatt = gatt

        if (status == BluetoothGatt.GATT_SUCCESS) {
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                Log.d(TAG, "Connected to Watchy")
                if(gatt.services.isEmpty()) {
                    Handler(Looper.getMainLooper()).post {
                        gatt.discoverServices()
                    }
                }
                else {
                    if(notifications_changed) {
                        writeNotificationCharacteristic(gatt)
                    }
                    else {
                        writeTimeCharacteristic(gatt)
                    }
                }
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                SystemClock.sleep(10000)
                Log.d(TAG, "Disconnected - Trying to reconnect")
                gatt.connect()
            }
        } else {
            Log.d(TAG, "Error: $status, $newState - Reconnecting")
            gatt.disconnect()
            gatt.connect()
        }
    }
}