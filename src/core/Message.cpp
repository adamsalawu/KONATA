#include "core/Message.h"

#include <QDateTime>

#include "core/Id.h"

namespace Konata {
namespace Core {

Message Message::createText(const QString &authorId, const QString &text, qint64 order)
{
    Message m;
    m.id = IdGenerator::create(QStringLiteral("msg"));
    m.authorId = authorId;
    m.type = MessageType::Text;
    m.text = text;
    m.createdAt = QDateTime::currentDateTimeUtc();
    m.order = order;
    return m;
}

Message Message::createMedia(const QString &authorId, MessageType type,
                             const QString &text, qint64 order)
{
    Message m;
    m.id = IdGenerator::create(QStringLiteral("msg"));
    m.authorId = authorId;
    m.type = type;
    m.text = text;
    m.createdAt = QDateTime::currentDateTimeUtc();
    m.order = order;
    return m;
}

Message Message::createSystem(const QString &text, qint64 order)
{
    Message m;
    m.id = IdGenerator::create(QStringLiteral("msg"));
    m.type = MessageType::System;
    m.text = text;
    m.createdAt = QDateTime::currentDateTimeUtc();
    m.order = order;
    return m;
}

void Message::addReaction(const Reaction &reaction)
{
    reactions.append(reaction);
}

void Message::addAttachment(const Attachment &attachment)
{
    attachments.append(attachment);
}

} // namespace Core
} // namespace Konata
