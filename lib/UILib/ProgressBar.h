#ifndef UI_PROGRESS_BAR_H
#define UI_PROGRESS_BAR_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <stdio.h>
#include "UIComponent.h"

/**
 * @file ProgressBar.h
 * @brief Progress bar component with horizontal/vertical orientation,
 *        start/center origin fill modes, value range mapping, optional frame,
 *        and percentage label.
 */
class ProgressBar : public UIComponent
{
public:
    enum class Orientation
    {
        Horizontal,
        Vertical
    };

    enum class FillOrigin
    {
        Start,  // Standard fill from start (Min -> Max)
        Center  // Bi-directional fill from Center towards Min / Max
    };

    /**
     * @brief Construct a progress bar.
     * @param x Logical X position
     * @param y Logical Y position
     * @param w Logical width in pixels
     * @param h Logical height in pixels
     */
    ProgressBar(int16_t x, int16_t y, int16_t w, int16_t h)
        : UIComponent(x, y, w, h) {}

    // ----------------------------------------------------------
    // Value / Range
    // ----------------------------------------------------------

    /**
     * @brief Set the current value (clamped to [min_, max_]).
     * @param value Current value.
     */
    void setValue(int32_t value) { value_ = clamp(value, min_, max_); }
    int32_t value() const { return value_; }

    /**
     * @brief Set the value range [min, max].
     * @param min Minimum value.
     * @param max Maximum value (must be greater than min).
     */
    void setRange(int32_t min, int32_t max)
    {
        min_ = min;
        max_ = (max > min) ? max : min + 1;
        if (!customCenter_)
        {
            centerValue_ = (min_ + max_) / 2;
        }
        value_ = clamp(value_, min_, max_);
    }

    /**
     * @brief Set the value range [min, center, max].
     * @param min Minimum value.
     * @param center Center / neutral origin value.
     * @param max Maximum value.
     */
    void setRange(int32_t min, int32_t center, int32_t max)
    {
        min_ = min;
        max_ = (max > min) ? max : min + 1;
        centerValue_ = clamp(center, min_, max_);
        customCenter_ = true;
        value_ = clamp(value_, min_, max_);
    }

    int32_t minValue() const { return min_; }
    int32_t maxValue() const { return max_; }
    int32_t centerValue() const { return centerValue_; }

    /**
     * @brief Explicitly set the center origin value.
     * @param center Neutral center value.
     */
    void setCenterValue(int32_t center)
    {
        centerValue_ = clamp(center, min_, max_);
        customCenter_ = true;
    }

    // ----------------------------------------------------------
    // Appearance
    // ----------------------------------------------------------

    /**
     * @brief Set the bar orientation.
     * @param o Orientation enum value.
     */
    void setOrientation(Orientation o) { orientation_ = o; }
    Orientation orientation() const { return orientation_; }

    /**
     * @brief Set fill origin mode (Start vs Center).
     * @param origin FillOrigin enum value.
     */
    void setFillOrigin(FillOrigin origin) { fillOrigin_ = origin; }
    FillOrigin fillOrigin() const { return fillOrigin_; }

    /**
     * @brief Enable/disable bi-directional center origin fill mode.
     * @param en true to fill bi-directionally from center.
     */
    void setCenterOrigin(bool en)
    {
        fillOrigin_ = en ? FillOrigin::Center : FillOrigin::Start;
    }
    bool isCenterOrigin() const { return fillOrigin_ == FillOrigin::Center; }

    /**
     * @brief Enable/disable drawing a central vertical/horizontal marker line.
     * @param en true to draw center line marker.
     */
    void setShowCenterLine(bool en) { showCenterLine_ = en; }
    bool isShowCenterLine() const { return showCenterLine_; }

    /**
     * @brief Enable/disable drawing an outer frame around the bar.
     * @param en true to draw the frame.
     */
    void setFrame(bool en) { frame_ = en; }
    bool hasFrame() const { return frame_; }

    /**
     * @brief Enable/disable drawing a percentage text label in the center.
     * @param en true to show the label.
     */
    void setShowPercent(bool en) { showPercent_ = en; }
    bool isShowPercent() const { return showPercent_; }

    /**
     * @brief Set the font used for the percentage label.
     * @param font U8g2 font pointer.
     */
    void setLabelFont(const uint8_t *font) { labelFont_ = font; }

    // ----------------------------------------------------------
    // UIComponent interface
    // ----------------------------------------------------------

    void draw(U8G2 &u8g2, int16_t offX, int16_t offY) override
    {
        if (!isVisibleNow()) return;

        int16_t ax = x_ + offX; // absolute X
        int16_t ay = y_ + offY; // absolute Y

        // Inner area (shrink by 1 px on each side if frame is drawn)
        int16_t inX = ax, inY = ay, inW = w_, inH = h_;
        if (frame_)
        {
            u8g2.drawFrame(ax, ay, w_, h_);
            inX += 1;
            inY += 1;
            inW -= 2;
            inH -= 2;
        }

        if (inW <= 0 || inH <= 0)
            return;

        int32_t effectiveCenter = customCenter_ ? centerValue_ : (min_ + max_) / 2;

        if (fillOrigin_ == FillOrigin::Start)
        {
            // Standard fill from Start (Min -> Max)
            float frac = (float)(value_ - min_) / (float)(max_ - min_);
            if (frac < 0.0f) frac = 0.0f;
            if (frac > 1.0f) frac = 1.0f;

            if (orientation_ == Orientation::Horizontal)
            {
                int16_t fillW = (int16_t)((float)inW * frac);
                if (fillW > 0)
                {
                    u8g2.drawBox(inX, inY, fillW, inH);
                }
            }
            else
            {
                int16_t fillH = (int16_t)((float)inH * frac);
                if (fillH > 0)
                {
                    // Vertical fill grows from the bottom
                    u8g2.drawBox(inX, inY + inH - fillH, inW, fillH);
                }
            }
        }
        else // FillOrigin::Center (Bi-directional fill from center)
        {
            if (orientation_ == Orientation::Horizontal)
            {
                int16_t halfW = inW / 2;
                int16_t centerX = inX + halfW;

                if (value_ >= effectiveCenter)
                {
                    int32_t rangeRight = max_ - effectiveCenter;
                    float frac = (rangeRight > 0) ? (float)(value_ - effectiveCenter) / (float)rangeRight : 0.0f;
                    if (frac > 1.0f) frac = 1.0f;

                    int16_t fillW = (int16_t)((float)(inW - halfW) * frac);
                    if (fillW > 0)
                    {
                        u8g2.drawBox(centerX, inY, fillW, inH);
                    }
                }
                else
                {
                    int32_t rangeLeft = effectiveCenter - min_;
                    float frac = (rangeLeft > 0) ? (float)(effectiveCenter - value_) / (float)rangeLeft : 0.0f;
                    if (frac > 1.0f) frac = 1.0f;

                    int16_t fillW = (int16_t)((float)halfW * frac);
                    if (fillW > 0)
                    {
                        u8g2.drawBox(centerX - fillW, inY, fillW, inH);
                    }
                }

                if (showCenterLine_)
                {
                    u8g2.drawVLine(centerX, inY, inH);
                }
            }
            else // Vertical Center Origin
            {
                int16_t halfH = inH / 2;
                int16_t centerY = inY + halfH;

                if (value_ >= effectiveCenter)
                {
                    int32_t rangeUp = max_ - effectiveCenter;
                    float frac = (rangeUp > 0) ? (float)(value_ - effectiveCenter) / (float)rangeUp : 0.0f;
                    if (frac > 1.0f) frac = 1.0f;

                    int16_t fillH = (int16_t)((float)halfH * frac);
                    if (fillH > 0)
                    {
                        u8g2.drawBox(inX, centerY - fillH, inW, fillH);
                    }
                }
                else
                {
                    int32_t rangeDown = effectiveCenter - min_;
                    float frac = (rangeDown > 0) ? (float)(effectiveCenter - value_) / (float)rangeDown : 0.0f;
                    if (frac > 1.0f) frac = 1.0f;

                    int16_t fillH = (int16_t)((float)(inH - halfH) * frac);
                    if (fillH > 0)
                    {
                        u8g2.drawBox(inX, centerY, inW, fillH);
                    }
                }

                if (showCenterLine_)
                {
                    u8g2.drawHLine(inX, centerY, inW);
                }
            }
        }

        // Percentage label (only if there is enough space)
        if (showPercent_ && labelFont_ != nullptr && w_ >= 20 && h_ >= 8)
        {
            char buf[8];
            float frac = (float)(value_ - min_) / (float)(max_ - min_);
            if (frac < 0.0f) frac = 0.0f;
            if (frac > 1.0f) frac = 1.0f;

            uint8_t pct = (uint8_t)(frac * 100.0f);
            snprintf(buf, sizeof(buf), "%u%%", pct);

            u8g2.setFont(labelFont_);
            int16_t textW = u8g2.getStrWidth(buf);
            int16_t textX = ax + (w_ - textW) / 2;
            int16_t textY = ay + h_ / 2 + 3;
            u8g2.drawStr(textX, textY, buf);
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
    int32_t min_ = 0;
    int32_t max_ = 100;
    int32_t centerValue_ = 50;
    bool customCenter_ = false;

    Orientation orientation_ = Orientation::Horizontal;
    FillOrigin fillOrigin_ = FillOrigin::Start;
    bool showCenterLine_ = false;

    bool frame_ = true;
    bool showPercent_ = false;
    const uint8_t *labelFont_ = nullptr;
};

#endif // UI_PROGRESS_BAR_H