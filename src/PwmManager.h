#ifndef PWM_MANAGER_H
#define PWM_MANAGER_H

#include <Arduino.h>
#include "IDataProvider.h"

/**
 * @file PwmManager.h
 * @brief Hardware PWM Generator using ESP32 LEDC peripheral.
 *
 * Configures LEDC hardware timer/channel on user-specified GPIO pins,
 * converts high-time pulse width (microseconds) to 14-bit duty cycle counts,
 * and updates hardware output in real-time.
 */
class PwmManager {
public:
    PwmManager() = default;

    /**
     * @brief Initializes the ESP32 LEDC hardware PWM generator.
     * @param pin Output GPIO pin (e.g. GPIO 0).
     * @param freqHz PWM Frequency in Hz (e.g. 50Hz).
     * @param channel LEDC channel (default 0).
     */
    void begin(uint8_t pin = 0, uint32_t freqHz = 50, uint8_t channel = 0);

    /**
     * @brief Dynamic state synchronizer driven by system task loop.
     * @param dataProvider Live system state provider.
     */
    void update(const IDataProvider& dataProvider);

    /** @brief Sets output GPIO pin. */
    void setPin(uint8_t pin);

    /** @brief Sets PWM frequency in Hz. */
    void setFrequency(uint32_t freqHz);

    /** @brief Sets pulse high-time in microseconds. */
    void setPulseWidthUs(uint16_t highTimeUs);

    /** @brief Enables or disables hardware PWM output. */
    void setEnabled(bool enable);

private:
    uint8_t channel_ = 0;
    uint8_t pin_ = 0;
    uint32_t frequency_ = 50;
    uint16_t highTimeUs_ = 1500;
    bool enabled_ = false;
    bool initialized_ = false;

    const uint8_t resolutionBits_ = 14; // 14-bit resolution (0..16383)
    const uint32_t maxDutyCount_ = 16383;

    void applyHardwareSettings();
    uint32_t calculateDutyCount(uint16_t highTimeUs, uint32_t freqHz) const;
};

#endif // PWM_MANAGER_H
