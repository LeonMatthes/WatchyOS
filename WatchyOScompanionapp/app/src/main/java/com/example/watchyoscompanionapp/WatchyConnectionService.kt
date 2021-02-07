package com.example.watchyoscompanionapp

import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGattCallback
import android.content.Intent
import android.os.Build
import android.os.IBinder
import androidx.core.app.NotificationCompat

private val CHANNEL_ID = "ForegroundServiceChannel"


class WatchyConnectionService : Service() {

    val SERVICE_ID = 1

    var gattCallback: WatchyGattCallback? = null

    override fun onCreate() {
        super.onCreate()
    }

    override fun onStartCommand(intent: Intent, flags: Int, startId: Int): Int {
        createNotificationChannel()
        val notificationIntent = Intent(this, MainActivity::class.java)
        val pendingIntent = PendingIntent.getActivity(this, 0, notificationIntent, 0)

        val notification = NotificationCompat.Builder(this, CHANNEL_ID)
                .setContentTitle("Watchy connection")
                .setContentText("WatchyOS companion app is continuously connecting to Watchy for updates.")
                .setSmallIcon(R.drawable.ic_launcher_foreground)
                .setContentIntent(pendingIntent)
                .build()

        startForeground(SERVICE_ID, notification)

        val bleDevice: BluetoothDevice = intent.getParcelableExtra("WatchyBLEDevice")!!
        gattCallback = WatchyGattCallback()
        bleDevice.connectGatt(this, false, gattCallback)

        return START_NOT_STICKY
    }

    override fun onDestroy() {
        super.onDestroy()

        gattCallback?.gatt?.close()
    }

    override fun onBind(intent: Intent?): IBinder? {
        return null
    }

    private fun createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val serviceChannel = NotificationChannel(
                    CHANNEL_ID,
                    "Foreground Service Channel",
                    NotificationManager.IMPORTANCE_DEFAULT
            )
            val manager = getSystemService(NotificationManager::class.java)
            manager.createNotificationChannel(serviceChannel)
        }
    }
}