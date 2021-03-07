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
    private val commandQueue: Queue<BLECommand> = LinkedList<BLECommand>()

    fun pushCommand(command: BLECommand) {
        commandQueue.add(command)
    }

    private fun writeCommand(command: WriteGattCommand) {
        val characteristic = getCharacteristic(command.uuid)
        val gatt = this.gatt
        if(characteristic == null || gatt == null) {
            commandQueue.remove()
            return dispatch()
        }

        characteristic.writeType = command.writeType
        characteristic.value = command.data
        gatt.writeCharacteristic(characteristic)
    }

    private fun readCommand(command: ReadGattCommand) {
        val characteristic = getCharacteristic(command.uuid)
        val gatt = this.gatt
        if(characteristic == null || gatt == null) {
            commandQueue.remove()
            return dispatch()
        }

        gatt.readCharacteristic(characteristic)
    }

    private fun dispatch() {
        val gatt = this.gatt ?: return

        if(!commandQueue.isEmpty()) {
            when(val command = commandQueue.peek()) {
                is WriteGattCommand -> writeCommand(command)
                is ReadGattCommand -> readCommand(command)
            }
        }
        else {
            gatt.disconnect()
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

        val command = commandQueue.peek()
        if(command is WriteGattCommand && command.uuid == characteristic.uuid) {
            commandQueue.remove()

            when (status) {
                BluetoothGatt.GATT_SUCCESS -> {
                    Log.d(TAG, "Successfully wrote characteristic: ${characteristic.uuid}")

                    command.callback(true)
                }
                BluetoothGatt.GATT_WRITE_NOT_PERMITTED -> {
                    Log.w(TAG, "Write to characteristic: ${characteristic.uuid} not permitted")
                    command.callback(false)
                }
                else -> {
                    Log.w(TAG, "Characteristic '${characteristic.uuid}' write failed with status: $status!")
                    command.callback(false)
                }
            }

            dispatch()
        }
        else {
            Log.w(TAG, "wrote non-queued characteristic: ${characteristic.uuid}")
            // don't dispatch, there must still be an incoming write result
        }
    }

    override fun onCharacteristicRead(gatt: BluetoothGatt, characteristic: BluetoothGattCharacteristic, status: Int) {
        super.onCharacteristicRead(gatt, characteristic, status)

        val command = commandQueue.peek()
        if(command is ReadGattCommand && command.uuid == characteristic.uuid) {
            commandQueue.remove()

            when (status) {
                BluetoothGatt.GATT_SUCCESS -> {
                    Log.d(TAG, "Successfully read characteristic: ${characteristic.uuid}")

                    connectionService.onCharacteristicRead(characteristic)
                }
                BluetoothGatt.GATT_READ_NOT_PERMITTED -> {
                    Log.w(TAG, "Read of non-readable characteristic: ${characteristic.uuid}")
                }
                else -> {
                    Log.w(TAG, "Read of ${characteristic.uuid} returned with status: $status...")
                }
            }

            dispatch()
        }
        else {
            Log.w(TAG, "Read non-queued characteristic ${characteristic.uuid}")
            // There must still be another read incoming - do nothing
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
        successfullyConnected()
    }

    private fun successfullyConnected() {
        commandQueue.clear()
        commandQueue.add(ReadGattCommand(WATCHYOS_STATE_CHARACTERISTIC_UUID))
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
                // Without the delay, after a disconnect we will reconnect immediately, without Watchy ever disconnecting
                SystemClock.sleep(1500)
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