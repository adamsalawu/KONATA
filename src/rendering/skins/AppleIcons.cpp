#include "rendering/skins/AppleIcons.h"

#include <QPainter>

namespace {
QRectF circleRect(qreal inset)
{
    return QRectF(inset, inset, AppleIcons::boxSize() - 2.0 * inset,
                  AppleIcons::boxSize() - 2.0 * inset);
}
} // namespace

QPainterPath AppleIcons::strokePath(AppleIcon icon)
{
    QPainterPath path;
    switch (icon) {
    case AppleIcon::Back:
        path.moveTo(14.5, 5.5);
        path.lineTo(7.5, 12.0);
        path.lineTo(14.5, 18.5);
        break;
    case AppleIcon::Add:
        path.moveTo(12.0, 5.0);
        path.lineTo(12.0, 19.0);
        path.moveTo(5.0, 12.0);
        path.lineTo(19.0, 12.0);
        break;
    case AppleIcon::Camera:
        path.addEllipse(circleRect(7.5));
        break;
    case AppleIcon::Video:
        // Video camera: rounded rect + triangle lens
        path.moveTo(3.5, 7.0);
        path.lineTo(3.5, 17.0);
        path.lineTo(14.0, 17.0);
        path.lineTo(14.0, 7.0);
        path.closeSubpath();
        break;
    case AppleIcon::Mic:
        path.moveTo(12.0, 4.5);
        path.lineTo(12.0, 9.5);
        path.moveTo(8.5, 9.5);
        path.addEllipse(QRectF(8.5, 9.5, 7.0, 7.0));
        break;
    case AppleIcon::Search:
        path.addEllipse(QRectF(4.5, 4.5, 12.0, 12.0));
        path.moveTo(13.5, 13.5);
        path.lineTo(19.5, 19.5);
        break;
    case AppleIcon::Compose:
        path.moveTo(6.0, 18.0);
        path.lineTo(6.0, 6.0);
        path.lineTo(18.0, 6.0);
        path.moveTo(10.0, 14.0);
        path.lineTo(18.0, 6.0);
        break;
    case AppleIcon::Info:
    case AppleIcon::Send:
        break;
    }
    return path;
}

void AppleIcons::paintInfo(QPainter *painter, const QRectF &box, const QColor &foreground)
{
    painter->save();
    painter->setPen(QPen(foreground, strokeWidth(), Qt::SolidLine, Qt::RoundCap));
    painter->setBrush(Qt::NoBrush);
    painter->drawEllipse(box.adjusted(2.5, 2.5, -2.5, -2.5));
    painter->setPen(Qt::NoPen);
    painter->setBrush(foreground);
    const qreal cx = box.center().x();
    painter->drawEllipse(QRectF(cx - 1.4, box.top() + 5.4, 2.8, 2.8));
    painter->drawRoundedRect(QRectF(cx - 1.4, box.top() + 9.6, 2.8, 7.6), 1.4, 1.4);
    painter->restore();
}

void AppleIcons::paintSend(QPainter *painter, const QRectF &box, const QColor &foreground,
                           const QColor &background)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(Qt::NoPen);
    painter->setBrush(foreground);
    painter->drawEllipse(box.adjusted(1.5, 1.5, -1.5, -1.5));
    painter->setPen(QPen(background, 2.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->setBrush(Qt::NoBrush);
    const qreal cx = box.center().x();
    painter->drawLine(QPointF(cx, box.top() + 6.4), QPointF(cx, box.bottom() - 5.6));
    painter->drawLine(QPointF(cx, box.top() + 6.4), QPointF(cx - 3.4, box.top() + 10.2));
    painter->drawLine(QPointF(cx, box.top() + 6.4), QPointF(cx + 3.4, box.top() + 10.2));
    painter->restore();
}

void AppleIcons::paintVideo(QPainter *painter, const QRectF &box, const QColor &foreground)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(QPen(foreground, strokeWidth(), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->setBrush(Qt::NoBrush);
    // Camera body
    painter->drawRoundedRect(QRectF(box.left() + 3.5, box.top() + 7.0, 12.5, 10.0), 2.0, 2.0);
    // Lens triangle
    QPolygonF poly;
    poly << QPointF(box.left() + 15.5, box.top() + 9.0) << QPointF(box.left() + 20.5, box.top() + 6.5)
         << QPointF(box.left() + 20.5, box.top() + 17.5) << QPointF(box.left() + 15.5, box.top() + 15.0);
    painter->drawPolygon(poly);
    painter->restore();
}

void AppleIcons::paintMic(QPainter *painter, const QRectF &box, const QColor &foreground)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(QPen(foreground, strokeWidth(), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->setBrush(Qt::NoBrush);
    // Mic capsule
    painter->drawRoundedRect(QRectF(box.center().x() - 3.2, box.top() + 4.0, 6.4, 9.0), 3.2, 3.2);
    // Arc below
    QRectF arc(box.center().x() - 5.5, box.center().y() - 1.0, 11.0, 10.0);
    painter->drawArc(arc, 200 * 16, 140 * 16);
    // Stand
    painter->drawLine(QPointF(box.center().x(), arc.bottom()), QPointF(box.center().x(), box.bottom() - 3.5));
    painter->drawLine(QPointF(box.center().x() - 3.0, box.bottom() - 3.5),
                      QPointF(box.center().x() + 3.0, box.bottom() - 3.5));
    painter->restore();
}

void AppleIcons::paint(QPainter *painter, AppleIcon icon, const QRectF &rect,
                       const QColor &foreground, const QColor &background)
{
    if (!painter)
        return;
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    const qreal side = qMin(rect.width(), rect.height());
    const QRectF box(rect.center().x() - side / 2.0, rect.center().y() - side / 2.0, side, side);
    const qreal scale = side / boxSize();
    painter->translate(box.topLeft());
    painter->scale(scale, scale);
    if (icon == AppleIcon::Info) {
        paintInfo(painter, QRectF(0.0, 0.0, boxSize(), boxSize()), foreground);
    } else if (icon == AppleIcon::Send) {
        paintSend(painter, QRectF(0.0, 0.0, boxSize(), boxSize()), foreground, background);
    } else if (icon == AppleIcon::Video) {
        paintVideo(painter, QRectF(0.0, 0.0, boxSize(), boxSize()), foreground);
    } else if (icon == AppleIcon::Mic) {
        paintMic(painter, QRectF(0.0, 0.0, boxSize(), boxSize()), foreground);
    } else if (icon == AppleIcon::Camera) {
        painter->setPen(QPen(foreground, strokeWidth(), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(QRectF(2.5, 7.0, 19.0, 13.0), 3.5, 3.5);
        painter->drawPath(strokePath(icon));
    } else {
        painter->setPen(QPen(foreground, strokeWidth(), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(strokePath(icon));
    }
    painter->restore();
}

QPixmap AppleIcons::pixmap(AppleIcon icon, int pixelSize, const QColor &foreground,
                           const QColor &background)
{
    QPixmap pixmap(qMax(1, pixelSize), qMax(1, pixelSize));
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    paint(&painter, icon, pixmap.rect(), foreground, background);
    return pixmap;
}

QIcon AppleIcons::icon(AppleIcon icon, const QColor &foreground, const QColor &background)
{
    return QIcon(pixmap(icon, 64, foreground, background));
}
