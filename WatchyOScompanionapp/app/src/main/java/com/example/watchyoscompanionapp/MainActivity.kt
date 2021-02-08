package com.example.watchyoscompanionapp

import android.app.Activity
import android.content.Context
import android.content.Intent
import androidx.appcompat.app.AppCompatActivity
import android.widget.Button
import android.widget.TextView
import android.Manifest
import android.bluetooth.*
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanFilter
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.bluetooth.le.ScanSettings.CALLBACK_TYPE_FIRST_MATCH
import android.bluetooth.le.ScanSettings.MATCH_MODE_STICKY
import android.content.ComponentName
import android.content.pm.PackageManager
import android.os.*
import android.provider.Settings
import android.provider.Settings.ACTION_NOTIFICATION_LISTENER_SETTINGS
import android.text.TextUtils
import android.util.Log
import androidx.annotation.RequiresApi
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import java.util.*

private const val ENABLE_BLUETOOTH_REQUEST_CODE = 1
private const val LOCATION_PERMISSION_REQUEST_CODE = 2
private const val ENABLED_NOTIFICATION_LISTENERS = "enabled_notification_listeners"



class MainActivity : AppCompatActivity() {
    private val bluetoothAdapter: BluetoothAdapter by lazy {
        val bluetoothManager = getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
        bluetoothManager.adapter
    }

    private val bleScanner by lazy {
        bluetoothAdapter.bluetoothLeScanner
    }

    private val testButton by lazy {
        findViewById<Button>(R.id.test_button)
    }

    private val textView by lazy {
        findViewById<TextView>(R.id.textView)
    }

    val isLocationPermissionGranted
        get() = hasPermission(Manifest.permission.ACCESS_FINE_LOCATION)

    fun Context.hasPermission(permissionType: String): Boolean {
        return ContextCompat.checkSelfPermission(this, permissionType) ==
                PackageManager.PERMISSION_GRANTED
    }

    // adapted from https://github.com/Chagall/notification-listener-service-example/blob/5d2affa204a23f3154b2c87a372e95d50101a2ab/app/src/main/java/com/github/chagall/notificationlistenerexample/MainActivity.java#L100
    private fun isNotificationServiceEnabled(): Boolean {
        val flat = Settings.Secure.getString(contentResolver, ENABLED_NOTIFICATION_LISTENERS)
        if(!TextUtils.isEmpty(flat)) {
            val names = flat.split(":")
            for(name in names) {
                val componentName = ComponentName.unflattenFromString(name)
                if(componentName != null && TextUtils.equals(packageName, componentName.packageName)) {
                    return true
                }
            }
        }
        return false
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        testButton.setOnClickListener { test() }
        if(!isNotificationServiceEnabled()) {
            Log.d("Watchy", "Asking for Notification Listener approval")
            startActivity(Intent(ACTION_NOTIFICATION_LISTENER_SETTINGS))
        }
    }

    private var isScanning = false
        set(value) {
            field = value
            runOnUiThread { testButton.text = if (value) "Stop scan" else "Start scan" }
        }


    private fun test() {
        if(!isLocationPermissionGranted) {
            textView.text = "Location permission is needed on Android 6.0+ to use BLE!\nPlease grant this app location permissions.\nThis app won't use your location data!"
            return
        }

        /*val intent = Intent("com.example.watchyoscompanionapp.NOTIFICATION_LISTENER_SERVICE_EXAMPLE")
        intent.putExtra("command", "clearall")
        sendBroadcast(intent)
        textView.text = "Sent notification broadcast"*/


        if(isScanning) {
            stopBLEScan()
        }
        else {
            // stop any existing connection
            stopConnectionService()
            startBLEScan()
        }
    }

    private fun stopBLEScan() {
        bleScanner.stopScan(scanCallback)
        isScanning = false
    }

    private fun startBLEScan() {
        textView.text = "Scanning for BLE devices"

        val filter = ScanFilter.Builder()
            .setServiceUuid(ParcelUuid.fromString(WATCHYOS_SERVICE_UUID))
            .build()

        val scanSettingsBuilder = ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_LOW_POWER)
            .setReportDelay(0)
        if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            // only available in newer versions of Android
            scanSettingsBuilder
                .setCallbackType(CALLBACK_TYPE_FIRST_MATCH)
                .setMatchMode(MATCH_MODE_STICKY)
        }

        val scanSettings = scanSettingsBuilder.build()

        bleScanner.startScan(listOf(filter), scanSettings, scanCallback)
        isScanning = true
    }

    val context = this

    private val scanCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            runOnUiThread { textView.text = "Found BLE device!\nName: ${result.device.name ?: "Unnamed"}, address: ${result.device.address}" }

            if(isScanning) {
                stopBLEScan()
            }

            startConnectionService(result.device)
        }
    }

    private fun startConnectionService(bleDevice: BluetoothDevice) {
        val serviceIntent = Intent(this, WatchyConnectionService::class.java)
        serviceIntent.putExtra("WatchyBLEDevice", bleDevice)

        ContextCompat.startForegroundService(this, serviceIntent)
    }

    private fun stopConnectionService() {
        val serviceIntent = Intent(this, WatchyConnectionService::class.java)
        stopService(serviceIntent)
    }

    override fun onResume() {
        super.onResume()
        promptEnableBluetooth()
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && !isLocationPermissionGranted) {
            requestLocationPermission()
        }
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        when (requestCode) {
            ENABLE_BLUETOOTH_REQUEST_CODE -> {
                if (resultCode != Activity.RESULT_OK) {
                    promptEnableBluetooth()
                }
            }
        }
    }

    private fun promptEnableBluetooth() {
        if(!bluetoothAdapter.isEnabled) {
            val enableBtIntent = Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE)
            startActivityForResult(enableBtIntent, ENABLE_BLUETOOTH_REQUEST_CODE)
        }
    }

    private fun requestLocationPermission() {
        if (isLocationPermissionGranted) {
            return
        }
        runOnUiThread {
            requestPermission(
                Manifest.permission.ACCESS_FINE_LOCATION,
                LOCATION_PERMISSION_REQUEST_CODE
            )
        }
    }
    private fun Activity.requestPermission(permission: String, requestCode: Int) {
        ActivityCompat.requestPermissions(this, arrayOf(permission), requestCode)
    }
}