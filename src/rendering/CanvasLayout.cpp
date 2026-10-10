#include "rendering/CanvasLayout.h"

#include "rendering/skins/AppleGeometry.h"

CanvasLayout CanvasLayout::fromCanvas(const Konata::Core::CanvasSettings &canvas)
{
    return fromSize(canvas.logicalSize());
}

CanvasLayout CanvasLayout::fromSize(const QSize &logicalSize)
{
    CanvasLayout layout;
    layout.canvasSize = logicalSize;
    if (!logicalSize.isValid() || logicalSize.isEmpty())
        return layout;

    const int marginH = AppleGeometry::messageMarginHorizontal; // 12
    const int headerH = AppleGeometry::chatHeaderHeight; // 68
    const int composerH = AppleGeometry::composerFieldHeight + 16; // 52

    // Readable max content width: phone-like column, not stretched to 1920.
    // 600 is chosen as comfortable readable max (between 393 and 700), centered.
    constexpr int kMaxContentWidth = 600;
    const int availableContentW = logicalSize.width() - 2 * marginH;
    const int contentW = qMin(availableContentW, kMaxContentWidth);
    const int contentX = (logicalSize.width() - contentW) / 2;
    layout.contentRect = QRect(contentX, 0, contentW, logicalSize.height());

    layout.headerRect = QRect(contentX, 0, contentW, headerH);
    layout.composerRect = QRect(contentX, logicalSize.height() - composerH, contentW, composerH);
    const int messageY = layout.headerRect.bottom() + 8;
    const int messageH = layout.composerRect.top() - messageY - 8;
    layout.messageRect = QRect(contentX + marginH, messageY, contentW - 2*marginH, qMax(0, messageH));

    // Bubble max is 70% of message area, not of fixed 393
    layout.bubbleMaxWidth = layout.messageRect.width() * 70 / 100;
    if (layout.bubbleMaxWidth < 50) layout.bubbleMaxWidth = 50;
    return layout;
}
