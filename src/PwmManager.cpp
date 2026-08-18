#include "PwmManager.h"

void PwmManager::begin(uint8_t pin, uint32_t freqHz, uint8_t channel) {
    pin_ = pin;
    frequency_ = (freqHz > 0) ? freqHz : 50;
    channel_ = channel;

    applyHardwareSettings();
    initialized_ = true;
}

void PwmManager::applyHardwareSettings() {
    ledcSetup(channel_, frequency_, resolutionBits_);
    ledcAttachPin(pin_, channel_);

    if (enabled_) {
        uint32_t duty = calculateDutyCount(highTimeUs_, frequency_);
        ledcWrite(channel_, duty);
    } else {
        ledcWrite(channel_, 0);
    }
}

uint32_t PwmManager::calculateDutyCount(uint16_t highTimeUs, uint32_t freqHz) const {
    if (freqHz == 0) return 0;
    // Duty Ratio = (highTimeUs * freqHz) / 1,000,000
    // Duty Count = Duty Ratio * maxDutyCount_
    uint64_t product = (uint64_t)highTimeUs * (uint64_t)freqHz * (uint64_t)maxDutyCount_;
    uint32_t duty = (uint32_t)(product / 1000000ULL);
    if (duty > maxDutyCount_) duty = maxDutyCount_;
    return duty;
}

void PwmManager::update(const IDataProvider& dataProvider) {
    if (!initialized_) return;

    bool targetEnabled = dataProvider.isPwmEnabled();
    uint32_t targetFreq = dataProvider.getPwmFrequency();
    uint16_t targetHighUs = dataProvider.getPwmHighTime();
    uint8_t targetPin = dataProvider.getPwmGpioPin();

    bool configChanged = (targetPin != pin_) || (targetFreq != frequency_);

    if (configChanged) {
        if (targetPin != pin_) {
            ledcDetachPin(pin_); // Detach and release old GPIO pin from hardware LEDC
        }
        pin_ = targetPin;
        frequency_ = (targetFreq > 0) ? targetFreq : 50;
        applyHardwareSettings();
    }

    if (targetEnabled != enabled_ || targetHighUs != highTimeUs_ || configChanged) {
        enabled_ = targetEnabled;
        highTimeUs_ = targetHighUs;

        if (enabled_) {
            uint32_t duty = calculateDutyCount(highTimeUs_, frequency_);
            ledcWrite(channel_, duty);
        } else {
            ledcWrite(channel_, 0);
        }
    }
}

void PwmManager::setPin(uint8_t pin) {
    if (pin_ != pin) {
        pin_ = pin;
        applyHardwareSettings();
    }
}

void PwmManager::setFrequency(uint32_t freqHz) {
    if (frequency_ != freqHz && freqHz > 0) {
        frequency_ = freqHz;
        applyHardwareSettings();
    }
}

void PwmManager::setPulseWidthUs(uint16_t highTimeUs) {
    highTimeUs_ = highTimeUs;
    if (enabled_) {
        uint32_t duty = calculateDutyCount(highTimeUs_, frequency_);
        ledcWrite(channel_, duty);
    }
}

void PwmManager::setEnabled(bool enable) {
    enabled_ = enable;
    if (enabled_) {
        uint32_t duty = calculateDutyCount(highTimeUs_, frequency_);
        ledcWrite(channel_, duty);
    } else {
        ledcWrite(channel_, 0);
    }
}
