#ifndef UI_LABEL_H
#define UI_LABEL_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <string.h>
#include "UIComponent.h"

/**
 * @file Label.h
 * @brief Text label component with alignment and optional marquee scroll.
 *
 * The label stores a fixed-size character buffer. When the rendered text
 * is wider than the component and scroll is enabled, the text scrolls
 * horizontally using millis() (non-blocking).
 */
class Label : public UIComponent
{
public:
    enum class Alignment
    {
        Left,
        Center,
        Right
    };

    /**
     * @brief Vertical alignment of the glyphs within the label area.
     */
    enum class VAlignment
    {
        Center, // default: glyph box is vertically centered in the area
        Bottom  // e.g. all-caps text: glyphs sit on the area's bottom edge
    };

    /**
     * @brief Construct a label.
     * @param x Logical X position (top-left)
     * @param y Logical Y position (top-left)
     * @param text Initial text
     * @param font U8g2 font to use
     * @param w Logical width used for alignment / scroll bounds
     * @param h Logical height of the label area
     */
    Label(int16_t x, int16_t y, const char *text, const uint8_t *font, int16_t w = 72, int16_t h = 12)
        : UIComponent(x, y, w, h), font_(font), align_(Alignment::Left)
    {
        setText(text);
    }

    // ----------------------------------------------------------
    // Configuration
    // ----------------------------------------------------------

    /**
     * @brief Set the label text (clamped to the internal buffer).
     * @param text Null-terminated string.
     */
    void setText(const char *text)
    {
        if (text == nullptr)
        {
            buffer_[0] = '\0';
            return;
        }
        strncpy(buffer_, text, MAX_TEXT_LEN - 1);
        buffer_[MAX_TEXT_LEN - 1] = '\0';
    }

    /**
     * @brief Get the current text.
     */
    const char *text() const { return buffer_; }

    /**
     * @brief Set the font.
     * @param font U8g2 font pointer (e.g. u8g2_font_5x7_tr).
     */
    void setFont(const uint8_t *font) { font_ = font; }
    const uint8_t *font() const { return font_; }

    /**
     * @brief Set text alignment within the component width.
     * @param align Alignment enum value.
     */
    void setAlignment(Alignment align) { align_ = align; }
    Alignment alignment() const { return align_; }

    /**
     * @brief Set the vertical alignment of the glyphs in the label area.
     * @param va VAlignment enum value (Center default, Bottom for caps).
     */
    void setVAlignment(VAlignment va) { valign_ = va; }
    VAlignment vAlignment() const { return valign_; }

    /**
     * @brief Enable marquee scrolling for text wider than the component.
     * @param en true to enable scroll.
     */
    void setScrollEnabled(bool en) { scrollEnabled_ = en; }
    bool isScrollEnabled() const { return scrollEnabled_; }

    /**
     * @brief Set the scroll speed in pixels per second.
     * @param pxPerSec Pixels scrolled per second.
     */
    void setScrollSpeed(uint16_t pxPerSec)
    {
        scrollSpeed_ = (pxPerSec == 0) ? 1 : pxPerSec;
    }

    // ----------------------------------------------------------
    // UIComponent interface
    // ----------------------------------------------------------

    void update() override
    {
        if (!scrollEnabled_)
            return;

        // Advance scroll position continuously (non-blocking).
        // Use a float accumulator: integer division would round (dt * speed)
        // down to 0 at small dt values (e.g. 10ms * 20px/s = 0).
        uint32_t now = millis();
        if (lastUpdateMs_ == 0)
            lastUpdateMs_ = now;

        uint32_t dt = now - lastUpdateMs_;
        lastUpdateMs_ = now;

        scrollAccum_ += (dt * (float)scrollSpeed_) / 1000.0f;

        // Keep the accumulator bounded to the scroll cycle length so the
        // float never grows large enough to lose sub-pixel precision
        // (a 32-bit float cannot represent small increments above ~2^22).
        if (totalPx_ > 0)
            scrollAccum_ = fmodf(scrollAccum_, (float)totalPx_);

        scrollOffset_ = (uint32_t)scrollAccum_;
    }

    void draw(U8G2 &u8g2, int16_t offX, int16_t offY) override
    {
        if (buffer_[0] == '\0' || font_ == nullptr)
        {
            drawDebugBox(u8g2, offX, offY);
            return;
        }

        u8g2.setFont(font_);
        int16_t textW = u8g2.getStrWidth(buffer_);

        // Use the font's actual metrics so ascenders (D, l, t) and
        // descenders (g, y) are fully visible instead of being clipped.
        int8_t ascent = u8g2.getFontAscent();   // pixels above the baseline
        int8_t descent = u8g2.getFontDescent(); // pixels below the baseline (negative)
        int16_t glyphH = ascent - descent;

        // The clip band is always exactly the label area [y_, y_+h_).
        // This keeps the debug box and the text clip window aligned —
        // text never spills outside the visible label bounds.
        int16_t clipY0 = y_ + offY;
        int16_t clipY1 = y_ + offY + h_;
        int16_t clipH = h_;

        // Compute the baseline so the glyph box sits as requested inside
        // the label area (whose top-left is (x_, y_)).
        int16_t baselineY;
        if (valign_ == VAlignment::Bottom)
        {
            // Baseline at the last visible row. All-caps text sits on the
            // bottom edge; no pixels spill below the debug box.
            baselineY = clipY1 - 1;
        }
        else
        {
            // Vertically center the glyph box within the label area.
            int16_t oy = (clipH - glyphH) / 2;
            baselineY = clipY0 + oy + ascent;
        }

        int16_t drawX = x_ + offX;

        // Clip text drawing to the label's own horizontal bounds and the
        // vertical band so scrolling text stays inside the component
        // instead of spilling across the page.
        u8g2_t *g = u8g2.getU8g2();
        u8g2_uint_t saveX0 = g->clip_x0;
        u8g2_uint_t saveY0 = g->clip_y0;
        u8g2_uint_t saveX1 = g->clip_x1;
        u8g2_uint_t saveY1 = g->clip_y1;
        u8g2.setClipWindow(x_ + offX, clipY0,
                           x_ + offX + w_, clipY1);

        if (textW <= w_)
        {
            // Static text, apply alignment
            switch (align_)
            {
            case Alignment::Center:
                drawX += (w_ - textW) / 2;
                break;
            case Alignment::Right:
                drawX += (w_ - textW);
                break;
            case Alignment::Left:
            default:
                break;
            }

            u8g2.drawStr(drawX, baselineY, buffer_);
        }
        else if (scrollEnabled_)
        {
            // Continuous ticker: text enters from the right edge and scrolls
            // left until all characters have fully exited on the left. No
            // holds — it loops seamlessly.
            int16_t scrollRange = textW + w_; // total travel distance
            totalPx_ = (uint16_t)scrollRange; // cache for update() bounding
            uint32_t pos = (uint32_t)scrollAccum_ % totalPx_;

            // Start off the right edge and move left through the full range.
            int16_t sx = x_ + offX + w_ - (int16_t)pos;

            u8g2.drawStr(sx, baselineY, buffer_);
        }
        else
        {
            // Text wider than component and scroll disabled: left-aligned
            u8g2.drawStr(drawX, baselineY, buffer_);
        }

        // Restore the previous clip window
        u8g2.setClipWindow(saveX0, saveY0, saveX1, saveY1);

        drawDebugBox(u8g2, offX, offY);
    }

    // ----------------------------------------------------------
    // Constants
    // ----------------------------------------------------------
    static constexpr uint8_t MAX_TEXT_LEN = 64;

private:
    char buffer_[MAX_TEXT_LEN] = {0};
    const uint8_t *font_ = nullptr;
    Alignment align_ = Alignment::Left;
    VAlignment valign_ = VAlignment::Center;

    bool scrollEnabled_ = false;
    uint16_t scrollSpeed_ = 20;  // px per second
    float scrollAccum_ = 0.0f;   // float accumulator for sub-pixel scroll
    uint16_t totalPx_ = 0;       // cached scroll cycle length (pixels)
    uint32_t scrollOffset_ = 0;
    uint32_t lastUpdateMs_ = 0;
};

#endif // UI_LABEL_H