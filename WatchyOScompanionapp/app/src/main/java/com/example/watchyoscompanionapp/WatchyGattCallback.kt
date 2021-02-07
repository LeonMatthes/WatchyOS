package com.example.watchyoscompanionapp

import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCallback
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothProfile
import android.os.Handler
import android.os.Looper
import android.os.SystemClock
import android.util.Log
import java.util.*

const val TAG = "WatchyGattCallback"

class WatchyGattCallback : BluetoothGattCallback() {

    var gatt: BluetoothGatt? = null

    private fun writeTimeCharacteristic(gatt: BluetoothGatt) {
        if (gatt.services.isEmpty()) {
            Log.i("Watchy", "No service and characteristic available, call discoverServices() first?")
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
        Log.d("Watchy", "Writing date characteristic to value ${payload.joinToString()}")
    }

    override fun onCharacteristicWrite(gatt: BluetoothGatt, characteristic: BluetoothGattCharacteristic, status: Int) {
        super.onCharacteristicWrite(gatt, characteristic, status)
        if(status == BluetoothGatt.GATT_SUCCESS) {
            Log.d("Watchy", "Successfully wrote characteristic")
            gatt.disconnect()
        }
        else {
            Log.d("Watchy", "Characteristic write failed with status: $status! Retrying")
            writeTimeCharacteristic(gatt)
        }
    }

    private fun BluetoothGatt.printGattTable() {
        if (services.isEmpty()) {
            Log.i("printGattTable", "No service and characteristic available, call discoverServices() first?")
            return
        }
        services.forEach { service ->
            val characteristicsTable = service.characteristics.joinToString(
                    separator = "\n|--",
                    prefix = "|--"
            ) { it.uuid.toString() }
            Log.i("printGattTable", "\nService ${service.uuid}\nCharacteristics:\n$characteristicsTable"
            )
        }
    }

    override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
        with(gatt) {
            Log.w("BluetoothGattCallback", "Discovered ${services.size} services for ${device.address}")
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
                    writeTimeCharacteristic(gatt)
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