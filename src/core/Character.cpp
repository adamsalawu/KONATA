#include "core/Character.h"

#include "core/Id.h"

namespace Konata {
namespace Core {

Character Character::create(const QString &displayName,
                            const QString &username,
                            const QString &avatarRef,
                            const QString &status,
                            Presence presence)
{
    Character c;
    c.id = IdGenerator::create(QStringLiteral("char"));
    c.displayName = displayName;
    c.username = username;
    c.avatarRef = avatarRef;
    c.status = status;
    c.presence = presence;
    return c;
}

} // namespace Core
} // namespace Konata
