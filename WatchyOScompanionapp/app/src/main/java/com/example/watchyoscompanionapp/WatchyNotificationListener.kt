package com.example.watchyoscompanionapp

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.IBinder
import android.service.notification.NotificationListenerService
import android.service.notification.StatusBarNotification
import android.util.Log
import javax.net.ssl.SSLEngineResult

private const val TAG = "WatchyNotifiListener"

class WatchyNotificationListener : NotificationListenerService() {
    private val WHATSAPP_PACKAGE_NAME = "com.whatsapp"

    private var bound = false

    override fun onListenerConnected() {
        super.onListenerConnected()
        bound = true
        Log.d(TAG, "Listener connected - abc")

        val listenerService = this

        val receiver = object : BroadcastReceiver() {
            override fun onReceive(context: Context?, intent: Intent) {
                Log.d(TAG, "Received intent")
                if(intent.getStringExtra("command").equals("clearall")) {
                    listenerService.cancelAllNotifications()
                }
            }
        }
        val filter = IntentFilter()
        filter.addAction("com.example.watchyoscompanionapp.NOTIFICATION_LISTENER_SERVICE_EXAMPLE")
        registerReceiver(receiver, filter)
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
        Log.d(TAG, "Notification posted")

        if(sbn.packageName == WHATSAPP_PACKAGE_NAME) {
            sendNotificationUpdate()
        }
    }

    override fun onNotificationRemoved(sbn: StatusBarNotification) {
        super.onNotificationRemoved(sbn)
        Log.d(TAG, "Notification removed")

        if(sbn.packageName == WHATSAPP_PACKAGE_NAME) {
            sendNotificationUpdate()
        }
    }

    private fun sendNotificationUpdate() {
        val anyWhatsapp = activeNotifications.any { notification: StatusBarNotification -> notification.packageName == WHATSAPP_PACKAGE_NAME }

        val intent = Intent("com.example.watchyoscompanionapp.WATCHY_GATT_CALLBACK")
        val notificationBits : Byte = if (anyWhatsapp)  0x01 else 0x00
        intent.putExtra("notifications", notificationBits)
        sendBroadcast(intent)
    }
}