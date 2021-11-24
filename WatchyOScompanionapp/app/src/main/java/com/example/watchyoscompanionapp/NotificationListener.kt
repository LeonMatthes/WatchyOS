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
    private val packageFilter: Set<String> = hashSetOf("com.whatsapp", "com.samsung.android.email.provider")

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

    private fun isRelevant(sbn: StatusBarNotification): Boolean {
        val isGroupSummary = sbn.notification.flags and android.app.Notification.FLAG_GROUP_SUMMARY > 0
        return packageFilter.contains(sbn.packageName) && !isGroupSummary
    }

    override fun onNotificationPosted(sbn: StatusBarNotification) {
        super.onNotificationPosted(sbn)
        if(!bound) {
            return
        }

        Log.d(TAG, "Notification posted: ${sbn.packageName}")
        if(isRelevant(sbn)) {
            WatchyConnectionService.instance?.notificationPosted(sbn)
        }
    }

    override fun onNotificationRemoved(sbn: StatusBarNotification) {
        super.onNotificationRemoved(sbn)
        if(!bound) {
            return
        }

        Log.d(TAG, "Notification removed: ${sbn.packageName}")

        if(isRelevant(sbn)) {
            WatchyConnectionService.instance?.notificationRemoved(sbn)
        }
    }

    private fun sendNotificationUpdate() {
        for(sbn in activeNotifications) {
            if(isRelevant(sbn)) {
                WatchyConnectionService.instance?.notificationPosted(sbn)
            }
        }
    }
}