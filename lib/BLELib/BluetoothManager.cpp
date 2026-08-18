#include "BluetoothManager.h"

// Standard Nordic UART Service UUIDs
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

/**
 * @brief BLE server callbacks to handle connection and disconnection events.
 */
class MyServerCallbacks : public NimBLEServerCallbacks {
    BluetoothManager* _mgr;
public:
    MyServerCallbacks(BluetoothManager* mgr) : _mgr(mgr) {}

    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        Serial.printf("[BLE]: Device connected! Addr: %s\n", connInfo.getAddress().toString().c_str());
        _mgr->_connected = true;
        _mgr->_justConnected = true;
        _mgr->_statusChanged = true;
        _mgr->_clientSubscribed = false;
        _mgr->_justSubscribed = false;
        _mgr->_connectTimestampMs = millis();
        _mgr->_lastRxActivityMs = millis();
        _mgr->_lastRssiPollMs = millis();

        // Request high-speed BLE connection parameters (7.5ms - 15ms interval, 0 latency, 2s timeout)
        pServer->updateConnParams(connInfo.getConnHandle(), 6, 12, 0, 200);
    }

    void onMTUChange(uint16_t MTU, NimBLEConnInfo& connInfo) override {
        Serial.printf("[BLE]: MTU updated to %u bytes (ConnID: %u)\n", MTU, connInfo.getConnHandle());
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
        Serial.printf("[BLE]: Device disconnected. Reason: %d\n", reason);
        _mgr->_connected = false;
        _mgr->_clientSubscribed = false;
        _mgr->_justSubscribed = false;
        _mgr->_statusChanged = true;
    }
};

/**
 * @brief BLE characteristic callbacks to handle TX subscription events.
 */
class MyTxCharacteristicCallbacks : public NimBLECharacteristicCallbacks {
    BluetoothManager* _mgr;
public:
    MyTxCharacteristicCallbacks(BluetoothManager* mgr) : _mgr(mgr) {}

    void onSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, uint16_t subValue) override {
        Serial.printf("[BLE]: Client %s TX notifications (subValue: %u)\n",
                      (subValue > 0) ? "subscribed to" : "unsubscribed from", subValue);
        bool wasSubscribed = _mgr->_clientSubscribed;
        _mgr->_clientSubscribed = (subValue > 0);
        // Set justSubscribed flag on first subscription (triggers config_sync send)
        if (!wasSubscribed && _mgr->_clientSubscribed) {
            _mgr->_justSubscribed = true;
        }
    }
};

/**
 * @brief BLE characteristic callbacks to handle data written by the central device.
 */
class MyCharacteristicCallbacks : public NimBLECharacteristicCallbacks {
    BluetoothManager* _mgr;
public:
    MyCharacteristicCallbacks(BluetoothManager* mgr) : _mgr(mgr) {}

    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            std::lock_guard<std::mutex> lock(_mgr->_rxMutex);
            _mgr->_rxBuffer.push(value);
            _mgr->_rxActivityFlag = true;
            _mgr->_lastRxActivityMs = millis();
        }
    }
};

BluetoothManager::BluetoothManager() {}

void BluetoothManager::begin(const std::string& deviceName) {
    NimBLEDevice::init(deviceName);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);

    _pServer = NimBLEDevice::createServer();
    _pServer->setCallbacks(new MyServerCallbacks(this));

    NimBLEService* pService = _pServer->createService(SERVICE_UUID);

    _pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ
    );
    _pTxCharacteristic->setCallbacks(new MyTxCharacteristicCallbacks(this));

    NimBLECharacteristic* pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    pRxCharacteristic->setCallbacks(new MyCharacteristicCallbacks(this));
}

void BluetoothManager::startAdvertising(const std::string& deviceName) {
    if (_pServer) {
        _pServer->start();
    }

    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();

    NimBLEAdvertisementData advData;
    advData.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);
    advData.setName(deviceName);

    NimBLEAdvertisementData scanResponseData;
    scanResponseData.addServiceUUID(NimBLEUUID(SERVICE_UUID));
    scanResponseData.addServiceUUID(NimBLEUUID("00008018-0000-1000-8000-00805f9b34fb")); // BLEOTA Service

    pAdvertising->setAdvertisementData(advData);
    pAdvertising->setScanResponseData(scanResponseData);
    pAdvertising->setMinInterval(32);
    pAdvertising->setMaxInterval(64);
    pAdvertising->start();
    _advertising = true;
    Serial.println("[BLE]: Server started and advertising (NUS + BLEOTA services registered).");
}

void BluetoothManager::update() {
    if (!_connected && _oldConnected) {
        NimBLEDevice::getAdvertising()->start();
        _advertising = true;
        _oldConnected = _connected;
        _rssi = -99;
        _rssiInitialized = false;
        _minRssiSeen = 0;
        _maxRssiSeen = -127;
    }

    if (_connected && !_oldConnected) {
        _advertising = false;
        _oldConnected = _connected;
        _rssiInitialized = false;
        _minRssiSeen = 0;
        _maxRssiSeen = -127;
    }

    if (_connected) {
        uint32_t nowMs = millis();
        if (!_clientSubscribed && (nowMs - _connectTimestampMs > 30000)) {
            Serial.println("[BLE Watchdog]: Client never subscribed to TX notifications. Dropping zombie connection...");
            disconnectClient();
            return;
        }
        if (_clientSubscribed && (nowMs - _lastRxActivityMs > 30000)) {
            Serial.println("[BLE Watchdog]: No heartbeat ping received for 30 seconds. Dropping inactive connection...");
            disconnectClient();
            return;
        }
    }

    uint32_t now = millis();
    if (_connected && _pServer && (now - _connectTimestampMs >= 4000) && (now - _lastRssiPollMs >= 1000)) {
        _lastRssiPollMs = now;
        auto peers = _pServer->getPeerDevices();
        if (!peers.empty()) {
            uint16_t handle = peers[0];
            int8_t rawRssi = 0;
            if (ble_gap_conn_rssi(handle, &rawRssi) == 0) {
                bool boundsChanged = false;
                if (!_rssiInitialized || rawRssi < _minRssiSeen) { _minRssiSeen = rawRssi; boundsChanged = true; }
                if (!_rssiInitialized || rawRssi > _maxRssiSeen) { _maxRssiSeen = rawRssi; boundsChanged = true; }
                if (boundsChanged) {
                    Serial.printf("[BLE RSSI Bounds] New Min: %d dBm | New Max: %d dBm (Current Raw: %d dBm)\n",
                                  _minRssiSeen, _maxRssiSeen, rawRssi);
                }
                if (!_rssiInitialized) {
                    _rssi = rawRssi;
                    _rssiInitialized = true;
                } else {
                    _rssi = (int16_t)((_rssi * 6 + (int16_t)rawRssi * 4) / 10);
                }
            }
        }
    }
}

bool BluetoothManager::available() {
    std::lock_guard<std::mutex> lock(_rxMutex);
    return !_rxBuffer.empty();
}

std::string BluetoothManager::readString() {
    std::lock_guard<std::mutex> lock(_rxMutex);
    if (_rxBuffer.empty()) return "";
    std::string val = _rxBuffer.front();
    _rxBuffer.pop();
    return val;
}

void BluetoothManager::write(const std::string& data) {
    if (_connected && _pTxCharacteristic != nullptr) {
        _pTxCharacteristic->setValue(data);
        _pTxCharacteristic->notify();
        _txActivityFlag = true;
    }
}

bool BluetoothManager::hasRxActivity() {
    bool val = _rxActivityFlag;
    _rxActivityFlag = false;
    return val;
}

bool BluetoothManager::hasTxActivity() {
    bool val = _txActivityFlag;
    _txActivityFlag = false;
    return val;
}

bool BluetoothManager::hasConnectionChanged() {
    bool val = _statusChanged;
    _statusChanged = false;
    return val;
}

void BluetoothManager::sendStatus(bool pwmEnabled, uint16_t highTimeUs, uint32_t freq,
                                   uint16_t minUs, uint16_t centerUs, uint16_t maxUs,
                                   uint8_t pin, int16_t rssi) {
    char buf[192];
    snprintf(buf, sizeof(buf),
             "{\"type\":\"status\",\"pwm_en\":%s,\"high_us\":%u,\"freq\":%u,\"min_us\":%u,\"center_us\":%u,\"max_us\":%u,\"pin\":%u,\"rssi\":%d}",
             pwmEnabled ? "true" : "false", highTimeUs, (unsigned int)freq,
             minUs, centerUs, maxUs, (unsigned int)pin, rssi);
    write(std::string(buf));
}

void BluetoothManager::sendConfigSync(bool pwmEnabled, uint16_t currentUs,
                                       uint16_t minUs, uint16_t centerUs, uint16_t maxUs,
                                       uint32_t freqHz, uint8_t gpioPin) {
    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"type\":\"config_sync\",\"pwm_en\":%s,\"high_us\":%u,"
             "\"min_us\":%u,\"center_us\":%u,\"max_us\":%u,"
             "\"freq\":%u,\"pin\":%u}",
             pwmEnabled ? "true" : "false",
             currentUs, minUs, centerUs, maxUs,
             (unsigned int)freqHz, (unsigned int)gpioPin);
    write(std::string(buf));
    Serial.println("[BLE]: config_sync sent to client.");
}

void BluetoothManager::disconnectClient() {
    if (_connected && _pServer != nullptr) {
        auto peerDevices = _pServer->getPeerDevices();
        if (!peerDevices.empty()) {
            Serial.println("[BLE]: Disconnecting peer client due to watchdog / reset...");
            _pServer->disconnect(peerDevices[0]);
        }
    }
}
