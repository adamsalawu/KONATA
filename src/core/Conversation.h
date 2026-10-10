#pragma once

#include <QList>
#include <QString>
#include <QStringList>

#include "core/Message.h"

namespace Konata {
namespace Core {

enum class ConversationType {
    Private,
    Group
};

// Chat conversation. Participants are Character id references;
// messages are kept in insertion (chronological) order.
class Conversation
{
public:
    Conversation() = default;
    static Conversation create(const QString &title, ConversationType type);

    const QString &id() const { return m_id; }
    // Persistence support: restores the id assigned when the object was saved.
    void setId(const QString &id) { m_id = id; }
    const QString &title() const { return m_title; }
    void setTitle(const QString &title) { m_title = title; }

    ConversationType type() const { return m_type; }
    void setType(ConversationType type) { m_type = type; }

    const QStringList &participantIds() const { return m_participantIds; }
    void addParticipant(const QString &characterId);
    bool hasParticipant(const QString &characterId) const;
    // Authoring support (0.1D): participant list maintenance by Character ID.
    void setParticipantIds(const QStringList &characterIds);
    bool removeParticipant(const QString &characterId);

    const QList<Message> &messages() const { return m_messages; }
    void addMessage(const Message &message);
    int messageCount() const { return m_messages.size(); }
    const Message *findMessage(const QString &messageId) const;
    // Authoring support (0.1E): replace/remove by stable ID. Update preserves
    // list position and the persisted order value; remove keeps the remaining
    // orders stable (no renumbering). Returns false for unknown IDs.
    bool updateMessage(const Message &message);
    bool removeMessage(const QString &messageId);
    // Direct chat editing: move swaps adjacent messages (positions and order
    // values travel together, IDs are untouched); duplicate inserts a fresh-ID
    // copy right after the original with max+1 order. False at bounds/unknown.
    bool moveMessage(const QString &messageId, int delta);
    Message duplicateMessage(const QString &messageId);

private:
    QString m_id;
    QString m_title;
    ConversationType m_type = ConversationType::Private;
    QStringList m_participantIds;
    QList<Message> m_messages;
};

} // namespace Core
} // namespace Konata
