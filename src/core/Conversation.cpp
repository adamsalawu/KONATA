#include "core/Conversation.h"

#include "core/Id.h"

namespace Konata {
namespace Core {

Conversation Conversation::create(const QString &title, ConversationType type)
{
    Conversation c;
    c.m_id = IdGenerator::create(QStringLiteral("conv"));
    c.m_title = title;
    c.m_type = type;
    return c;
}

void Conversation::addParticipant(const QString &characterId)
{
    if (characterId.isEmpty() || m_participantIds.contains(characterId))
        return;
    m_participantIds.append(characterId);
}

bool Conversation::hasParticipant(const QString &characterId) const
{
    return m_participantIds.contains(characterId);
}

void Conversation::setParticipantIds(const QStringList &characterIds)
{
    m_participantIds.clear();
    for (const QString &id : characterIds)
        addParticipant(id);
}

bool Conversation::removeParticipant(const QString &characterId)
{
    return m_participantIds.removeOne(characterId);
}

void Conversation::addMessage(const Message &message)
{
    Message copy = message;
    if (copy.conversationId.isEmpty())
        copy.conversationId = m_id;
    if (copy.order == 0)
        copy.order = static_cast<qint64>(m_messages.size()) + 1;
    m_messages.append(copy);
}

const Message *Conversation::findMessage(const QString &messageId) const
{
    for (const Message &m : m_messages) {
        if (m.id == messageId)
            return &m;
    }
    return nullptr;
}

bool Conversation::updateMessage(const Message &message)
{
    for (Message &m : m_messages) {
        if (m.id == message.id) {
            m = message;
            return true;
        }
    }
    return false;
}

bool Conversation::removeMessage(const QString &messageId)
{
    if (messageId.isEmpty())
        return false;
    for (int i = m_messages.size() - 1; i >= 0; --i) {
        if (m_messages.at(i).id == messageId) {
            m_messages.removeAt(i);
            return true;
        }
    }
    return false;
}

bool Conversation::moveMessage(const QString &messageId, int delta)
{
    if (messageId.isEmpty() || (delta != -1 && delta != 1))
        return false;
    for (int i = 0; i < m_messages.size(); ++i) {
        if (m_messages.at(i).id == messageId) {
            const int j = i + delta;
            if (j < 0 || j >= m_messages.size())
                return false;
            m_messages.swapItemsAt(i, j);
            const qint64 order = m_messages[i].order;
            m_messages[i].order = m_messages[j].order;
            m_messages[j].order = order;
            return true;
        }
    }
    return false;
}

Message Conversation::duplicateMessage(const QString &messageId)
{
    for (int i = 0; i < m_messages.size(); ++i) {
        if (m_messages.at(i).id == messageId) {
            Message copy = m_messages.at(i);
            copy.id = IdGenerator::create(QStringLiteral("msg"));
            copy.conversationId = m_id;
            qint64 maxOrder = 0;
            for (const Message &m : m_messages)
                maxOrder = qMax(maxOrder, m.order);
            copy.order = maxOrder + 1;
            copy.createdAt = QDateTime::currentDateTimeUtc();
            m_messages.insert(i + 1, copy);
            return copy;
        }
    }
    return Message();
}

} // namespace Core
} // namespace Konata
