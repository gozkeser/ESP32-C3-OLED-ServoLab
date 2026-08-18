#ifndef BLE_OTA_HANDLER_H
#define BLE_OTA_HANDLER_H

#include <Arduino.h>

// We directly use NimBLEOTA.h (NimBLE backend of BLEOTA library).
// This avoids the ESP_ARDUINO_VERSION dispatch in BLEOTA.h which on
// core >= 3.3.0 would redirect to BLEDevice.h — incompatible with
// our NimBLE-Arduino setup.
//
// Force the macro so NimBLEOTA.h actually defines NimBLEOTAClass.
#define BLEOTA_USE_NIMBLE
#include <NimBLEDevice.h>
#include <NimBLEOTA.h>

/**
 * @file BleOtaHandler.h
 * @brief Integrates the BLEOTA library (gb88/BLEOTA v1.0.6, NimBLE path)
 *        with the existing NimBLE server.
 *
 * Registers a NimBLEOTAClass instance on the NimBLE server created by
 * BluetoothManager. BLEOTA adds its own GATT service (firmware receive +
 * command + device info service) to the existing server without disturbing
 * the Nordic UART Service.
 */
class BleOtaHandler {
public:
    BleOtaHandler() = default;

    /**
     * @brief Registers the BLEOTA GATT service on the existing NimBLE server.
     * Must be called AFTER NimBLEDevice::init() and server creation,
     * but BEFORE NimBLEServer::start().
     * @param pServer Pointer to the active NimBLE server.
     */
    void begin(NimBLEServer* pServer);

    /**
     * @brief Periodically processes BLEOTA background tasks and handles auto-reboot when finished.
     * Should be called in the main loop.
     */
    void update();

    /** @brief Returns true if BLEOTA service was successfully registered. */
    bool isRegistered() const { return registered_; }

private:
    NimBLEOTAClass bleota_;    // NimBLE OTA service instance
    bool registered_ = false;
};

#endif // BLE_OTA_HANDLER_H
