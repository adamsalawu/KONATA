#pragma once

#include <QDateTime>
#include <QString>

namespace Konata {
namespace Core {

// One emoji reaction on a message, attributed to a character.
// A message holds a QList<Reaction>; no UI lives here.
struct Reaction {
    QString emoji;        // e.g. "👍"
    QString characterId;  // Character::id of the reactor
    QDateTime createdAt;
};

} // namespace Core
} // namespace Konata
