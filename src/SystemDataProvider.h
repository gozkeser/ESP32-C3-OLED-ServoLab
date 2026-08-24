#ifndef SYSTEM_DATA_PROVIDER_H
#define SYSTEM_DATA_PROVIDER_H

#include <Arduino.h>
#include "IDataProvider.h"

/**
 * @file SystemDataProvider.h
 * @brief Concrete implementation of IDataProvider storing live system state.
 *
 * Provides getter implementations for UI queries and thread-safe setter
 * methods for system tasks (Bluetooth, PWM Controller, etc.) to update state.
 */
class SystemDataProvider : public IDataProvider
{
public:
    SystemDataProvider() = default;

    // ----------------------------------------------------------
    // IDataProvider Interface Implementations
    // ----------------------------------------------------------

    const char* getFirmwareVersion() const override { return FIRMWARE_VERSION; }
    bool isBleConnected() const override { return bleConnected_; }
    int16_t getRssi() const override { return rssi_; }
    bool isPwmEnabled() const override { return pwmEnabled_; }
    uint32_t getPwmFrequency() const override { return pwmFrequency_; }
    uint16_t getPwmHighTime() const override { return pwmHighTime_; }
    uint16_t getPwmMinHighTime() const override { return pwmMinHighTime_; }
    uint16_t getPwmMaxHighTime() const override { return pwmMaxHighTime_; }
    uint16_t getPwmCenterHighTime() const override { return pwmCenterHighTime_; }
    uint8_t getPwmGpioPin() const override { return pwmGpioPin_; }

    // ----------------------------------------------------------
    // State Mutators (Setters for system modules)
    // ----------------------------------------------------------

    void setBleConnected(bool connected) { bleConnected_ = connected; }
    void setRssi(int16_t rssi) { rssi_ = rssi; }
    void setPwmEnabled(bool enabled) { pwmEnabled_ = enabled; }
    void setPwmFrequency(uint32_t freq) { pwmFrequency_ = freq; }
    void setPwmHighTime(uint16_t highTimeUs) { pwmHighTime_ = highTimeUs; }
    void setPwmCenterHighTime(uint16_t centerUs) { pwmCenterHighTime_ = centerUs; }
    void setPwmGpioPin(uint8_t pin) { pwmGpioPin_ = pin; }

    void setPwmRange(uint16_t minUs, uint16_t maxUs)
    {
        pwmMinHighTime_ = minUs;
        pwmMaxHighTime_ = (maxUs > minUs) ? maxUs : minUs + 1;
    }

private:
    bool bleConnected_ = false;
    int16_t rssi_ = -99;

    bool pwmEnabled_ = false;
    uint32_t pwmFrequency_ = 50;       // 50 Hz default for servo PWM
    uint16_t pwmHighTime_ = 1500;      // 1500 us neutral pulse width
    uint16_t pwmMinHighTime_ = 500;    // 500 us min
    uint16_t pwmCenterHighTime_ = 1500; // 1500 us center position
    uint16_t pwmMaxHighTime_ = 2500;   // 2500 us max
    uint8_t pwmGpioPin_ = 0;           // Default GPIO 0
};

#endif // SYSTEM_DATA_PROVIDER_H
