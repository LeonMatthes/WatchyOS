package com.example.watchyoscompanionapp

import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGattCharacteristic
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.Build
import android.os.IBinder
import android.util.Log
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationCompat.PRIORITY_LOW
import java.util.*

private val CHANNEL_ID = "ForegroundServiceChannel"

private const val TAG = "WatchyConnectionService"

class WatchyConnectionService : Service() {

    val SERVICE_ID = 1

    var gattCallback = WatchyGattCallback(this)

    var notificationBits : Byte = 0x00
    var watchyStateID : Byte = 0x00
    var phoneStateID : Byte = 0x00

    var receiver: BroadcastReceiver? = null

    override fun onCreate() {
        super.onCreate()
    }

    override fun onStartCommand(intent: Intent, flags: Int, startId: Int): Int {
        createNotificationChannel()
        val notificationIntent = Intent(this, MainActivity::class.java)
        val pendingIntent = PendingIntent.getActivity(this, 0, notificationIntent, 0)

        val notification = NotificationCompat.Builder(this, CHANNEL_ID)
                .setContentTitle(getString(R.string.conn_not_title))
                .setContentText(getString(R.string.conn_not_text))
                .setSmallIcon(R.drawable.ic_notification)
                .setContentIntent(pendingIntent)
                .setPriority(PRIORITY_LOW)
                .build()

        startForeground(SERVICE_ID, notification)

        val bleDevice: BluetoothDevice = intent.getParcelableExtra("WatchyBLEDevice")!!
        bleDevice.connectGatt(this, false, gattCallback)

        receiver = object : BroadcastReceiver() {
            override fun onReceive(context: Context?, intent: Intent) {
                Log.d(TAG, "Received intent")

                if(intent.hasExtra(BuildConfig.APPLICATION_ID + ".NotificationBits")) {
                    val newNotificationBits = intent.getByteExtra(BuildConfig.APPLICATION_ID + ".NotificationBits", 127)
                    if(newNotificationBits != notificationBits) {
                        if (phoneStateID == Byte.MAX_VALUE) {
                            phoneStateID = Byte.MIN_VALUE
                        } else {
                            phoneStateID++
                        }
                    }
                    notificationBits = newNotificationBits
                    Log.d(TAG, "Notifications: $notificationBits")
                }
            }
        }
        val filter = IntentFilter()
        filter.addAction("com.example.watchyoscompanionapp.WATCHY_GATT_CALLBACK")
        registerReceiver(receiver, filter)

        val intent = Intent(BuildConfig.APPLICATION_ID + ".WATCHY_NOTIFICATION_LISTENER")
        // request update from NotificationListener
        intent.putExtra(BuildConfig.APPLICATION_ID+ ".Command", "sendUpdate")
        sendBroadcast(intent)

        return START_NOT_STICKY
    }

    override fun onDestroy() {
        super.onDestroy()

        if(receiver != null) {
            unregisterReceiver(receiver)
        }
        gattCallback.stop()
    }

    override fun onBind(intent: Intent?): IBinder? {
        return null
    }

    private fun writeTimeCharacteristic() {
        val characteristic = gattCallback.getCharacteristic(WATCHYOS_TIME_CHARACTERISTIC_UUID) ?: return

        characteristic.writeType = BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
        val cal = Calendar.getInstance()
        val payload = byteArrayOf(
                (cal.get(Calendar.YEAR) - 1970).toByte(),
                (cal.get(Calendar.MONTH) + 1).toByte(),
                cal.get(Calendar.DAY_OF_MONTH).toByte(),
                cal.get(Calendar.DAY_OF_WEEK).toByte(), // Sunday == 1
                cal.get(Calendar.HOUR_OF_DAY).toByte(),
                cal.get(Calendar.MINUTE).toByte(),
                cal.get(Calendar.SECOND).toByte()
        )
        characteristic.value = payload
        gattCallback.pushWrite(characteristic)
        Log.d(TAG, "Writing date characteristic to value ${payload.joinToString()}")
    }

    private fun writeNotificationCharacteristic() {
        val characteristic = gattCallback.getCharacteristic(WATCHYOS_NOTIFICATIONS_CHARACTERISTIC_UUID) ?: return

        characteristic.writeType = BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
        characteristic.value = byteArrayOf(notificationBits)

        gattCallback.pushWrite(characteristic)
    }

    private fun writeWatchyState(state: Byte, stateID: Byte) {
        val characteristic = gattCallback.getCharacteristic(WATCHYOS_STATE_CHARACTERISTIC_UUID) ?: return

        characteristic.writeType = BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
        characteristic.value = byteArrayOf(state, stateID)

        gattCallback.pushWrite(characteristic) {watchyStateID = stateID}
    }

    private fun fullSync(state: Byte = WatchyBLEState.REBOOT.value) {
        writeNotificationCharacteristic()
        writeTimeCharacteristic()
        writeWatchyState(WatchyBLEState.DISCONNECT.value, phoneStateID)
    }

    private fun stateRead(characteristic: BluetoothGattCharacteristic) {
        val value = characteristic.value
        if(value.size != 2) {
            Log.e(TAG, "WatchyOS State characteristic has unsupported value size: ${value.size}")
            return
        }
        val state = value[0]
        val stateID = value[1]
        when(state) {
            WatchyBLEState.REBOOT.value -> {
                fullSync()
            }
            WatchyBLEState.FAST_UPDATE.value -> {
                if(stateID != watchyStateID) {
                    // Watchy has a different state then it should have
                    Log.w(TAG, "Watchy state $stateID is not the expected $watchyStateID")
                    fullSync(state)
                }
                else if(watchyStateID != phoneStateID) {
                    writeNotificationCharacteristic()
                }
                // this will cause Watchy to initiate a disconnect, which is much faster then Android disconnecting
                writeWatchyState(WatchyBLEState.DISCONNECT.value, phoneStateID)
            }
            else -> {
                Log.w(TAG, "WatchyOS State characteristic has unsupported value: ${value.first()}")
                return
            }
        }
    }

    fun onCharacteristicRead(characteristic: BluetoothGattCharacteristic) {
        when(characteristic.uuid) {
            WATCHYOS_STATE_CHARACTERISTIC_UUID -> stateRead(characteristic)
            else -> return
        }
    }

    private fun createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val serviceChannel = NotificationChannel(
                    CHANNEL_ID,
                    getString(R.string.conn_not_title),
                    NotificationManager.IMPORTANCE_LOW
            )
            serviceChannel.enableLights(false)
            serviceChannel.enableVibration(false)
            val manager = getSystemService(NotificationManager::class.java)
            manager.createNotificationChannel(serviceChannel)
        }
    }
}