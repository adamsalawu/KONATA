#include "core/Story.h"

#include "core/Id.h"

namespace Konata {
namespace Core {

Story Story::create(const QString &title, const QString &description)
{
    Story s;
    s.m_id = IdGenerator::create(QStringLiteral("story"));
    s.m_title = title;
    s.m_description = description;
    s.m_createdAt = QDateTime::currentDateTimeUtc();
    return s;
}

void Story::addCharacter(const Character &character)
{
    m_characters.append(character);
}

void Story::addConversation(const Conversation &conversation)
{
    m_conversations.append(conversation);
}

void Story::addScene(const Scene &scene)
{
    m_scenes.append(scene);
}

bool Story::updateCharacter(const Character &character)
{
    for (Character &c : m_characters) {
        if (c.id == character.id) {
            c = character;
            return true;
        }
    }
    return false;
}

bool Story::removeCharacter(const QString &characterId)
{
    if (characterId.isEmpty())
        return false;
    bool removed = false;
    for (int i = m_characters.size() - 1; i >= 0; --i) {
        if (m_characters.at(i).id == characterId) {
            m_characters.removeAt(i);
            removed = true;
        }
    }
    if (removed) {
        for (Conversation &c : m_conversations)
            c.removeParticipant(characterId);
    }
    return removed;
}

bool Story::updateConversation(const Conversation &conversation)
{
    for (Conversation &c : m_conversations) {
        if (c.id() == conversation.id()) {
            c = conversation;
            return true;
        }
    }
    return false;
}

bool Story::removeConversation(const QString &conversationId)
{
    if (conversationId.isEmpty())
        return false;
    for (int i = m_conversations.size() - 1; i >= 0; --i) {
        if (m_conversations.at(i).id() == conversationId) {
            m_conversations.removeAt(i);
            return true;
        }
    }
    return false;
}

bool Story::updateScene(const Scene &scene)
{
    for (Scene &s : m_scenes) {
        if (s.id() == scene.id()) {
            s = scene;
            return true;
        }
    }
    return false;
}

bool Story::removeScene(const QString &sceneId)
{
    if (sceneId.isEmpty())
        return false;
    for (int i = m_scenes.size() - 1; i >= 0; --i) {
        if (m_scenes.at(i).id() == sceneId) {
            m_scenes.removeAt(i);
            return true;
        }
    }
    return false;
}

const Character *Story::findCharacter(const QString &characterId) const
{
    for (const Character &c : m_characters) {
        if (c.id == characterId)
            return &c;
    }
    return nullptr;
}

const Conversation *Story::findConversation(const QString &conversationId) const
{
    for (const Conversation &c : m_conversations) {
        if (c.id() == conversationId)
            return &c;
    }
    return nullptr;
}

const Scene *Story::findScene(const QString &sceneId) const
{
    for (const Scene &s : m_scenes) {
        if (s.id() == sceneId)
            return &s;
    }
    return nullptr;
}

} // namespace Core
} // namespace Konata
