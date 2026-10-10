#include "ui/UiElementGraphicsItem.h"

#include <QPainter>
#include <QPen>
#include <QCursor>
#include <QFileInfo>

using namespace Konata::Core;

UiElementGraphicsItem::UiElementGraphicsItem(const UiElement &element, QGraphicsItem *parent)
    : QGraphicsItem(parent), m_element(element)
{
    setPos(m_element.x, m_element.y);
    setZValue(m_element.zValue);
    setOpacity(m_element.opacity);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, !m_element.locked);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
}

void UiElementGraphicsItem::setElement(const UiElement &el)
{
    prepareGeometryChange();
    m_element = el;
    setPos(m_element.x, m_element.y);
    setZValue(m_element.zValue);
    setOpacity(m_element.opacity);
    setFlag(QGraphicsItem::ItemIsMovable, !m_element.locked);
    setVisible(m_element.visible);
    update();
}

QRectF UiElementGraphicsItem::boundingRect() const
{
    const double m = kHandleSize + 2;
    return QRectF(-m, -m, m_element.width + 2 * m, m_element.height + 2 * m);
}

UiElementGraphicsItem::Handle UiElementGraphicsItem::handleAt(const QPointF &pos) const
{
    if (!isSelected()) return NoHandle;
    const double hs = kHandleSize;
    const double w = m_element.width;
    const double h = m_element.height;
    auto near = [](double v1, double v2, double tolerance) { return qAbs(v1 - v2) <= tolerance; };
    if (near(pos.x(), 0, hs) && near(pos.y(), 0, hs)) return TopLeft;
    if (near(pos.x(), w, hs) && near(pos.y(), 0, hs)) return TopRight;
    if (near(pos.x(), 0, hs) && near(pos.y(), h, hs)) return BottomLeft;
    if (near(pos.x(), w, hs) && near(pos.y(), h, hs)) return BottomRight;
    if (near(pos.x(), 0, hs) && pos.y() > hs && pos.y() < h - hs) return Left;
    if (near(pos.x(), w, hs) && pos.y() > hs && pos.y() < h - hs) return Right;
    if (near(pos.y(), 0, hs) && pos.x() > hs && pos.x() < w - hs) return Top;
    if (near(pos.y(), h, hs) && pos.x() > hs && pos.x() < w - hs) return Bottom;
    return NoHandle;
}

QCursor UiElementGraphicsItem::cursorForHandle(Handle h)
{
    switch (h) {
    case TopLeft:     return Qt::SizeFDiagCursor;
    case TopRight:    return Qt::SizeBDiagCursor;
    case BottomLeft:  return Qt::SizeBDiagCursor;
    case BottomRight: return Qt::SizeFDiagCursor;
    case Left:        return Qt::SizeHorCursor;
    case Right:       return Qt::SizeHorCursor;
    case Top:         return Qt::SizeVerCursor;
    case Bottom:      return Qt::SizeVerCursor;
    default:          return Qt::ArrowCursor;
    }
}

QVariant UiElementGraphicsItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionHasChanged) {
        QPointF pos = value.toPointF();
        m_element.x = pos.x();
        m_element.y = pos.y();
    }
    return QGraphicsItem::itemChange(change, value);
}

void UiElementGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::TextAntialiasing, true);
    paintElement(painter);
    paintSelection(painter);
    Q_UNUSED(option);
}

void UiElementGraphicsItem::paintElement(QPainter *painter)
{
    const double w = m_element.width;
    const double h = m_element.height;

    switch (m_element.type) {
    case UiElementType::Rectangle: {
        if (!m_element.fill.isEmpty()) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(m_element.fill));
            painter->drawRect(0, 0, w, h);
        }
        if (m_element.strokeWidth > 0 && !m_element.stroke.isEmpty()) {
            painter->setPen(QPen(QColor(m_element.stroke), m_element.strokeWidth));
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(0, 0, w, h);
        }
        break;
    }
    case UiElementType::RoundedRect: {
        const double r = m_element.cornerRadius;
        if (!m_element.fill.isEmpty()) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(m_element.fill));
            double tl = m_element.cornerRadiusTL >= 0 ? m_element.cornerRadiusTL : r;
            double tr = m_element.cornerRadiusTR >= 0 ? m_element.cornerRadiusTR : r;
            double bl = m_element.cornerRadiusBL >= 0 ? m_element.cornerRadiusBL : r;
            double br = m_element.cornerRadiusBR >= 0 ? m_element.cornerRadiusBR : r;
            QPainterPath path;
            path.moveTo(tl, 0);
            path.lineTo(w - tr, 0);
            if (tr > 0) path.arcTo(w - 2*tr, 0, 2*tr, 2*tr, 90, -90);
            path.lineTo(w, h - br);
            if (br > 0) path.arcTo(w - 2*br, h - 2*br, 2*br, 2*br, 0, -90);
            path.lineTo(bl, h);
            if (bl > 0) path.arcTo(0, h - 2*bl, 2*bl, 2*bl, 270, -90);
            path.lineTo(0, tl);
            if (tl > 0) path.arcTo(0, 0, 2*tl, 2*tl, 180, -90);
            path.closeSubpath();
            painter->drawPath(path);
        }
        if (m_element.strokeWidth > 0 && !m_element.stroke.isEmpty()) {
            painter->setPen(QPen(QColor(m_element.stroke), m_element.strokeWidth));
            painter->setBrush(Qt::NoBrush);
            painter->drawRoundedRect(0, 0, w, h, r, r);
        }
        break;
    }
    case UiElementType::Circle: {
        if (!m_element.fill.isEmpty()) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(m_element.fill));
            painter->drawEllipse(QRectF(0, 0, w, h));
        }
        if (m_element.strokeWidth > 0 && !m_element.stroke.isEmpty()) {
            painter->setPen(QPen(QColor(m_element.stroke), m_element.strokeWidth));
            painter->setBrush(Qt::NoBrush);
            painter->drawEllipse(QRectF(0, 0, w, h));
        }
        break;
    }
    case UiElementType::Line: {
        painter->setPen(QPen(QColor(m_element.lineColor.isEmpty() ? QStringLiteral("#555555") : m_element.lineColor), m_element.lineWidth));
        painter->drawLine(QPointF(0, 0), QPointF(w, h));
        break;
    }
    case UiElementType::Image: {
        if (!m_element.fill.isEmpty()) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(m_element.fill));
            painter->drawRect(0, 0, w, h);
        }
        if (!m_element.imageRef.isEmpty()) {
            QPixmap pix(m_element.imageRef);
            if (!pix.isNull()) {
                pix = pix.scaled(QSize(w, h), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                double ox = (w - pix.width()) / 2.0;
                double oy = (h - pix.height()) / 2.0;
                painter->drawPixmap(QPointF(ox, oy), pix);
            }
        }
        painter->setPen(QColor(QStringLiteral("#8E8E93")));
        painter->setBrush(Qt::NoBrush);
        painter->setFont(QFont(QStringLiteral("Segoe UI"), 10));
        painter->drawText(QRectF(0, 0, w, h), Qt::AlignCenter,
                          m_element.imageRef.isEmpty() ? QStringLiteral("Image") : QFileInfo(m_element.imageRef).fileName());
        break;
    }
    case UiElementType::Text: {
        if (!m_element.fill.isEmpty()) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(m_element.fill));
            painter->drawRect(0, 0, w, h);
        }
        painter->setPen(QColor(m_element.textColor.isEmpty() ? QStringLiteral("#FFFFFF") : m_element.textColor));
        painter->setBrush(Qt::NoBrush);
        QFont f(m_element.fontFamily.isEmpty() ? QStringLiteral("Segoe UI") : m_element.fontFamily, m_element.fontSize);
        f.setWeight(QFont::Weight(m_element.fontWeight));
        f.setItalic(m_element.fontItalic);
        painter->setFont(f);
        Qt::Alignment align = Qt::AlignLeft | Qt::AlignTop;
        if (m_element.textAlign == 1) align = Qt::AlignHCenter | Qt::AlignTop;
        else if (m_element.textAlign == 2) align = Qt::AlignRight | Qt::AlignTop;
        painter->drawText(QRectF(4, 2, w - 8, h - 4), align | Qt::TextWordWrap, m_element.text);
        break;
    }
    case UiElementType::MessageBubble: {
        const QColor fillColor(m_element.bubbleColor.isEmpty() ? m_element.fill : m_element.bubbleColor);
        const double r = m_element.bubbleCornerRadius;
        const double gr = m_element.groupedCornerRadius;
        QPainterPath path;
        QRectF bounds(0, 0, w, h);
        if (m_element.tailEnabled) {
            if (m_element.bubbleOutgoing) bounds.adjust(0, 0, -m_element.tailWidth, 0);
            else bounds.adjust(m_element.tailWidth, 0, 0, 0);
        }
        double tl = r, tr = r, bl = r, br = r;
        path.moveTo(bounds.left() + tl, bounds.top());
        if (tr > 0) path.arcTo(bounds.right() - 2*tr, bounds.top(), 2*tr, 2*tr, 90, -90);
        path.lineTo(bounds.right(), bounds.bottom() - br);
        if (br > 0) path.arcTo(bounds.right() - 2*br, bounds.bottom() - 2*br, 2*br, 2*br, 0, -90);
        path.lineTo(bounds.left() + bl, bounds.bottom());
        if (bl > 0) path.arcTo(bounds.left(), bounds.bottom() - 2*bl, 2*bl, 2*bl, 270, -90);
        path.lineTo(bounds.left(), bounds.top() + tl);
        if (tl > 0) path.arcTo(bounds.left(), bounds.top(), 2*tl, 2*tl, 180, -90);
        path.closeSubpath();
        if (m_element.tailEnabled) {
            QPainterPath tail;
            const double bx = m_element.bubbleOutgoing ? bounds.right() : bounds.left();
            tail.moveTo(bx - (m_element.bubbleOutgoing ? 7 : -7), bounds.bottom() - 4.5);
            tail.cubicTo(bx - (m_element.bubbleOutgoing ? 1 : -1), bounds.bottom() - 2.5,
                         bx + (m_element.bubbleOutgoing ? 3 : -3), bounds.bottom() + 1.5,
                         bx - (m_element.bubbleOutgoing ? 2 : -2), bounds.bottom() - 0.5);
            tail.lineTo(bx - (m_element.bubbleOutgoing ? 7 : -7), bounds.bottom() - 4.5);
            tail.closeSubpath();
            path = path.united(tail);
        }
        painter->setPen(Qt::NoPen);
        painter->setBrush(fillColor);
        painter->drawPath(path);
        painter->setPen(QColor(m_element.textColor.isEmpty() ? QStringLiteral("#FFFFFF") : m_element.textColor));
        painter->setBrush(Qt::NoBrush);
        QFont f(m_element.fontFamily.isEmpty() ? QStringLiteral("Segoe UI") : m_element.fontFamily, m_element.fontSize);
        painter->setFont(f);
        QRectF textRect = bounds.adjusted(m_element.bubblePaddingH, m_element.bubblePaddingV,
                                          -m_element.bubblePaddingH, -m_element.bubblePaddingV);
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, m_element.text);
        break;
    }
    case UiElementType::Avatar: {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(m_element.fill.isEmpty() ? QStringLiteral("#3A3A3C") : m_element.fill));
        painter->drawEllipse(QRectF(0, 0, w, h));
        if (!m_element.text.isEmpty()) {
            painter->setPen(QColor(QStringLiteral("#FFFFFF")));
            QFont f(m_element.fontFamily.isEmpty() ? QStringLiteral("Segoe UI") : m_element.fontFamily, m_element.fontSize > 0 ? m_element.fontSize : 12, QFont::DemiBold);
            painter->setFont(f);
            painter->drawText(QRectF(0, 0, w, h), Qt::AlignCenter, m_element.text.left(1).toUpper());
        }
        break;
    }
    case UiElementType::SenderName:
    case UiElementType::Timestamp: {
        painter->setPen(QColor(m_element.textColor.isEmpty() ? QStringLiteral("#8E8E93") : m_element.textColor));
        painter->setBrush(Qt::NoBrush);
        QFont f(m_element.fontFamily.isEmpty() ? QStringLiteral("Segoe UI") : m_element.fontFamily, m_element.fontSize);
        painter->setFont(f);
        painter->drawText(QRectF(0, 0, w, h), Qt::AlignLeft | Qt::AlignVCenter, m_element.text);
        break;
    }
    case UiElementType::Composer: {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(m_element.fill.isEmpty() ? QStringLiteral("#000000") : m_element.fill));
        painter->drawRect(0, 0, w, h);
        break;
    }
    case UiElementType::InputField: {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(m_element.fill.isEmpty() ? QStringLiteral("#1C1C1E") : m_element.fill));
        painter->drawRoundedRect(0, 0, w, h, m_element.cornerRadius, m_element.cornerRadius);
        painter->setPen(QColor(m_element.textColor.isEmpty() ? QStringLiteral("#8E8E93") : m_element.textColor));
        painter->setBrush(Qt::NoBrush);
        QFont f(m_element.fontFamily.isEmpty() ? QStringLiteral("Segoe UI") : m_element.fontFamily, m_element.fontSize);
        painter->setFont(f);
        painter->drawText(QRectF(12, 0, w - 24, h), Qt::AlignVCenter, m_element.text);
        break;
    }
    case UiElementType::Button: {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(m_element.fill.isEmpty() ? QStringLiteral("#0A84FF") : m_element.fill));
        painter->drawRoundedRect(0, 0, w, h, m_element.cornerRadius, m_element.cornerRadius);
        painter->setPen(QColor(m_element.textColor.isEmpty() ? QStringLiteral("#FFFFFF") : m_element.textColor));
        painter->setBrush(Qt::NoBrush);
        QFont f(m_element.fontFamily.isEmpty() ? QStringLiteral("Segoe UI") : m_element.fontFamily, m_element.fontSize);
        painter->setFont(f);
        painter->drawText(QRectF(0, 0, w, h), Qt::AlignCenter, m_element.text);
        break;
    }
    case UiElementType::Icon: {
        painter->setPen(QColor(m_element.textColor.isEmpty() ? QStringLiteral("#0A84FF") : m_element.textColor));
        painter->setBrush(Qt::NoBrush);
        QFont f(QStringLiteral("Segoe UI"), qMin(w, h) * 0.7);
        painter->setFont(f);
        painter->drawText(QRectF(0, 0, w, h), Qt::AlignCenter, m_element.iconType.isEmpty() ? QStringLiteral("+") : m_element.iconType);
        break;
    }
    case UiElementType::Divider: {
        painter->setPen(QPen(QColor(m_element.lineColor.isEmpty() ? QStringLiteral("#38383A") : m_element.lineColor), m_element.lineWidth));
        painter->drawLine(QPointF(0, h/2), QPointF(w, h/2));
        break;
    }
    }
}

void UiElementGraphicsItem::paintSelection(QPainter *painter)
{
    if (!isSelected()) return;
    const double hs = kHandleSize;
    painter->setPen(QPen(QColor(QStringLiteral("#0A84FF")), 1.5, Qt::SolidLine));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(QRectF(-1, -1, m_element.width + 2, m_element.height + 2));
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(QStringLiteral("#0A84FF")));
    const double w = m_element.width;
    const double h = m_element.height;
    painter->drawRect(QRectF(-hs/2, -hs/2, hs, hs));
    painter->drawRect(QRectF(w - hs/2, -hs/2, hs, hs));
    painter->drawRect(QRectF(-hs/2, h - hs/2, hs, hs));
    painter->drawRect(QRectF(w - hs/2, h - hs/2, hs, hs));
}
