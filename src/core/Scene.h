#pragma once

#include <QList>
#include <QString>

#include "core/SceneEvent.h"

namespace Konata {
namespace Core {

// Ordered sequence of scene events. No playback logic in Core.
class Scene
{
public:
    Scene() = default;
    static Scene create(const QString &name,
                        const QString &description = QString());

    const QString &id() const { return m_id; }
    // Persistence support: restores the id assigned when the object was saved.
    void setId(const QString &id) { m_id = id; }
    const QString &name() const { return m_name; }
    void setName(const QString &name) { m_name = name; }
    const QString &description() const { return m_description; }
    void setDescription(const QString &description) { m_description = description; }

    const QList<SceneEvent> &events() const { return m_events; }
    void addEvent(const SceneEvent &event);
    int eventCount() const { return m_events.size(); }
    const SceneEvent *findEvent(const QString &eventId) const;
    // Authoring support (0.1E): replace/remove by stable ID. Update preserves
    // list position. Returns false for unknown IDs.
    bool updateEvent(const SceneEvent &event);
    bool removeEvent(const QString &eventId);

private:
    QString m_id;
    QString m_name;
    QString m_description;
    QList<SceneEvent> m_events;
};

} // namespace Core
} // namespace Konata
