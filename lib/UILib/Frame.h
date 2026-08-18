#ifndef UI_FRAME_H
#define UI_FRAME_H

#include <Arduino.h>
#include <U8g2lib.h>
#include "UIComponent.h"

/**
 * @file Frame.h
 * @brief Rectangle frame (outline) component with optional rounded corners.
 */
class Frame : public UIComponent
{
public:
    /**
     * @brief Construct a frame.
     * @param x Logical X position
     * @param y Logical Y position
     * @param w Logical width in pixels
     * @param h Logical height in pixels
     * @param radius Corner radius (0 = sharp corners).
     */
    Frame(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t radius = 0)
        : UIComponent(x, y, w, h), radius_(radius) {}

    /**
     * @brief Set the corner radius.
     * @param radius Corner radius (0 = sharp corners).
     */
    void setRadius(uint8_t radius) { radius_ = radius; }
    uint8_t radius() const { return radius_; }

    /**
     * @brief Set the line thickness in pixels (1 or 2 supported by u8g2).
     * @param thickness Line thickness.
     */
    void setThickness(uint8_t thickness) { thickness_ = thickness; }
    uint8_t thickness() const { return thickness_; }

    // ----------------------------------------------------------
    // UIComponent interface
    // ----------------------------------------------------------

    void draw(U8G2 &u8g2, int16_t offX, int16_t offY) override
    {
        int16_t ax = x_ + offX;
        int16_t ay = y_ + offY;

        if (radius_ > 0)
        {
            u8g2.drawRFrame(ax, ay, w_, h_, radius_);
        }
        else
        {
            u8g2.drawFrame(ax, ay, w_, h_);
        }

        // u8g2 thickness support: draw a second inset frame for 2px thickness
        if (thickness_ >= 2 && w_ > 2 && h_ > 2)
        {
            u8g2.drawFrame(ax + 1, ay + 1, w_ - 2, h_ - 2);
        }

        drawDebugBox(u8g2, offX, offY);
    }

private:
    uint8_t radius_ = 0;
    uint8_t thickness_ = 1;
};

#endif // UI_FRAME_H