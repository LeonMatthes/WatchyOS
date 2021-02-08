package com.example.watchyoscompanionapp

import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGattCallback
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.Build
import android.os.IBinder
import android.util.Log
import androidx.core.app.NotificationCompat

private val CHANNEL_ID = "ForegroundServiceChannel"

private const val TAG = "WatchyConnectionService"

class WatchyConnectionService : Service() {

    val SERVICE_ID = 1

    var gattCallback = WatchyGattCallback()

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
                .build()

        startForeground(SERVICE_ID, notification)

        val bleDevice: BluetoothDevice = intent.getParcelableExtra("WatchyBLEDevice")!!
        bleDevice.connectGatt(this, false, gattCallback)

        val receiver = object : BroadcastReceiver() {
            override fun onReceive(context: Context?, intent: Intent) {
                Log.d(TAG, "Received intent")

                if(intent.hasExtra("notifications")) {
                    gattCallback.notification_bits = intent.getByteExtra("notifications", 0x00)
                    gattCallback.notifications_changed = true
                    Log.d(TAG, "Notifications: ${gattCallback.notification_bits}")
                }
            }
        }
        val filter = IntentFilter()
        filter.addAction("com.example.watchyoscompanionapp.WATCHY_GATT_CALLBACK")
        registerReceiver(receiver, filter)

        return START_NOT_STICKY
    }

    override fun onDestroy() {
        super.onDestroy()

        gattCallback.gatt?.close()
    }

    override fun onBind(intent: Intent?): IBinder? {
        return null
    }

    private fun createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val serviceChannel = NotificationChannel(
                    CHANNEL_ID,
                    getString(R.string.conn_not_title),
                    NotificationManager.IMPORTANCE_LOW
            )
            val manager = getSystemService(NotificationManager::class.java)
            manager.createNotificationChannel(serviceChannel)
        }
    }
}