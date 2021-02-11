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

private const val TAG = "WatchyGattCallback"

class WatchyGattCallback(private val connectionService: WatchyConnectionService) : BluetoothGattCallback() {

    var gatt: BluetoothGatt? = null

    // warning! These queues will be reset on connect
    private val writeQueue : Queue<Pair<BluetoothGattCharacteristic, (Boolean) -> Unit>> = LinkedList<Pair<BluetoothGattCharacteristic, (Boolean) -> Unit>>()
    private val readQueue: Queue<BluetoothGattCharacteristic> = LinkedList<BluetoothGattCharacteristic>()

    fun pushWrite(characteristic: BluetoothGattCharacteristic, successCallback: (Boolean) -> Unit = {}) {
        writeQueue.add(Pair(characteristic, successCallback))
    }

    fun pushRead(characteristic: BluetoothGattCharacteristic) {
        readQueue.add(characteristic)
    }

    private fun dispatch() {
        val gatt = this.gatt ?: return

        when {
            !readQueue.isEmpty() -> gatt.readCharacteristic(readQueue.peek())
            !writeQueue.isEmpty() -> gatt.writeCharacteristic(writeQueue.peek()?.first)
            else -> gatt.disconnect()
        }
    }

    fun getCharacteristic(characteristicUUID: UUID, serviceUUID: UUID = WATCHYOS_SERVICE_UUID): BluetoothGattCharacteristic? {
        val gatt = this.gatt ?: return null
        if (gatt.services.isEmpty()) {
            Log.w(TAG, "No service and characteristic available, call discoverServices() first?")
            return null
        }

        return gatt.getService(serviceUUID)?.getCharacteristic(characteristicUUID)
    }

    override fun onCharacteristicWrite(gatt: BluetoothGatt, characteristic: BluetoothGattCharacteristic, status: Int) {
        super.onCharacteristicWrite(gatt, characteristic, status)

        if(writeQueue.peek()?.first?.uuid == characteristic.uuid) {
            when (status) {
                BluetoothGatt.GATT_SUCCESS -> {
                    Log.d(TAG, "Successfully wrote characteristic: ${characteristic.uuid}")

                    writeQueue.remove().second(true)
                }
                BluetoothGatt.GATT_WRITE_NOT_PERMITTED -> {
                    Log.w(TAG, "Write to characteristic: ${characteristic.uuid} not permitted")
                    writeQueue.remove().second(false)
                }
                else -> {
                    Log.w(TAG, "Characteristic write failed with status: $status!")
                    writeQueue.remove().second(false)
                }
            }
        }
        else {
            Log.w(TAG, "wrote non-queued characteristic: ${characteristic.uuid}")
        }
        dispatch()
    }

    override fun onCharacteristicRead(gatt: BluetoothGatt, characteristic: BluetoothGattCharacteristic, status: Int) {
        super.onCharacteristicRead(gatt, characteristic, status)

        if(readQueue.peek()?.uuid == characteristic.uuid) {
            when (status) {
                BluetoothGatt.GATT_SUCCESS -> {
                    Log.d(TAG, "Successfully read characteristic: ${characteristic.uuid}")

                    connectionService.onCharacteristicRead(characteristic)
                    readQueue.remove()
                }
                BluetoothGatt.GATT_READ_NOT_PERMITTED -> {
                    Log.w(TAG, "Read of non-readable characteristic: ${characteristic.uuid}")
                    readQueue.remove()
                }
                else -> {
                    Log.w(TAG, "Read of ${characteristic.uuid} returned with status: $status, retrying one more time...")
                    gatt.readCharacteristic(readQueue.remove())
                    return
                }
            }
        }
        else {
            Log.w(TAG, "Read non-queued characteristic ${characteristic.uuid}")
        }
        dispatch()
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
        successfullyConnected()
    }

    private fun successfullyConnected() {
        readQueue.clear()
        writeQueue.clear()
        val characteristic = getCharacteristic(WATCHYOS_STATE_CHARACTERISTIC_UUID)
        if (characteristic != null) {
            readQueue.add(characteristic)
        }
        dispatch()
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
                    successfullyConnected()
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

    fun stop() {
        gatt?.disconnect()
        gatt?.close()
    }
}