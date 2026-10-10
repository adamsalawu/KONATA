#pragma once

#include <QString>

namespace Konata {
namespace Core {

enum class SceneEventType {
    SendMessage,
    Wait,
    StartTyping,
    StopTyping,
    Notification
};

// One step in a scene timeline. References (character/message/
// conversation ids) are data only; the timeline player comes later.
struct SceneEvent {
    QString id;
    SceneEventType type = SceneEventType::SendMessage;
    QString characterId;    // actor for SendMessage/Typing events
    QString messageId;      // payload for SendMessage
    QString conversationId; // target conversation
    int waitMs = 0;         // payload for Wait
    QString text;           // payload for Notification

    static SceneEvent sendMessage(const QString &conversationId,
                                  const QString &messageId);
    static SceneEvent wait(int waitMs);
    static SceneEvent typing(const QString &characterId,
                             const QString &conversationId,
                             bool start);
    static SceneEvent notification(const QString &text);
};

} // namespace Core
} // namespace Konata
