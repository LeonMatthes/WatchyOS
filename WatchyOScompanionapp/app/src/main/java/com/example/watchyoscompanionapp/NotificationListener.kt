package com.example.watchyoscompanionapp

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.IBinder
import android.service.notification.NotificationListenerService
import android.service.notification.StatusBarNotification
import android.util.Log

class NotificationListener : NotificationListenerService() {
    private val WHATSAPP_PACK_NAME = "com.whatsapp"

    private var bound = false

    override fun onListenerConnected() {
        super.onListenerConnected()
        bound = true
        Log.d("Watchy", "Listener connected")
        val listenerService = this

        val receiver = object : BroadcastReceiver() {
            override fun onReceive(context: Context?, intent: Intent) {
                Log.d("Watchy", "Received intent")
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
        Log.d("Watchy", "Listener disconnected")
    }

    override fun onNotificationPosted(sbn: StatusBarNotification?) {
        super.onNotificationPosted(sbn)
        Log.d("Watchy", "Notification posted")
    }

    override fun onNotificationRemoved(sbn: StatusBarNotification?) {
        super.onNotificationRemoved(sbn)
        Log.d("Watchy", "Notification removed")
    }
}