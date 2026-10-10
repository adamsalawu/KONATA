#pragma once

#include <QColor>
#include <QIcon>
#include <QPainterPath>
#include <QPixmap>
#include <QRectF>

// Vector icon system (0.2C.3): reference-measured from private/group screenshots.
// 24×24-unit box, never Unicode glyphs, never copied artwork.
// Original simplified geometry in the spirit of the target — clean strokes,
// round caps — not traced artwork.
enum class AppleIcon {
    Back,     // navigation chevron <
    Info,     // circled i
    Add,      // plus in circle (composer)
    Camera,   // camera rectangle with lens
    Video,    // FaceTime video camera (private header right)
    Mic,      // microphone (composer, light mode)
    Send,     // arrow-up in circle (composer send)
    Search,
    Compose
};

class AppleIcons
{
public:
    static constexpr qreal boxSize() { return 24.0; }
    static constexpr qreal strokeWidth() { return 2.5; }

    static QPainterPath strokePath(AppleIcon icon);
    static QPixmap pixmap(AppleIcon icon, int pixelSize, const QColor &foreground,
                          const QColor &background);
    static void paint(QPainter *painter, AppleIcon icon, const QRectF &rect,
                      const QColor &foreground, const QColor &background);
    static QIcon icon(AppleIcon icon, const QColor &foreground, const QColor &background);

private:
    AppleIcons() = delete;
    static void paintInfo(QPainter *painter, const QRectF &box, const QColor &foreground);
    static void paintSend(QPainter *painter, const QRectF &box, const QColor &foreground,
                          const QColor &background);
    static void paintVideo(QPainter *painter, const QRectF &box, const QColor &foreground);
    static void paintMic(QPainter *painter, const QRectF &box, const QColor &foreground);
};
