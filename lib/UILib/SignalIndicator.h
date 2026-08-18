#ifndef UI_SIGNAL_INDICATOR_H
#define UI_SIGNAL_INDICATOR_H

#include <Arduino.h>
#include <U8g2lib.h>
#include "UIComponent.h"

/**
 * @file SignalIndicator.h
 * @brief Segmented signal strength indicator (e.g. RSSI / Wi-Fi bars).
 *
 * Draws N vertical bars. Filled bars indicate the current signal level;
 * empty bars are drawn as outlines. The value is mapped from a configurable
 * range (e.g. -99..-30 for RSSI) onto the segment count.
 */
class SignalIndicator : public UIComponent
{
public:
    /**
     * @brief Construct a signal indicator.
     * @param x Logical X position
     * @param y Logical Y position
     * @param w Logical width in pixels
     * @param h Logical height in pixels
     */
    SignalIndicator(int16_t x, int16_t y, int16_t w, int16_t h)
        : UIComponent(x, y, w, h) {}

    // ----------------------------------------------------------
    // Value / Range
    // ----------------------------------------------------------

    /**
     * @brief Set the raw value (clamped to [min_, max_]).
     * @param value Raw value, e.g. RSSI in dBm.
     */
    void setValue(int32_t value) { value_ = clamp(value, min_, max_); }
    int32_t value() const { return value_; }

    /**
     * @brief Set the value range.
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
     * @brief Set the number of signal segments (bars).
     * @param n Number of bars (1..16).
     */
    void setSegments(uint8_t n)
    {
        segments_ = (n == 0) ? 1 : (n > 16 ? 16 : n);
    }
    uint8_t segments() const { return segments_; }

    /**
     * @brief Enable/disable drawing an outer frame.
     * @param en true to draw the frame.
     */
    void setFrame(bool en) { frame_ = en; }
    bool hasFrame() const { return frame_; }

    /**
     * @brief Set the gap (in pixels) between bars.
     * @param gap Gap in pixels.
     */
    void setGap(uint8_t gap) { gap_ = gap; }
    uint8_t gap() const { return gap_; }

    // ----------------------------------------------------------
    // UIComponent interface
    // ----------------------------------------------------------

    void draw(U8G2 &u8g2, int16_t offX, int16_t offY) override
    {
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

        if (inW <= 0 || inH <= 0 || segments_ == 0)
            return;

        // Number of filled segments based on value position within range
        float frac = (float)(value_ - min_) / (float)(max_ - min_);
        if (frac < 0.0f) frac = 0.0f;
        if (frac > 1.0f) frac = 1.0f;

        uint8_t filled = (uint8_t)(frac * segments_ + 0.5f); // round to nearest
        if (filled > segments_) filled = segments_;

        // Layout bars
        uint8_t barCount = segments_;
        int16_t totalGap = (int16_t)(barCount - 1) * gap_;
        int16_t barW = (inW - totalGap) / barCount;
        if (barW < 1) barW = 1;

        // Account for rounding remainder
        int16_t usedW = barW * barCount + totalGap;
        int16_t startX = inX + (inW - usedW) / 2;

        // Bar heights grow linearly from left to right
        for (uint8_t i = 0; i < barCount; i++)
        {
            int16_t bx = startX + (int16_t)(i * (barW + gap_));
            int16_t barH = ((i + 1) * inH) / barCount;

            if (i < filled)
            {
                u8g2.drawBox(bx, inY + inH - barH, barW, barH);
            }
            else
            {
                u8g2.drawFrame(bx, inY + inH - barH, barW, barH);
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
    int32_t min_ = -99;
    int32_t max_ = -30;

    uint8_t segments_ = 4;
    uint8_t gap_ = 2;
    bool frame_ = false;
};

#endif // UI_SIGNAL_INDICATOR_H