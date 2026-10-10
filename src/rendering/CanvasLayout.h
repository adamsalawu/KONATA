#pragma once

#include <QRect>
#include <QSize>

#include "core/CanvasSettings.h"

// Shared canvas-aware layout: single source of truth for preview and export.
// Given a logical canvas size (e.g., 1080×1920 for 9:16), computes header,
// message, and composer bounds that are centered and not stretched.
// Chat content is capped at a readable max width (600) and centered within
// the canvas; bubbles are 70% of the message area, not of the fixed phone.
struct CanvasLayout {
    QSize canvasSize;
    QRect contentRect; // centered readable column within canvas
    QRect headerRect;
    QRect messageRect;
    QRect composerRect;
    int bubbleMaxWidth = 0;

    static CanvasLayout fromCanvas(const Konata::Core::CanvasSettings &canvas);
    static CanvasLayout fromSize(const QSize &logicalSize);

    bool isValid() const { return canvasSize.isValid() && !canvasSize.isEmpty(); }
};
