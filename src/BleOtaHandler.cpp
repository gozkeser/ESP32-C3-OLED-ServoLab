#include "BleOtaHandler.h"

void BleOtaHandler::begin(NimBLEServer* pServer)
{
    if (!pServer) {
        Serial.println("[BleOTA]: ERROR - null NimBLE server pointer!");
        return;
    }
    // Register BLEOTA GATT service on the existing NimBLE server.
    // bleota_ is a BLEOTAClass (== NimBLEOTAClass via BLEOTA_USE_NIMBLE).
    // begin() adds the OTA service + Device Info Service to the server.
    // init() configures the internal flash writer for OTA operation.
    bleota_.begin(pServer);
    bleota_.init();
    registered_ = true;
    Serial.println("[BleOTA]: BLEOTA service registered and initialized.");
}

void BleOtaHandler::update()
{
    if (registered_) {
        bleota_.process(true); // process callbacks & restart ESP32 when update finishes
    }
}
