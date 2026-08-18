#ifndef UI_ICON_H
#define UI_ICON_H

#include <Arduino.h>
#include <U8g2lib.h>
#include "UIComponent.h"

/**
 * @file Icon.h
 * @brief Icon component that renders a glyph from an icon font
 *        (e.g. u8g2_font_open_iconic_*) or a raw XBM bitmap.
 */
class Icon : public UIComponent
{
public:
    /**
     * @brief Construct a glyph-based icon.
     * @param x Logical X position
     * @param y Logical Y position
     * @param w Logical width (bounding area)
     * @param h Logical height (bounding area)
     * @param glyph U8g2 glyph code (e.g. 74 for the BLE symbol)
     * @param font Icon font pointer (e.g. u8g2_font_open_iconic_embedded_4x_t)
     */
    Icon(int16_t x, int16_t y, int16_t w, int16_t h,
         uint8_t glyph, const uint8_t *font)
        : UIComponent(x, y, w, h), glyph_(glyph), font_(font), useBitmap_(false) {}

    /**
     * @brief Construct a bitmap-based icon from XBM data.
     * @param x Logical X position
     * @param y Logical Y position
     * @param w Bitmap width in pixels
     * @param h Bitmap height in pixels
     * @param bitmap XBM byte array
     */
    Icon(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *bitmap)
        : UIComponent(x, y, w, h), bitmap_(bitmap), useBitmap_(true) {}

    // ----------------------------------------------------------
    // Configuration
    // ----------------------------------------------------------

    /**
     * @brief Switch to glyph rendering mode.
     * @param glyph U8g2 glyph code.
     * @param font Icon font pointer.
     */
    void setGlyph(uint8_t glyph, const uint8_t *font)
    {
        glyph_ = glyph;
        font_ = font;
        useBitmap_ = false;
    }

    /**
     * @brief Switch to bitmap rendering mode.
     * @param bitmap XBM byte array.
     */
    void setBitmap(const uint8_t *bitmap)
    {
        bitmap_ = bitmap;
        useBitmap_ = true;
    }

    /**
     * @brief Center the icon inside its bounding area.
     * @param en true to center, false to draw at top-left.
     */
    void setCentered(bool en) { centered_ = en; }
    bool isCentered() const { return centered_; }

    // ----------------------------------------------------------
    // UIComponent interface
    // ----------------------------------------------------------

    void draw(U8G2 &u8g2, int16_t offX, int16_t offY) override
    {
        int16_t ax = x_ + offX;
        int16_t ay = y_ + offY;

        if (useBitmap_)
        {
            if (bitmap_ == nullptr)
            {
                drawDebugBox(u8g2, offX, offY);
                return;
            }

            int16_t drawX = centered_ ? ax + (w_ - w_) / 2 : ax; // bitmap width == w_
            int16_t drawY = centered_ ? ay + (h_ - h_) / 2 : ay; // bitmap height == h_
            u8g2.drawXBM(drawX, drawY, w_, h_, bitmap_);
        }
        else
        {
            if (font_ == nullptr)
            {
                drawDebugBox(u8g2, offX, offY);
                return;
            }

            u8g2.setFont(font_);

            // Glyph width/height from the font
            uint8_t glyphW = u8g2.getMaxCharWidth();
            uint8_t glyphH = u8g2.getFontAscent() - u8g2.getFontDescent();

            int16_t drawX = centered_ ? ax + (w_ - glyphW) / 2 : ax;
            int16_t drawY = centered_ ? ay + (h_ - glyphH) / 2 + u8g2.getFontAscent() : ay + u8g2.getFontAscent();

            u8g2.drawGlyph(drawX, drawY, glyph_);
        }

        drawDebugBox(u8g2, offX, offY);
    }

private:
    uint8_t glyph_ = 0;
    const uint8_t *font_ = nullptr;
    const uint8_t *bitmap_ = nullptr;
    bool useBitmap_ = false;
    bool centered_ = true;
};

#endif // UI_ICON_H