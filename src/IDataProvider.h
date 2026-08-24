#ifndef I_DATA_PROVIDER_H
#define I_DATA_PROVIDER_H

#include <Arduino.h>

/**
 * @file IDataProvider.h
 * @brief Abstract data provider interface for system state queries.
 *
 * Provides a decoupled interface for UI components to query live system
 * metrics (e.g. BLE connectivity, RSSI signal strength, PWM/servo state).
 */
#define FIRMWARE_VERSION "v0.9.2"

class IDataProvider
{
public:
    virtual ~IDataProvider() = default;

    /** @brief Returns installed firmware version string (e.g. "v0.9.0"). */
    virtual const char* getFirmwareVersion() const = 0;

    /** @brief Returns true if BLE device is connected. */
    virtual bool isBleConnected() const = 0;

    /** @brief Returns current BLE RSSI signal level in dBm. */
    virtual int16_t getRssi() const = 0;

    /** @brief Returns true if PWM output is enabled. */
    virtual bool isPwmEnabled() const = 0;

    /** @brief Returns PWM frequency in Hz (e.g. 50Hz). */
    virtual uint32_t getPwmFrequency() const = 0;

    /** @brief Returns PWM high-time pulse width in microseconds (e.g. 1500us). */
    virtual uint16_t getPwmHighTime() const = 0;

    /** @brief Returns minimum supported PWM high-time in microseconds (e.g. 500us). */
    virtual uint16_t getPwmMinHighTime() const = 0;

    /** @brief Returns maximum supported PWM high-time in microseconds (e.g. 2500us). */
    virtual uint16_t getPwmMaxHighTime() const = 0;

    /** @brief Returns configured center PWM high-time in microseconds (e.g. 1500us). */
    virtual uint16_t getPwmCenterHighTime() const = 0;

    /** @brief Returns active PWM output GPIO pin (e.g. GPIO 0). */
    virtual uint8_t getPwmGpioPin() const = 0;
};

#endif // I_DATA_PROVIDER_H
