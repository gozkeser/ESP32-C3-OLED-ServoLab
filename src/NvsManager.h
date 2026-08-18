#ifndef NVS_MANAGER_H
#define NVS_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

/**
 * @file NvsManager.h
 * @brief Persistent settings manager using ESP32 NVS (Non-Volatile Storage).
 *
 * Wraps Preferences.h to store and retrieve PWM configuration parameters
 * across power cycles. Uses the "servolab" NVS namespace.
 */
class NvsManager {
public:
    NvsManager() = default;

    /**
     * @brief Loads all PWM settings from NVS.
     * @param minUs     Output: minimum pulse width in microseconds.
     * @param centerUs  Output: center/neutral pulse width in microseconds.
     * @param maxUs     Output: maximum pulse width in microseconds.
     * @param freqHz    Output: PWM refresh frequency in Hz.
     * @param gpioPin   Output: PWM output GPIO pin number.
     * @return true if saved settings were found and loaded; false if NVS is empty.
     */
    bool load(uint16_t& minUs, uint16_t& centerUs, uint16_t& maxUs,
              uint32_t& freqHz, uint8_t& gpioPin);

    /**
     * @brief Saves all PWM settings to NVS.
     * @param minUs     Minimum pulse width in microseconds.
     * @param centerUs  Center/neutral pulse width in microseconds.
     * @param maxUs     Maximum pulse width in microseconds.
     * @param freqHz    PWM refresh frequency in Hz.
     * @param gpioPin   PWM output GPIO pin number.
     */
    void save(uint16_t minUs, uint16_t centerUs, uint16_t maxUs,
              uint32_t freqHz, uint8_t gpioPin);

    /** @brief Clears all saved settings from NVS (factory reset). */
    void reset();

private:
    Preferences prefs_;
    static constexpr const char* kNamespace = "servolab";
};

#endif // NVS_MANAGER_H
