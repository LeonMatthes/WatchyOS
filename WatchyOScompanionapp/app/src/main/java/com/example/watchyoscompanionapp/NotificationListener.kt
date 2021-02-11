package com.example.watchyoscompanionapp

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.Build
import android.os.IBinder
import android.service.notification.NotificationListenerService
import android.service.notification.StatusBarNotification
import android.util.Log
import javax.net.ssl.SSLEngineResult

private const val TAG = "WatchyNotifiListener"

class NotificationListener : NotificationListenerService() {
    private val WHATSAPP_PACKAGE_NAME = "com.whatsapp"

    private var bound = false

    override fun onListenerConnected() {
        super.onListenerConnected()
        bound = true
        Log.d(TAG, "Listener connected")

        val listenerService = this

        val receiver = object : BroadcastReceiver() {
            override fun onReceive(context: Context?, intent: Intent) {
                Log.d(TAG, "Received intent")
                when (intent.getStringExtra(BuildConfig.APPLICATION_ID + ".Command")) {
                    "clearall" -> {
                        cancelAllNotifications()
                    }
                    "sendUpdate" -> {
                        sendNotificationUpdate()
                    }
                }
            }
        }
        val filter = IntentFilter()
        filter.addAction("com.example.watchyoscompanionapp.WATCHY_NOTIFICATION_LISTENER")
        registerReceiver(receiver, filter)

        sendNotificationUpdate()
    }

    override fun onListenerDisconnected() {
        super.onListenerDisconnected()
        bound = false
        Log.d(TAG, "Listener disconnected")
    }

    override fun onNotificationPosted(sbn: StatusBarNotification) {
        super.onNotificationPosted(sbn)
        if(!bound) {
            return
        }

        Log.d(TAG, "Notification posted: ${sbn.packageName}")

        if(sbn.packageName == WHATSAPP_PACKAGE_NAME) {
            sendNotificationUpdate()
        }
    }

    override fun onNotificationRemoved(sbn: StatusBarNotification) {
        super.onNotificationRemoved(sbn)
        if(!bound) {
            return
        }

        Log.d(TAG, "Notification removed: ${sbn.packageName}")

        if(sbn.packageName == WHATSAPP_PACKAGE_NAME) {
            sendNotificationUpdate()
        }
    }

    private fun sendNotificationUpdate() {
        val anyWhatsapp = activeNotifications.any { notification: StatusBarNotification -> notification.packageName == WHATSAPP_PACKAGE_NAME }

        val intent = Intent(BuildConfig.APPLICATION_ID + ".WATCHY_GATT_CALLBACK")
        val notificationBits : Byte = if (anyWhatsapp)  {
            0x01
        } else {
            0x00
        }
        intent.putExtra(BuildConfig.APPLICATION_ID+ ".NotificationBits", notificationBits)
        sendBroadcast(intent)

        Log.d(TAG, "anyWhatsapp: $anyWhatsapp, notificationBits: $notificationBits")
    }
}