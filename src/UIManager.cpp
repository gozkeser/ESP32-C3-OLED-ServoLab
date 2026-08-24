#include "UIManager.h"
#include <stdio.h>

UIManager::UIManager()
    : bleIcon_(0, 0, 10, 16, 74, u8g2_font_open_iconic_embedded_2x_t),
      signalBar_(12, 3, 34, 10, 7, 4, 1),
      rssiTextLabel_(50, 3, "---", u8g2_font_neuecraft_tr, 22, 11),
      PWMStatusTextLabel_(0, 20, "PWM IS OFF! SERVO LAB v0.9.1", u8g2_font_profont17_tr, 72, 20),
      BLEConnectionStatusTextLabel_(16, 3, "BLE NOT CONNECTED! SEARCH FOR \"ESP32-SERVOLAB\" ON YOUR PHONE.", u8g2_font_neuecraft_tr, 54, 11),
      PWMHighTimeTextLabel_(0, 17, "----", u8g2_font_profont22_tr, 72, 16),
      PWMHighTimeProgress_(0, 35, 72, 5)
{
    pwmValBuffer_[0] = '\0';
    statusBuffer_[0] = '\0';
}

void UIManager::setup(U8G2 &u8g2, IDataProvider &dataProvider)
{
    dataProvider_ = &dataProvider;

    // Configure BLE Icon
    bleIcon_.setDebug(false);
    bleIcon_.setBlinkEnabled(true);
    bleIcon_.setBlinkPeriod(750);

    // Configure RSSI Text Label
    rssiTextLabel_.setAlignment(Label::Alignment::Right);
    rssiTextLabel_.setVAlignment(Label::VAlignment::Bottom);
    
    BLEConnectionStatusTextLabel_.setVAlignment(Label::VAlignment::Bottom);
    BLEConnectionStatusTextLabel_.setScrollEnabled(true);
    BLEConnectionStatusTextLabel_.setScrollSpeed(40);

    PWMStatusTextLabel_.setAlignment(Label::Alignment::Left);
    PWMStatusTextLabel_.setVAlignment(Label::VAlignment::Bottom);
    PWMStatusTextLabel_.setScrollEnabled(true);
    PWMStatusTextLabel_.setScrollSpeed(40);

    PWMHighTimeTextLabel_.setVisible(false);
    PWMHighTimeTextLabel_.setAlignment(Label::Alignment::Center);
    PWMHighTimeTextLabel_.setVAlignment(Label::VAlignment::Bottom);

    PWMHighTimeProgress_.setShowPercent(false);
    PWMHighTimeProgress_.setOrientation(ProgressBar::Orientation::Horizontal);
    PWMHighTimeProgress_.setRange(500, 1500, 2500);
    PWMHighTimeProgress_.setCenterOrigin(true);
    PWMHighTimeProgress_.setValue(1500);
    PWMHighTimeProgress_.setVisible(false);
    PWMHighTimeProgress_.setFrame(false);

    // Add components to main page
    mainPage_.addComponent(&bleIcon_);
    mainPage_.addComponent(&signalBar_);
    mainPage_.addComponent(&rssiTextLabel_);
    mainPage_.addComponent(&PWMStatusTextLabel_);
    mainPage_.addComponent(&BLEConnectionStatusTextLabel_);
    mainPage_.addComponent(&PWMHighTimeTextLabel_);
    mainPage_.addComponent(&PWMHighTimeProgress_);

    dirty_ = true;
}

void UIManager::update()
{
    if (dataProvider_ == nullptr)
        return;

    bool bleConnected = dataProvider_->isBleConnected();
    int16_t rssi      = dataProvider_->getRssi();
    bool pwmEnabled   = dataProvider_->isPwmEnabled();
    uint16_t pwmHighTime = dataProvider_->getPwmHighTime();
    uint16_t pwmMin   = dataProvider_->getPwmMinHighTime();
    uint16_t pwmMax   = dataProvider_->getPwmMaxHighTime();

    // Track previous state for dirty detection
    static bool prevBleConnected = false;
    static int16_t prevRssi = -99;
    static bool prevPwmEnabled = false;
    static uint16_t prevPwmHighTime = 0;

    bool dataChanged = (bleConnected  != prevBleConnected)
                    || (pwmEnabled    != prevPwmEnabled)
                    || (pwmHighTime   != prevPwmHighTime)
                    || (rssi          != prevRssi);

    if (dataChanged) {
        prevBleConnected = bleConnected;
        prevRssi         = rssi;
        prevPwmEnabled   = pwmEnabled;
        prevPwmHighTime  = pwmHighTime;
        dirty_ = true;
    }

    // Sync BLE & Signal Status
    if (bleConnected)
    {
        bleIcon_.setBlinkEnabled(false);
        bleIcon_.setVisible(true);
        signalBar_.setVisible(true);
        signalBar_.setValue(rssi);
        snprintf(statusBuffer_, sizeof(statusBuffer_), "%d", rssi);
        rssiTextLabel_.setVisible(true);
        BLEConnectionStatusTextLabel_.setVisible(false);

        // Animations active while connected: signal bar may update
        hasActiveAnimations_ = false;
    }
    else
    {
        bleIcon_.setBlinkEnabled(true); // Blink while waiting for BLE
        signalBar_.setVisible(false);
        snprintf(statusBuffer_, sizeof(statusBuffer_), "---");
        rssiTextLabel_.setVisible(false);
        BLEConnectionStatusTextLabel_.setVisible(true);

        // Blink icon + scrolling text = active animations
        hasActiveAnimations_ = true;
    }
    rssiTextLabel_.setText(statusBuffer_);

    if (pwmEnabled)
    {
        snprintf(pwmValBuffer_, sizeof(pwmValBuffer_), "%u", pwmHighTime);
        PWMHighTimeTextLabel_.setText(pwmValBuffer_);
        PWMStatusTextLabel_.setVisible(false);
        PWMHighTimeTextLabel_.setVisible(true);
        PWMHighTimeProgress_.setVisible(true);
        PWMHighTimeProgress_.setValue(pwmHighTime);
        // No scroll animation when PWM is active (static text)
    }
    else
    {
        PWMStatusTextLabel_.setVisible(true);
        // PWM status text scrolls → active animation
        hasActiveAnimations_ = true;
        PWMHighTimeTextLabel_.setVisible(false);
        PWMHighTimeProgress_.setVisible(false);
    }

    // Update page components (drives animation state machines internally)
    mainPage_.update();
}

bool UIManager::needsRender(uint32_t nowMs)
{
    // Always render if data changed
    if (dirty_) return true;

    // If animations are active, render at ~30 Hz to keep them smooth
    if (hasActiveAnimations_) {
        if ((nowMs - lastAnimRenderMs_) >= 33) { // ~30 Hz
            lastAnimRenderMs_ = nowMs;
            return true;
        }
    }

    return false;
}

void UIManager::render(U8G2 &u8g2)
{
    mainPage_.render(u8g2);
    lastAnimRenderMs_ = millis(); // track last actual render
}
