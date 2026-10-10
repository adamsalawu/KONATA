#pragma once

#include <QString>

namespace Konata {
namespace Core {

// Centralized session/project ID generator.
// Simple monotonic counter per process, prefixed per object kind
// (e.g. "char_000001"). Unique within a session. After loading a
// persisted project, noteLoadedId() advances the counter past loaded
// IDs so newly created objects cannot collide with them.
class IdGenerator
{
public:
    static QString create(const QString &prefix);
    static void resetForTests();
    static void noteLoadedId(const QString &id);

private:
    IdGenerator() = delete;
};

} // namespace Core
} // namespace Konata
