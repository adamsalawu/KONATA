#include "core/Scene.h"

#include "core/Id.h"

namespace Konata {
namespace Core {

SceneEvent SceneEvent::sendMessage(const QString &conversationId, const QString &messageId)
{
    SceneEvent e;
    e.id = IdGenerator::create(QStringLiteral("evt"));
    e.type = SceneEventType::SendMessage;
    e.conversationId = conversationId;
    e.messageId = messageId;
    return e;
}

SceneEvent SceneEvent::wait(int waitMs)
{
    SceneEvent e;
    e.id = IdGenerator::create(QStringLiteral("evt"));
    e.type = SceneEventType::Wait;
    e.waitMs = waitMs;
    return e;
}

SceneEvent SceneEvent::typing(const QString &characterId,
                              const QString &conversationId, bool start)
{
    SceneEvent e;
    e.id = IdGenerator::create(QStringLiteral("evt"));
    e.type = start ? SceneEventType::StartTyping : SceneEventType::StopTyping;
    e.characterId = characterId;
    e.conversationId = conversationId;
    return e;
}

SceneEvent SceneEvent::notification(const QString &text)
{
    SceneEvent e;
    e.id = IdGenerator::create(QStringLiteral("evt"));
    e.type = SceneEventType::Notification;
    e.text = text;
    return e;
}

Scene Scene::create(const QString &name, const QString &description)
{
    Scene s;
    s.m_id = IdGenerator::create(QStringLiteral("scene"));
    s.m_name = name;
    s.m_description = description;
    return s;
}

void Scene::addEvent(const SceneEvent &event)
{
    m_events.append(event);
}

const SceneEvent *Scene::findEvent(const QString &eventId) const
{
    for (const SceneEvent &e : m_events) {
        if (e.id == eventId)
            return &e;
    }
    return nullptr;
}

bool Scene::updateEvent(const SceneEvent &event)
{
    for (SceneEvent &e : m_events) {
        if (e.id == event.id) {
            e = event;
            return true;
        }
    }
    return false;
}

bool Scene::removeEvent(const QString &eventId)
{
    if (eventId.isEmpty())
        return false;
    for (int i = m_events.size() - 1; i >= 0; --i) {
        if (m_events.at(i).id == eventId) {
            m_events.removeAt(i);
            return true;
        }
    }
    return false;
}

} // namespace Core
} // namespace Konata
