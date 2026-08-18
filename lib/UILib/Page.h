#ifndef UI_PAGE_H
#define UI_PAGE_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <vector>
#include "UIComponent.h"

/**
 * @file Page.h
 * @brief Container that owns and renders a list of UI components.
 *
 * The Page works in LOGICAL screen space (72 x 40 pixels) and applies
 * the physical display offset (e.g. 28, 24 on a 128x64 panel) when
 * rendering. Components are drawn on top of each other in the order
 * they were added.
 */
class Page
{
public:
    /**
     * @brief Construct a page with a logical size and physical offset.
     * @param width  Logical width in pixels (e.g. 72)
     * @param height Logical height in pixels (e.g. 40)
     * @param offsetX Physical X offset on the display (e.g. 28)
     * @param offsetY Physical Y offset on the display (e.g. 24)
     */
    Page(int16_t width = 72, int16_t height = 40,
         int16_t offsetX = 28, int16_t offsetY = 24)
        : width_(width), height_(height), offsetX_(offsetX), offsetY_(offsetY) {}

    /**
     * @brief Add a component to the page.
     * @param comp Pointer to the component. Ownership stays with the caller.
     */
    void addComponent(UIComponent *comp)
    {
        components_.push_back(comp);
    }

    /**
     * @brief Remove a component from the page.
     * @param comp Pointer to the component to remove. No deletion is performed.
     */
    void removeComponent(UIComponent *comp)
    {
        for (auto it = components_.begin(); it != components_.end(); ++it)
        {
            if (*it == comp)
            {
                components_.erase(it);
                return;
            }
        }
    }

    /**
     * @brief Remove all components. No deletion is performed.
     */
    void clearComponents()
    {
        components_.clear();
    }

    /**
     * @brief Update all components (calls update() on each).
     *        Should be called once per loop iteration.
     */
    void update()
    {
        for (auto *comp : components_)
        {
            comp->update();
        }
    }

    /**
     * @brief Clear the buffer, draw all visible components, and send the buffer.
     * @param u8g2 Reference to the U8g2 display object.
     */
    void render(U8G2 &u8g2)
    {
        u8g2.clearBuffer();

        // Use exclusive bounds (offset + size) so the right/bottom edge
        // pixels of full-size components (e.g. a 72x40 Frame) are not
        // clipped away. The physical panel clips anything beyond 128x64.
        u8g2.setClipWindow(offsetX_, offsetY_,
                           offsetX_ + width_,
                           offsetY_ + height_);

        for (auto *comp : components_)
        {
            if (!comp->isVisibleNow())
                continue;

            comp->draw(u8g2, offsetX_, offsetY_);
        }

        u8g2.setMaxClipWindow();
        u8g2.sendBuffer();
    }

    /**
     * @brief Enable/disable debug bounding boxes on all components.
     * @param d true to show bounding boxes, false to hide.
     */
    void setDebug(bool d)
    {
        for (auto *comp : components_)
        {
            comp->setDebug(d);
        }
    }

    // ----------------------------------------------------------
    // Getters
    // ----------------------------------------------------------

    int16_t width() const { return width_; }
    int16_t height() const { return height_; }
    int16_t offsetX() const { return offsetX_; }
    int16_t offsetY() const { return offsetY_; }
    size_t componentCount() const { return components_.size(); }

private:
    std::vector<UIComponent *> components_;
    int16_t width_;
    int16_t height_;
    int16_t offsetX_;
    int16_t offsetY_;
};

#endif // UI_PAGE_H