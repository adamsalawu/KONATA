#include "rendering/CanvasChatPainter.h"

#include <QPainter>
#include <QPainterPath>

#include "rendering/CanvasLayout.h"
#include "rendering/KonataRenderer.h"
#include "rendering/skins/AppleGeometry.h"
#include "rendering/skins/AppleIcons.h"
#include "rendering/skins/AppleTypography.h"

using namespace Konata;

namespace {

// Every text drawing operation must go through this helper to guarantee
// no brush leak produces a dark rectangle behind the text.
void drawTextClean(QPainter *p, const QRect &rect, int flags, const QString &text)
{
    p->setBrush(Qt::NoBrush);
    p->setBackground(Qt::transparent);
    p->setBackgroundMode(Qt::TransparentMode);
    p->drawText(rect, flags, text);
}

void paintBackground(QPainter *p, const QRect &rect, const QString &colorName)
{
    p->fillRect(rect, QColor(colorName.isEmpty() ? QStringLiteral("#000000") : colorName));
}

void paintHeader(QPainter *p, const QRect &headerRect,
                 const RenderedConversation &conv,
                 const Core::AppDefinition &app)
{
    p->fillRect(headerRect, QColor(app.navigation.barColor.isEmpty() ? QStringLiteral("#000000") : app.navigation.barColor));
    p->setPen(QColor(QStringLiteral("#333333")));
    p->setBrush(Qt::NoBrush);
    p->drawLine(headerRect.bottomLeft(), headerRect.bottomRight());
    const bool isGroup = conv.isGroup;
    const int avatarSize = isGroup ? AppleGeometry::groupHeaderAvatarSize : AppleGeometry::headerAvatarSize;
    const QRect avatarRect(headerRect.center().x() - avatarSize/2, headerRect.top() + 8, avatarSize, avatarSize);
    p->setRenderHint(QPainter::Antialiasing, true);
    p->setPen(Qt::NoPen);
    p->setBrush(QColor(QStringLiteral("#3A3A3C")));
    p->drawEllipse(avatarRect);
    if (!conv.participantNames.isEmpty()) {
        const QString initial = conv.participantNames.first().left(1).toUpper();
        p->setPen(QColor(QStringLiteral("#FFFFFF")));
        p->setFont(AppleTypography::font(14, QFont::DemiBold));
        drawTextClean(p, avatarRect, Qt::AlignCenter, initial);
    }
    const QString title = conv.title.isEmpty() ? QStringLiteral("(untitled)") : conv.title;
    const QColor accent(app.theme.accentColor.isEmpty() ? QStringLiteral("#0A84FF") : app.theme.accentColor);
    p->setPen(accent);
    p->setFont(AppleTypography::font(AppleTypography::titleSize(), QFont::DemiBold));
    const QRect titleRect(headerRect.left(), avatarRect.bottom() + 4, headerRect.width(), 20);
    drawTextClean(p, titleRect, Qt::AlignHCenter | Qt::AlignTop, title);
    AppleIcons::paint(p, AppleIcon::Back, QRectF(headerRect.left()+12, headerRect.top()+20, 20, 20), accent, Qt::transparent);
    if (!isGroup) {
        AppleIcons::paint(p, AppleIcon::Video, QRectF(headerRect.right()-32, headerRect.top()+20, 20, 20), accent, Qt::transparent);
    }
    p->setPen(accent);
    p->setFont(AppleTypography::font(16));
    const QRect chevronRect(titleRect.right()-10, titleRect.top(), 10, 20);
    if (!title.contains(QChar::fromLatin1('>'))) {
        drawTextClean(p, chevronRect, Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("›"));
    }
}

void paintBubble(QPainter *p, const QRect &bubbleRect,
                 const RenderedMessage &msg,
                 const Core::AppDefinition &app)
{
    const QColor fill(msg.bubbleColor.isEmpty() ? QStringLiteral("#2C2C2E") : msg.bubbleColor);
    const int outer = app.message.cornerRadius;
    const int inner = app.structure.chat.messageArea.groupedCornerRadius;
    int tl = outer, tr = outer, bl = outer, br = outer;
    if (msg.group == Core::MessageGroup::First) {
        if (msg.outgoing) br = inner; else bl = inner;
    } else if (msg.group == Core::MessageGroup::Middle) {
        if (msg.outgoing) { tr = inner; br = inner; } else { tl = inner; bl = inner; }
    } else if (msg.group == Core::MessageGroup::Last) {
        if (msg.outgoing) tr = inner; else tl = inner;
    }
    QPainterPath path;
    QRectF bounds = QRectF(bubbleRect).adjusted(0.5,0.5,-0.5,-0.5);
    const bool hasTail = msg.hasTail;
    const qreal tailW = 6;
    if (hasTail) {
        if (msg.outgoing) bounds.adjust(0,0,-tailW,0);
        else bounds.adjust(tailW,0,0,0);
    }
    const qreal tlR = qMin<qreal>(tl, qMin(bounds.width(), bounds.height())/2);
    const qreal trR = qMin<qreal>(tr, qMin(bounds.width(), bounds.height())/2);
    const qreal blR = qMin<qreal>(bl, qMin(bounds.width(), bounds.height())/2);
    const qreal brR = qMin<qreal>(br, qMin(bounds.width(), bounds.height())/2);
    path.moveTo(bounds.left(), bounds.top()+tlR);
    if (tlR>0) path.arcTo(bounds.left(), bounds.top(), 2*tlR, 2*tlR, 180, -90);
    path.lineTo(bounds.right()-trR, bounds.top());
    if (trR>0) path.arcTo(bounds.right()-2*trR, bounds.top(), 2*trR, 2*trR, 90, -90);
    path.lineTo(bounds.right(), bounds.bottom()-brR);
    if (brR>0) path.arcTo(bounds.right()-2*brR, bounds.bottom()-2*brR, 2*brR, 2*brR, 0, -90);
    path.lineTo(bounds.left()+blR, bounds.bottom());
    if (blR>0) path.arcTo(bounds.left(), bounds.bottom()-2*blR, 2*blR, 2*blR, 270, -90);
    path.closeSubpath();
    if (hasTail) {
        QPainterPath tail;
        if (msg.outgoing) {
            tail.moveTo(bounds.right()-7, bounds.bottom()-4.5);
            tail.cubicTo(bounds.right()-1, bounds.bottom()-2.5, bounds.right()+3, bounds.bottom()+1.5, bounds.right()-2, bounds.bottom()-0.5);
            tail.lineTo(bounds.right()-7, bounds.bottom()-4.5);
        } else {
            tail.moveTo(bounds.left()+7, bounds.bottom()-4.5);
            tail.cubicTo(bounds.left()+1, bounds.bottom()-2.5, bounds.left()-3, bounds.bottom()+1.5, bounds.left()+2, bounds.bottom()-0.5);
            tail.lineTo(bounds.left()+7, bounds.bottom()-4.5);
        }
        tail.closeSubpath();
        path = path.united(tail);
    }
    p->setPen(Qt::NoPen);
    p->setBrush(fill);
    p->drawPath(path);
    // Text: explicitly clear brush and set transparent background to prevent
    // any dark rectangle behind the text.
    const QColor textColor(msg.bubbleTextColor.isEmpty() ? QStringLiteral("#FFFFFF") : msg.bubbleTextColor);
    p->setPen(textColor);
    p->setFont(AppleTypography::font(AppleTypography::bodySize()));
    const QRect textRect = bubbleRect.adjusted(10,6,-10,-6);
    const QString displayText = msg.textEmpty ? QStringLiteral("(empty message)") : msg.text;
    drawTextClean(p, textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, displayText);
}

void paintComposer(QPainter *p, const QRect &composerRect, const Core::AppDefinition &app)
{
    p->fillRect(composerRect, QColor(app.navigation.barColor.isEmpty() ? QStringLiteral("#000000") : app.navigation.barColor));
    p->setPen(QColor(QStringLiteral("#333333")));
    p->setBrush(Qt::NoBrush);
    p->drawLine(composerRect.topLeft(), composerRect.topRight());
    const QRect fieldRect = composerRect.adjusted(50, 8, -50, -8);
    p->setPen(Qt::NoPen);
    p->setBrush(QColor(QStringLiteral("#1C1C1E")));
    p->drawRoundedRect(fieldRect, 16, 16);
    p->setPen(QColor(QStringLiteral("#8E8E93")));
    p->setFont(AppleTypography::font(14));
    const QString placeholder = app.structure.chat.composer.placeholder.isEmpty() ? QStringLiteral("iMessage") : app.structure.chat.composer.placeholder;
    drawTextClean(p, fieldRect.adjusted(12,0,-12,0), Qt::AlignVCenter, placeholder);
    const QColor gray(QStringLiteral("#8E8E93"));
    AppleIcons::paint(p, AppleIcon::Add, QRectF(composerRect.left()+12, composerRect.center().y()-11, 22,22), gray, Qt::transparent);
    AppleIcons::paint(p, AppleIcon::Mic, QRectF(composerRect.right()-34, composerRect.center().y()-11, 22,22), gray, Qt::transparent);
}

} // namespace

void CanvasChatPainter::paint(QPainter *painter, const QRect &canvasRect,
                              const Core::Story &story,
                              const Core::AppDefinition &app,
                              const QString &conversationId)
{
    const RenderedConversation conv = KonataRenderer::renderConversation(story, app, conversationId);
    if (!conv.valid) {
        paintBackground(painter, canvasRect, app.chat.backgroundColor);
        painter->setPen(QColor(QStringLiteral("#8E8E93")));
        painter->setFont(AppleTypography::font(14));
        drawTextClean(painter, canvasRect, Qt::AlignCenter, QStringLiteral("No conversation"));
        return;
    }
    const CanvasLayout layout = CanvasLayout::fromSize(canvasRect.size());
    QRect headerRect = layout.headerRect.translated(canvasRect.topLeft());
    QRect messageRect = layout.messageRect.translated(canvasRect.topLeft());
    QRect composerRect = layout.composerRect.translated(canvasRect.topLeft());
    paintBackground(painter, canvasRect, app.chat.backgroundColor);
    paintHeader(painter, headerRect, conv, app);
    int y = messageRect.top();
    const int bubbleMaxW = layout.bubbleMaxWidth;
    for (int i=0; i<conv.rows.size(); ++i) {
        const RenderedMessage &msg = conv.rows.at(i);
        if (msg.startsNewDay) {
            const QString label = msg.dayLabel.isEmpty() ? QStringLiteral("Today") : msg.dayLabel;
            painter->setPen(QColor(QStringLiteral("#8E8E93")));
            painter->setFont(AppleTypography::font(12));
            const QRect pillRect(messageRect.left(), y, messageRect.width(), 18);
            drawTextClean(painter, pillRect, Qt::AlignCenter, label);
            y += 22;
        }
        if (msg.type == Core::MessageType::System) {
            painter->setPen(QColor(QStringLiteral("#8E8E93")));
            painter->setFont(AppleTypography::font(12));
            const QRect sysRect(messageRect.left(), y, messageRect.width(), 16);
            drawTextClean(painter, sysRect, Qt::AlignCenter, msg.text.isEmpty() ? QStringLiteral("Missed call") : msg.text);
            y += 20;
            continue;
        }
        if (msg.showSenderName) {
            painter->setPen(QColor(QStringLiteral("#8E8E93")));
            painter->setFont(AppleTypography::font(12));
            const QRect senderRect(messageRect.left()+36+8, y, messageRect.width()-44, 14);
            drawTextClean(painter, senderRect, Qt::AlignLeft | Qt::AlignVCenter, msg.senderName);
            y += 14;
        }
        const int bubbleW = qMin(bubbleMaxW, 300);
        QFont font = AppleTypography::font(AppleTypography::bodySize());
        QFontMetrics fm(font);
        const QRect textBounds = fm.boundingRect(QRect(0,0,bubbleW-20, 1000), Qt::TextWordWrap, msg.text.isEmpty() ? QStringLiteral("(empty)") : msg.text);
        const int bubbleH = textBounds.height() + 16;
        const int bubbleWActual = qMin(bubbleMaxW, textBounds.width()+24);
        QRect bubbleRect;
        if (msg.outgoing) {
            bubbleRect = QRect(messageRect.right()-bubbleWActual, y, bubbleWActual, bubbleH);
        } else {
            int avatarOffset = msg.showAvatar ? 36+8 : 0;
            bubbleRect = QRect(messageRect.left()+avatarOffset, y, bubbleWActual, bubbleH);
            if (msg.showAvatar) {
                const QRect avatarRect(messageRect.left(), y + bubbleH - 30, 30,30);
                painter->setPen(Qt::NoPen);
                painter->setBrush(QColor(QStringLiteral("#3A3A3C")));
                painter->drawEllipse(avatarRect);
                const QString initial = msg.authorName.left(1).toUpper();
                painter->setPen(QColor(QStringLiteral("#FFFFFF")));
                painter->setFont(AppleTypography::font(12, QFont::DemiBold));
                drawTextClean(painter, avatarRect, Qt::AlignCenter, initial.isEmpty() ? QStringLiteral("?") : initial);
            }
        }
        paintBubble(painter, bubbleRect, msg, app);
        y += bubbleH + 4;
        if (i+1 < conv.rows.size()) {
            const bool continues = msg.group != Core::MessageGroup::Single && msg.group != Core::MessageGroup::Last;
            y += continues ? AppleGeometry::messageSpacingSameAuthor : AppleGeometry::messageSpacingGroups;
        }
        if (y > composerRect.top()) break;
    }
    paintComposer(painter, composerRect, app);
}

QImage CanvasChatPainter::renderToImage(const Core::Story &story,
                                        const Core::AppDefinition &app,
                                        const Core::CanvasSettings &canvas,
                                        const QString &conversationId)
{
    const QSize size = canvas.logicalSize();
    QImage image(size, QImage::Format_ARGB32);
    image.fill(QColor(app.chat.backgroundColor.isEmpty() ? QStringLiteral("#000000") : app.chat.backgroundColor));
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    paint(&painter, QRect(QPoint(0,0), size), story, app, conversationId);
    return image;
}
