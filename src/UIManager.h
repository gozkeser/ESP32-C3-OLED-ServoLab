#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <Arduino.h>
#include <U8g2lib.h>
#include "UIFramework.h"
#include "IDataProvider.h"

/**
 * @file UIManager.h
 * @brief User Interface Manager controlling pages, updates, and rendering.
 *
 * Manages UI pages and component bindings. Decoupled from concrete system
 * hardware state by consuming data through the IDataProvider interface.
 * Uses dirty-flag + animation-aware render gating to minimize SPI traffic.
 */
class UIManager
{
public:
    UIManager();
    ~UIManager() = default;

    /**
     * @brief Initialize UI manager, pages, and components.
     * @param u8g2 Reference to display instance.
     * @param dataProvider Reference to system data provider interface.
     */
    void setup(U8G2 &u8g2, IDataProvider &dataProvider);

    /**
     * @brief Periodically sync UI components with data provider state.
     * Sets dirty flag if data changed. Must be called every loop.
     */
    void update();

    /**
     * @brief Render active UI pages to the display.
     * @param u8g2 Reference to display instance.
     */
    void render(U8G2 &u8g2);

    /**
     * @brief Returns true if a render pass is needed.
     * Accounts for both data changes (dirty flag) and active animations
     * (scrolling text, blinking icon) which need periodic re-renders.
     * @param nowMs Current millis() timestamp.
     */
    bool needsRender(uint32_t nowMs);

    /** @brief Clears the dirty flag after a successful render. */
    void clearDirty() { dirty_ = false; }

private:
    IDataProvider *dataProvider_ = nullptr;

    // Render gating
    bool dirty_ = true;                 // Data changed, render needed
    bool hasActiveAnimations_ = false;  // Scrolling text or blink active
    uint32_t lastAnimRenderMs_ = 0;     // Last animation-driven render timestamp

    // Active UI Page container
    Page mainPage_;

    // Header Status Bar Components
    Icon bleIcon_;
    SignalBar signalBar_;
    Label rssiTextLabel_;
    Label BLEConnectionStatusTextLabel_;
    Label PWMStatusTextLabel_;
    Label PWMHighTimeTextLabel_;
    ProgressBar PWMHighTimeProgress_;

    char pwmValBuffer_[16];
    char statusBuffer_[16];
};

#endif // UI_MANAGER_H
