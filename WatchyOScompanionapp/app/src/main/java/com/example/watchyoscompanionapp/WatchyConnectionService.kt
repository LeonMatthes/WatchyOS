package com.example.watchyoscompanionapp

import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGattCharacteristic
import android.content.Intent
import android.os.Build
import android.os.IBinder
import android.service.notification.StatusBarNotification
import android.util.Log
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationCompat.PRIORITY_LOW
import java.util.*
import java.util.concurrent.locks.ReentrantLock
import kotlin.collections.ArrayList
import kotlin.concurrent.withLock

private const val CHANNEL_ID = "ForegroundServiceChannel"

private const val TAG = "WatchyConnectionService"

private const val SERVICE_ID = 1

class WatchyConnectionService : Service() {
    var gattCallback = WatchyGattCallback(this)

    private val watchyNotifications = ArrayList<WatchyNotification>()
    private val phoneNotifications = ArrayList<WatchyNotification>()
    private val notificationsLock = ReentrantLock()

    private var watchyStateID : Byte = 0x00
    private var phoneStateID : Byte = 0x00

    companion object {
        var instance: WatchyConnectionService? = null
    }

    override fun onCreate() {
        super.onCreate()
        instance = this
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

        val intent = Intent(BuildConfig.APPLICATION_ID + ".WATCHY_NOTIFICATION_LISTENER")
        // request update from NotificationListener
        intent.putExtra(BuildConfig.APPLICATION_ID+ ".Command", "sendUpdate")
        sendBroadcast(intent)

        return START_NOT_STICKY
    }

    override fun onDestroy() {
        super.onDestroy()

        gattCallback.stop()
    }

    override fun onBind(intent: Intent?): IBinder? {
        return null
    }


    private fun newNotificationId(): Byte? {
        var id = -1
        do {
            id += 1
            val found = phoneNotifications.find { notification -> notification.id == id.toByte() } != null
                    || watchyNotifications.find { notification -> notification.id == id.toByte() } != null
        } while(found && id < 256)

        // all id's already taken
        return if(id > 255) {
            null
        }
        else {
            id.toByte()
        }
    }

    fun notificationPosted(sbn: StatusBarNotification) {
        notificationsLock.withLock {
            Log.d(TAG, sbn.toString())
            Log.d(TAG, "Is summary: ${sbn.notification.flags and android.app.Notification.FLAG_GROUP_SUMMARY}")
            val watchyNotification = WatchyNotification(0x00, sbn)
            Log.d(TAG, "Title: ${watchyNotification.title()}, Text: ${watchyNotification.text()}")

            val previous = phoneNotifications.find { notification: WatchyNotification -> notification.sbn.key == sbn.key }
            if(previous != null) {
                previous.sbn = sbn
            }
            else {
                val newId = newNotificationId() ?: return
                phoneNotifications.add(WatchyNotification(newId, sbn))
            }

            increasePhoneStateId()
            Log.d(TAG, "Notification size: ${phoneNotifications.size}")
        }
    }

    private fun increasePhoneStateId() {
        if(phoneStateID == Byte.MAX_VALUE) {
            // make sure the phoneStateID never includes 0.
            // 0 is reserved for an uninitialized Watchy
            phoneStateID = (Byte.MIN_VALUE + 1).toByte()
        }
        else {
            phoneStateID++
        }
    }

    fun notificationRemoved(sbn: StatusBarNotification) {
        Log.i(TAG, "WhatsApp Notification removed")
        notificationsLock.withLock {
            phoneNotifications.removeAll { notification -> notification.sbn.key == sbn.key }

            increasePhoneStateId()
        }
    }


    private fun writeTimeCharacteristic() {
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
        gattCallback.pushCommand(WriteGattCommand(WATCHYOS_TIME_CHARACTERISTIC_UUID, BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT, payload))
        Log.d(TAG, "Writing date characteristic to value ${payload.joinToString()}")
    }

    private fun clearWatchyNotifications() {
        writeNotificationCharacteristic(byteArrayOf(WatchyNotificationCommands.REMOVE_ALL.value))

        notificationsLock.withLock {
            watchyNotifications.clear()
        }
    }

    private fun writeNotificationCharacteristic(bytes: ByteArray) {
        gattCallback.pushCommand(
                WriteGattCommand(
                        WATCHYOS_NOTIFICATIONS_CHARACTERISTIC_UUID,
                        BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT,
                        bytes))
    }

    private fun writeNotifications() {
        notificationsLock.withLock {
            for(watchyNotification in watchyNotifications) {
                if(phoneNotifications.find { notification -> notification.id == watchyNotification.id } == null) {
                    writeNotificationCharacteristic(watchyNotification.removalPayload())
                }
            }

            for(phoneNotification in phoneNotifications) {
                if(!watchyNotifications.contains(phoneNotification)) {
                    writeNotificationCharacteristic(phoneNotification.creationPayload())
                }
            }

            watchyNotifications.clear()
            watchyNotifications.addAll(phoneNotifications)
        }

    }

    private fun writeWatchyState(state: Byte, stateID: Byte) {
        val characteristic = gattCallback.getCharacteristic(WATCHYOS_STATE_CHARACTERISTIC_UUID) ?: return

        characteristic.writeType = BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
        characteristic.value = byteArrayOf(state, stateID)

        gattCallback.pushCommand(
                WriteGattCommand(
                        WATCHYOS_STATE_CHARACTERISTIC_UUID,
                        BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE,
                        byteArrayOf(state, stateID))
                        { notificationsLock.withLock { watchyStateID = stateID } })
    }

    private fun fullSync(state: Byte = WatchyBLEState.REBOOT.value) {
        clearWatchyNotifications()
        writeNotifications()
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
                    writeNotifications()
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