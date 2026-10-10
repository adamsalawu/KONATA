#pragma once

#include <QString>

namespace Konata {
namespace Core {

enum class Presence {
    Offline,
    Online,
    Away,
    Busy
};

// Fictional chat character. avatarRef is data only (file path, URI,
// or resource key). No rendering lives in Core.
struct Character {
    QString id;
    QString displayName;
    QString username;   // handle without '@'
    QString avatarRef;  // data/reference only
    QString status;     // free-text status line
    Presence presence = Presence::Offline;

    static Character create(const QString &displayName,
                            const QString &username,
                            const QString &avatarRef = QString(),
                            const QString &status = QString(),
                            Presence presence = Presence::Offline);
};

} // namespace Core
} // namespace Konata
