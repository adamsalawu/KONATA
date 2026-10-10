#pragma once

#include <QDateTime>
#include <QList>
#include <QString>

#include "core/Character.h"
#include "core/Conversation.h"
#include "core/Scene.h"

namespace Konata {
namespace Core {

// Project-level container for characters, conversations, and scenes.
// No save/load here; serialization is a later milestone.
class Story
{
public:
    Story() = default;
    static Story create(const QString &title,
                        const QString &description = QString());

    const QString &id() const { return m_id; }
    // Persistence support: restores the id assigned when the object was saved.
    void setId(const QString &id) { m_id = id; }
    const QString &title() const { return m_title; }
    void setTitle(const QString &title) { m_title = title; }
    const QString &description() const { return m_description; }
    const QDateTime &createdAt() const { return m_createdAt; }
    // Persistence support: restores the creation time recorded when saved.
    void setCreatedAt(const QDateTime &createdAt) { m_createdAt = createdAt; }

    const QList<Character> &characters() const { return m_characters; }
    const QList<Conversation> &conversations() const { return m_conversations; }
    const QList<Scene> &scenes() const { return m_scenes; }

    void addCharacter(const Character &character);
    void addConversation(const Conversation &conversation);
    void addScene(const Scene &scene);

    // Authoring support (0.1D): replace/remove by stable ID. Update preserves
    // list position; remove of a character also strips it from every
    // conversation's participant list so no dangling ID references remain.
    // Returns false when no object with the given ID exists.
    bool updateCharacter(const Character &character);
    bool removeCharacter(const QString &characterId);
    bool updateConversation(const Conversation &conversation);
    bool removeConversation(const QString &conversationId);
    bool updateScene(const Scene &scene);
    bool removeScene(const QString &sceneId);

    int characterCount() const { return m_characters.size(); }
    int conversationCount() const { return m_conversations.size(); }
    int sceneCount() const { return m_scenes.size(); }

    const Character *findCharacter(const QString &characterId) const;
    const Conversation *findConversation(const QString &conversationId) const;
    const Scene *findScene(const QString &sceneId) const;

private:
    QString m_id;
    QString m_title;
    QString m_description;
    QDateTime m_createdAt;
    QList<Character> m_characters;
    QList<Conversation> m_conversations;
    QList<Scene> m_scenes;
};

} // namespace Core
} // namespace Konata
