#ifndef UI_SIGNAL_BAR_H
#define UI_SIGNAL_BAR_H

#include <Arduino.h>
#include <U8g2lib.h>
#include "UIComponent.h"

/**
 * @file SignalBar.h
 * @brief Discrete block signal strength indicator component.
 *
 * Draws N discrete boxes of fixed width separated by a configurable gap.
 * Filled boxes indicate current signal strength; empty boxes are drawn as outlines.
 * Component width (w_) is strictly defined by the user in design time.
 */
class SignalBar : public UIComponent
{
public:
    /**
     * @brief Construct a discrete signal bar.
     * @param x Logical X position
     * @param y Logical Y position
     * @param w Logical width in pixels (defined by user)
     * @param h Logical height in pixels
     * @param boxCount Number of signal boxes/segments
     * @param boxWidth Fixed width of each box in pixels
     * @param gap Pixel spacing between boxes
     */
    SignalBar(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t boxCount = 6, int16_t boxWidth = 5, uint8_t gap = 2)
        : UIComponent(x, y, w, h), boxCount_(boxCount == 0 ? 1 : boxCount), boxWidth_(boxWidth < 1 ? 1 : boxWidth), gap_(gap) {}

    // ----------------------------------------------------------
    // Value / Range
    // ----------------------------------------------------------

    /**
     * @brief Set current value (clamped to [min_, max_]).
     * @param value Raw value (e.g. RSSI in dBm).
     */
    void setValue(int32_t value) { value_ = clamp(value, min_, max_); }
    int32_t value() const { return value_; }

    /**
     * @brief Set value range.
     * @param min Minimum value (e.g. -99).
     * @param max Maximum value (e.g. -30).
     */
    void setRange(int32_t min, int32_t max)
    {
        min_ = min;
        max_ = (max > min) ? max : min + 1;
        value_ = clamp(value_, min_, max_);
    }

    int32_t minValue() const { return min_; }
    int32_t maxValue() const { return max_; }

    // ----------------------------------------------------------
    // Appearance
    // ----------------------------------------------------------

    /**
     * @brief Set number of signal boxes (1..32).
     * @param count Number of boxes.
     */
    void setBoxCount(uint8_t count)
    {
        boxCount_ = (count == 0) ? 1 : (count > 32 ? 32 : count);
    }
    uint8_t boxCount() const { return boxCount_; }

    /**
     * @brief Set fixed width of each box in pixels.
     * @param width Box width in pixels.
     */
    void setBoxWidth(int16_t width)
    {
        boxWidth_ = (width < 1) ? 1 : width;
    }
    int16_t boxWidth() const { return boxWidth_; }

    /**
     * @brief Set gap in pixels between boxes.
     * @param gap Gap in pixels.
     */
    void setGap(uint8_t gap) { gap_ = gap; }
    uint8_t gap() const { return gap_; }

    /**
     * @brief Enable/disable drawing an outer frame around component.
     * @param en true to draw frame.
     */
    void setFrame(bool en) { frame_ = en; }
    bool hasFrame() const { return frame_; }

    /**
     * @brief Enable/disable growing height per box (linear RSSI style vs uniform block style).
     * @param en true for growing height, false for uniform full height.
     */
    void setGrowingHeight(bool en) { growingHeight_ = en; }
    bool isGrowingHeight() const { return growingHeight_; }

    /**
     * @brief Calculate required width for current box count, box width and gap.
     * @return Total required width in pixels.
     */
    int16_t calculateRequiredWidth() const
    {
        return (boxCount_ * boxWidth_) + ((boxCount_ - 1) * gap_);
    }

    // ----------------------------------------------------------
    // UIComponent interface
    // ----------------------------------------------------------

    void draw(U8G2 &u8g2, int16_t offX, int16_t offY) override
    {
        if (!isVisibleNow()) return;

        int16_t ax = x_ + offX;
        int16_t ay = y_ + offY;

        int16_t inX = ax, inY = ay, inW = w_, inH = h_;
        if (frame_)
        {
            u8g2.drawFrame(ax, ay, w_, h_);
            inX += 1;
            inY += 1;
            inW -= 2;
            inH -= 2;
        }

        if (inW <= 0 || inH <= 0 || boxCount_ == 0)
            return;

        // Normalized fill fraction 0.0 .. 1.0
        float frac = (float)(value_ - min_) / (float)(max_ - min_);
        if (frac < 0.0f) frac = 0.0f;
        if (frac > 1.0f) frac = 1.0f;

        // Number of filled boxes
        uint8_t filled = (uint8_t)(frac * boxCount_ + 0.5f);
        if (filled > boxCount_) filled = boxCount_;

        // Start drawing directly at left edge (inX)
        for (uint8_t i = 0; i < boxCount_; i++)
        {
            int16_t bx = inX + i * (boxWidth_ + gap_);
            int16_t bh = inH;
            int16_t by = inY;

            if (growingHeight_)
            {
                bh = ((i + 1) * inH) / boxCount_;
                if (bh < 1) bh = 1;
                by = inY + (inH - bh);
            }

            if (i < filled)
            {
                u8g2.drawBox(bx, by, boxWidth_, bh);
            }
            else
            {
                u8g2.drawFrame(bx, by, boxWidth_, bh);
            }
        }

        drawDebugBox(u8g2, offX, offY);
    }

private:
    static int32_t clamp(int32_t v, int32_t lo, int32_t hi)
    {
        if (v < lo) return lo;
        if (v > hi) return hi;
        return v;
    }

    int32_t value_ = 0;
    int32_t min_ = -100;
    int32_t max_ = -50;

    uint8_t boxCount_ = 6;
    int16_t boxWidth_ = 5;
    uint8_t gap_ = 2;
    bool frame_ = false;
    bool growingHeight_ = false;
};

#endif // UI_SIGNAL_BAR_H
