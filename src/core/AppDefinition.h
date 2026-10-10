#pragma once

#include <QString>

#include "core/AppUi.h"

namespace Konata {
namespace Core {

// Visual building blocks of the fictional messaging app.
// Data only; no theme editor or rendering in Core.
struct NavigationAppearance {
    QString barColor = QStringLiteral("#2d2d30");
    QString activeTint = QStringLiteral("#d4d4d4");
    QString inactiveTint = QStringLiteral("#858585");
    int barHeight = 56;
};

struct ChatAppearance {
    QString backgroundColor = QStringLiteral("#1e1e1e");
    QString datePillColor = QStringLiteral("#2d2d30");
    int fontSize = 14;
};

struct MessageAppearance {
    QString incomingBubbleColor = QStringLiteral("#2d2d30");
    QString outgoingBubbleColor = QStringLiteral("#094771");
    QString incomingTextColor = QStringLiteral("#d4d4d4");
    QString outgoingTextColor = QStringLiteral("#ffffff");
    int cornerRadius = 12;
    int fontSize = 14;
};

struct NotificationAppearance {
    bool enabled = true;
    QString soundRef;
    QString bannerColor = QStringLiteral("#252526");
};

struct ThemeInfo {
    QString themeName = QStringLiteral("Konata Dark");
    QString accentColor = QStringLiteral("#4a90a4");
    bool darkMode = true;
};

// Fictional messaging application being recreated.
struct AppDefinition {
    QString id;
    QString appName;
    QString packageId; // e.g. reverse-DNS style identifier
    ThemeInfo theme;
    NavigationAppearance navigation;
    ChatAppearance chat;
    MessageAppearance message;
    NotificationAppearance notification;
    // 0.2B: generic App UI architecture. The appearance structs above keep
    // their exact JSON shape; structure + tokens are additive and optional
    // on load so older .konata files keep working.
    AppStructure structure;
    DesignTokens tokens;

    static AppDefinition defaults(const QString &appName = QStringLiteral("Konata Chat"));
};

} // namespace Core
} // namespace Konata
