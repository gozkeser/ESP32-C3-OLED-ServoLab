#ifndef UI_BOX_H
#define UI_BOX_H

#include <Arduino.h>
#include <U8g2lib.h>
#include "UIComponent.h"

/**
 * @file Box.h
 * @brief Filled rectangle component with optional rounded corners.
 */
class Box : public UIComponent
{
public:
    /**
     * @brief Construct a box.
     * @param x Logical X position
     * @param y Logical Y position
     * @param w Logical width in pixels
     * @param h Logical height in pixels
     * @param radius Corner radius (0 = sharp corners).
     */
    Box(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t radius = 0)
        : UIComponent(x, y, w, h), radius_(radius) {}

    /**
     * @brief Set the corner radius.
     * @param radius Corner radius (0 = sharp corners).
     */
    void setRadius(uint8_t radius) { radius_ = radius; }
    uint8_t radius() const { return radius_; }

    // ----------------------------------------------------------
    // UIComponent interface
    // ----------------------------------------------------------

    void draw(U8G2 &u8g2, int16_t offX, int16_t offY) override
    {
        int16_t ax = x_ + offX;
        int16_t ay = y_ + offY;

        if (radius_ > 0)
        {
            u8g2.drawRBox(ax, ay, w_, h_, radius_);
        }
        else
        {
            u8g2.drawBox(ax, ay, w_, h_);
        }

        drawDebugBox(u8g2, offX, offY);
    }

private:
    uint8_t radius_ = 0;
};

#endif // UI_BOX_H