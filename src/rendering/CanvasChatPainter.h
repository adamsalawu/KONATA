#pragma once

#include <QImage>
#include <QPainter>
#include <QRect>
#include <QSize>

#include "core/AppDefinition.h"
#include "core/CanvasSettings.h"
#include "core/Story.h"
#include "rendering/RenderModel.h"

// Single source of truth for canvas-aware chat layout and painting.
// Used by both Preview (scaled for editor) and Export (exact logical size).
// No widget geometry, no scaling of a fixed phone — the canvas itself is the composition space.
class CanvasChatPainter
{
public:
    // Paints the chat for the given conversation into painter's current transform,
    // clipped to canvasRect (logical canvas, e.g., 1080×1920). Handles background,
    // header, messages (bubbles, tails, avatars, sender names, system, day separators),
    // and composer. No editor chrome.
    static void paint(QPainter *painter, const QRect &canvasRect,
                      const Konata::Core::Story &story,
                      const Konata::Core::AppDefinition &app,
                      const QString &conversationId);

    // Convenience: renders to a QImage of the canvas's logical size.
    static QImage renderToImage(const Konata::Core::Story &story,
                                const Konata::Core::AppDefinition &app,
                                const Konata::Core::CanvasSettings &canvas,
                                const QString &conversationId);

private:
    CanvasChatPainter() = delete;
};
