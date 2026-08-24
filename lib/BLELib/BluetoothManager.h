#ifndef BLUETOOTH_MANAGER_H
#define BLUETOOTH_MANAGER_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <string>
#include <queue>
#include <mutex>

/**
 * @brief BLE UART (Nordic UART Service) Implementation for ESP32-C3.
 * Provides a serial-like interface over Bluetooth Low Energy.
 */
class BluetoothManager {
public:
    BluetoothManager();
    
    /**
     * @brief Initializes the BLE stack, GATT server, and Nordic UART Service.
     * @param deviceName Name to advertise.
     */
    void begin(const std::string& deviceName);

    /**
     * @brief Starts the GATT server and BLE advertising with all registered services.
     * Call this after all GATT services (NUS, BLEOTA) have been added to the server.
     * @param deviceName Name to advertise.
     */
    void startAdvertising(const std::string& deviceName);

    /** @brief Handles state maintenance and reconnection. */
    void update();

    /** @brief Returns true if data is available to read. */
    bool available();

    /** @brief Reads a string from the receive buffer. */
    std::string readString();

    /** @brief Sends a string to the connected device. */
    void write(const std::string& data);

    /** @brief Returns true if a device is currently connected. */
    bool isConnected() const { return _connected; }

    /** @brief Returns the current filtered RSSI signal level in dBm. */
    int16_t getRssi() const { return _rssi; }

    /** @brief Returns minimum RSSI recorded during active connection. */
    int16_t getMinRssiSeen() const { return _minRssiSeen; }

    /** @brief Returns maximum RSSI recorded during active connection. */
    int16_t getMaxRssiSeen() const { return _maxRssiSeen; }

    /** @brief Returns true if the device is currently advertising. */
    bool isAdvertising() const { return _advertising; }

    /** @brief Resets the BLE inactivity watchdog timer timestamp. */
    void touchRxActivity() { _lastRxActivityMs = millis(); }

    /** @brief Checks for RX activity and clears the flag. */
    bool hasRxActivity();

    /** @brief Checks for TX activity and clears the flag. */
    bool hasTxActivity();

    /** @brief Checks if connection status changed and clears the flag. */
    bool hasConnectionChanged();

    /** @brief Checks if device has just connected and clears the flag. */
    bool hasJustConnected() {
        bool val = _justConnected;
        _justConnected = false;
        return val;
    }

    /** @brief Returns true if connected client has subscribed to TX notifications. */
    bool isSubscribed() const { return _clientSubscribed; }

    /** @brief Returns true once after client subscribes (clears itself). Used to trigger config_sync. */
    bool hasJustSubscribed() {
        bool val = _justSubscribed;
        _justSubscribed = false;
        return val;
    }

    /** @brief Returns the NimBLE server pointer (used by BleOtaHandler). */
    NimBLEServer* getServer() const { return _pServer; }

    /**
     * @brief Formats and sends system status JSON packet to the connected client.
     */
    void sendStatus(bool pwmEnabled, uint16_t highTimeUs, uint32_t freq,
                    uint16_t minUs, uint16_t centerUs, uint16_t maxUs,
                    uint8_t pin, int16_t rssi, const char* fwVer = "");

    /**
     * @brief Sends a full config_sync packet to the connected client on first subscription.
     * The HTML client uses this to populate the settings form with NVS-backed values.
     */
    void sendConfigSync(bool pwmEnabled, uint16_t currentUs, uint16_t minUs,
                        uint16_t centerUs, uint16_t maxUs, uint32_t freqHz,
                        uint8_t gpioPin, const char* fwVer = "");

    /** @brief Disconnects the currently connected peer client and restarts advertising. */
    void disconnectClient();

private:
    NimBLEServer* _pServer = nullptr;
    NimBLECharacteristic* _pTxCharacteristic = nullptr;
    
    bool _connected = false;
    bool _oldConnected = false;
    bool _justConnected = false;
    bool _justSubscribed = false;   // true once after CCCD subscription
    bool _advertising = false;
    bool _statusChanged = false;
    
    int16_t _rssi = -99;
    int16_t _minRssiSeen = 0;
    int16_t _maxRssiSeen = -127;
    uint32_t _lastRssiPollMs = 0;
    bool _rssiInitialized = false;
    
    bool _rxActivityFlag = false;
    bool _txActivityFlag = false;

    bool _clientSubscribed = false;
    uint32_t _connectTimestampMs = 0;
    uint32_t _lastRxActivityMs = 0;
    std::queue<std::string> _rxBuffer;
    mutable std::mutex _rxMutex;

    // Internal Callback classes
    friend class MyServerCallbacks;
    friend class MyCharacteristicCallbacks;
    friend class MyTxCharacteristicCallbacks;
};

#endif // BLUETOOTH_MANAGER_H