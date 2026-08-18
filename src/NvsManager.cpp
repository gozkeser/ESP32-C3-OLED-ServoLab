#include "NvsManager.h"

bool NvsManager::load(uint16_t& minUs, uint16_t& centerUs, uint16_t& maxUs,
                      uint32_t& freqHz, uint8_t& gpioPin)
{
    prefs_.begin(kNamespace, true); // read-only
    bool hasData = prefs_.isKey("min_us");

    if (hasData) {
        minUs    = prefs_.getUShort("min_us",    500);
        centerUs = prefs_.getUShort("center_us", 1500);
        maxUs    = prefs_.getUShort("max_us",    2500);
        freqHz   = prefs_.getULong("freq_hz",   50);
        gpioPin  = prefs_.getUChar("gpio_pin",  0);
        Serial.printf("[NVS]: Loaded — min:%u center:%u max:%u freq:%lu pin:%u\n",
                      minUs, centerUs, maxUs, (unsigned long)freqHz, gpioPin);
    }

    prefs_.end();
    return hasData;
}

void NvsManager::save(uint16_t minUs, uint16_t centerUs, uint16_t maxUs,
                      uint32_t freqHz, uint8_t gpioPin)
{
    prefs_.begin(kNamespace, false); // read-write
    prefs_.putUShort("min_us",    minUs);
    prefs_.putUShort("center_us", centerUs);
    prefs_.putUShort("max_us",    maxUs);
    prefs_.putULong("freq_hz",   freqHz);
    prefs_.putUChar("gpio_pin",  gpioPin);
    prefs_.end();
    Serial.printf("[NVS]: Saved — min:%u center:%u max:%u freq:%lu pin:%u\n",
                  minUs, centerUs, maxUs, (unsigned long)freqHz, gpioPin);
}

void NvsManager::reset()
{
    prefs_.begin(kNamespace, false);
    prefs_.clear();
    prefs_.end();
    Serial.println("[NVS]: All settings cleared (factory reset).");
}
