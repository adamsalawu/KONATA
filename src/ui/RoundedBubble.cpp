#include "ui/RoundedBubble.h"

#include <QPainter>
#include <QPainterPath>

RoundedBubble::RoundedBubble(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground, true);
}

void RoundedBubble::setFillColor(const QColor &color)
{
    if (color.isValid())
        m_fill = color;
    setProperty("fillColorName", m_fill.name());
    update();
}

void RoundedBubble::setCorners(int topLeft, int topRight, int bottomLeft, int bottomRight)
{
    m_topLeft = qMax(0, topLeft);
    m_topRight = qMax(0, topRight);
    m_bottomLeft = qMax(0, bottomLeft);
    m_bottomRight = qMax(0, bottomRight);
    update();
}

void RoundedBubble::setTail(bool hasTail, bool outgoing)
{
    m_hasTail = hasTail;
    m_outgoing = outgoing;
    update();
}

void RoundedBubble::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(m_fill);

    // Reserve tailWidth on side where tail protrudes, then draw rounded rect + small triangle.
    constexpr qreal tailW = 6.0;
    constexpr qreal tailH = 8.0;
    const bool tail = m_hasTail;
    QRectF bounds = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    if (tail) {
        if (m_outgoing)
            bounds.adjust(0, 0, -tailW, 0);
        else
            bounds.adjust(tailW, 0, 0, 0);
    }
    const qreal tl = qMin<qreal>(m_topLeft, qMin(bounds.width(), bounds.height()) / 2.0);
    const qreal tr = qMin<qreal>(m_topRight, qMin(bounds.width(), bounds.height()) / 2.0);
    const qreal bl = qMin<qreal>(m_bottomLeft, qMin(bounds.width(), bounds.height()) / 2.0);
    const qreal br = qMin<qreal>(m_bottomRight, qMin(bounds.width(), bounds.height()) / 2.0);
    QPainterPath path;
    path.moveTo(bounds.left(), bounds.top() + tl);
    if (tl > 0.0)
        path.arcTo(bounds.left(), bounds.top(), 2.0 * tl, 2.0 * tl, 180.0, -90.0);
    path.lineTo(bounds.right() - tr, bounds.top());
    if (tr > 0.0)
        path.arcTo(bounds.right() - 2.0 * tr, bounds.top(), 2.0 * tr, 2.0 * tr, 90.0, -90.0);
    path.lineTo(bounds.right(), bounds.bottom() - br);
    if (br > 0.0)
        path.arcTo(bounds.right() - 2.0 * br, bounds.bottom() - 2.0 * br, 2.0 * br, 2.0 * br, 0.0, -90.0);
    path.lineTo(bounds.left() + bl, bounds.bottom());
    if (bl > 0.0)
        path.arcTo(bounds.left(), bounds.bottom() - 2.0 * bl, 2.0 * bl, 2.0 * bl, 270.0, -90.0);
    path.closeSubpath();
    if (tail) {
        // Forensic: reference tail is a subtle curved lip, ~6×4px, integrated
        // via smooth cubic, not a pasted triangle. Measured from private/group
        // screenshots at native zoom: width ~6, height ~4, base ~7px from corner.
        if (m_outgoing) {
            QPainterPath tailPath;
            tailPath.moveTo(bounds.right() - 7, bounds.bottom() - 4.5);
            tailPath.cubicTo(bounds.right() - 1, bounds.bottom() - 2.5,
                             bounds.right() + 3, bounds.bottom() + 1.5,
                             bounds.right() - 2, bounds.bottom() - 0.5);
            tailPath.lineTo(bounds.right() - 7, bounds.bottom() - 4.5);
            tailPath.closeSubpath();
            path = path.united(tailPath);
        } else {
            QPainterPath tailPath;
            tailPath.moveTo(bounds.left() + 7, bounds.bottom() - 4.5);
            tailPath.cubicTo(bounds.left() + 1, bounds.bottom() - 2.5,
                             bounds.left() - 3, bounds.bottom() + 1.5,
                             bounds.left() + 2, bounds.bottom() - 0.5);
            tailPath.lineTo(bounds.left() + 7, bounds.bottom() - 4.5);
            tailPath.closeSubpath();
            path = path.united(tailPath);
        }
    }
    painter.drawPath(path);
}
