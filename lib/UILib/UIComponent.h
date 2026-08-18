#ifndef UI_COMPONENT_H
#define UI_COMPONENT_H

#include <Arduino.h>
#include <U8g2lib.h>

/**
 * @file UIComponent.h
 * @brief Abstract base class for all UI components.
 *
 * All coordinates are in LOGICAL screen space (0..71 X, 0..39 Y).
 * The Page class passes the physical offset (X_OFFSET / Y_OFFSET) to
 * draw(), and every derived component must add these offsets to all
 * draw calls so that rendering stays inside the visible window.
 */
class UIComponent
{
public:
    /**
     * @brief Construct a UI component.
     * @param x Logical X position (0..71)
     * @param y Logical Y position (0..39)
     * @param w Logical width in pixels
     * @param h Logical height in pixels
     */
    UIComponent(int16_t x, int16_t y, int16_t w, int16_t h)
        : x_(x), y_(y), w_(w), h_(h) {}

    virtual ~UIComponent() = default;

    // ----------------------------------------------------------
    // Lifecycle
    // ----------------------------------------------------------

    /**
     * @brief Draw the component using the given U8g2 device.
     * @param u8g2 Reference to the U8g2 display object.
     * @param offX Physical X offset to add to all logical X coordinates.
     * @param offY Physical Y offset to add to all logical Y coordinates.
     */
    virtual void draw(U8G2 &u8g2, int16_t offX, int16_t offY) = 0;

    /**
     * @brief Called every frame by Page::update().
     *        Override in derived classes for non-blocking, time-based
     *        behaviour (e.g. counts, scroll positions, custom logic).
     */
    virtual void update() {}

    // ----------------------------------------------------------
    // Visibility
    // ----------------------------------------------------------

    void setVisible(bool v) { visible_ = v; }
    bool isVisible() const { return visible_; }

    /**
     * @brief Effective visibility including blink animation state.
     * @return true if the component should be drawn this frame.
     */
    bool isVisibleNow() const
    {
        if (!visible_) return false;
        if (!blinkEnabled_) return true;
        return ((millis() / blinkPeriodMs_) % 2) == 0;
    }

    // ----------------------------------------------------------
    // Debug
    // ----------------------------------------------------------

    void setDebug(bool d) { debug_ = d; }
    bool isDebug() const { return debug_; }

    // ----------------------------------------------------------
    // Blink animation (non-blocking, millis() based)
    // ----------------------------------------------------------

    void setBlinkEnabled(bool en) { blinkEnabled_ = en; }
    bool isBlinkEnabled() const { return blinkEnabled_; }

    /**
     * @brief Set the blink period in milliseconds.
     * @param ms Period of one full blink cycle (ON + OFF).
     */
    void setBlinkPeriod(uint32_t ms) { blinkPeriodMs_ = (ms == 0) ? 1 : ms; }
    uint32_t blinkPeriod() const { return blinkPeriodMs_; }

    // ----------------------------------------------------------
    // Geometry
    // ----------------------------------------------------------

    void setPosition(int16_t x, int16_t y) { x_ = x; y_ = y; }
    void setSize(int16_t w, int16_t h) { w_ = w; h_ = h; }

    int16_t x() const { return x_; }
    int16_t y() const { return y_; }
    int16_t width() const { return w_; }
    int16_t height() const { return h_; }

    // ----------------------------------------------------------
    // Common helpers
    // ----------------------------------------------------------

    /**
     * @brief Draw a debug bounding box around this component.
     *        Call at the end of draw() in derived classes when debug_ is true.
     * @param u8g2 Reference to the U8g2 display object.
     * @param offX Physical X offset.
     * @param offY Physical Y offset.
     */
    void drawDebugBox(U8G2 &u8g2, int16_t offX, int16_t offY) const
    {
        if (debug_)
        {
            u8g2.drawFrame(x_ + offX, y_ + offY, w_, h_);
        }
    }

protected:
    int16_t x_, y_;   // Logical top-left position
    int16_t w_, h_;   // Logical size

    bool visible_ = true;
    bool debug_ = false;

    bool blinkEnabled_ = false;
    uint32_t blinkPeriodMs_ = 750;
};

#endif // UI_COMPONENT_H