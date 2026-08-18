#ifndef UI_FRAMEWORK_H
#define UI_FRAMEWORK_H

/**
 * @file UIFramework.h
 * @brief Umbrella header for the UI component framework.
 *
 * Include this single header to access all UI components:
 *
 *   #include "UIFramework.h"
 *
 * Framework layout:
 *   - UIComponent     : Abstract base class (visibility, debug, blink, geometry)
 *   - Page            : Container that renders components with physical offset
 *   - Label           : Text with alignment and optional marquee scroll
 *   - ProgressBar     : H/V progress bar with range and optional percentage
 *   - Icon            : Glyph (icon font) or XBM bitmap icon
 *   - SignalIndicator : Segmented signal strength bars (e.g. RSSI)
 *   - SignalBar       : Discrete block signal bar with fixed box width & gap
 *   - Frame           : Rectangle outline with optional rounded corners
 *   - Box             : Filled rectangle with optional rounded corners
 *
 * All coordinates are LOGICAL (0..71 X, 0..39 Y). The Page applies the
 * physical offset (28, 24) for the 128x64 panel.
 */

#include "UIComponent.h"
#include "Page.h"
#include "Label.h"
#include "ProgressBar.h"
#include "Icon.h"
#include "SignalIndicator.h"
#include "SignalBar.h"
#include "Frame.h"
#include "Box.h"

#endif // UI_FRAMEWORK_H