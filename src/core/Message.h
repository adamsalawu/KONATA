#pragma once

#include <QDateTime>
#include <QList>
#include <QString>
#include <QtGlobal>

#include "core/Attachment.h"
#include "core/Reaction.h"

namespace Konata {
namespace Core {

enum class MessageType {
    Text,
    Image,
    File,
    System
};

// Ordered chat message. sender/author is a Character id reference
// (empty for System messages). Reactions/attachments are data only.
struct Message {
    QString id;
    QString conversationId; // parent conversation (may be empty until added)
    QString authorId;       // Character::id, empty for System
    MessageType type = MessageType::Text;
    QString text;           // body for Text/System; caption for media
    QDateTime createdAt;
    qint64 order = 0;       // position within the conversation
    QList<Reaction> reactions;
    QList<Attachment> attachments;

    static Message createText(const QString &authorId,
                              const QString &text,
                              qint64 order = 0);
    static Message createMedia(const QString &authorId,
                               MessageType type,
                               const QString &text = QString(),
                               qint64 order = 0);
    static Message createSystem(const QString &text, qint64 order = 0);

    void addReaction(const Reaction &reaction);
    void addAttachment(const Attachment &attachment);
};

} // namespace Core
} // namespace Konata
