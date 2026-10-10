#pragma once

#include <QList>
#include <QString>

namespace Konata {
namespace Core {

// Generic messaging-app UI description (0.2B). Structure and layout intent
// only — never pixels, never rendered output, never app-specific classes.
// Apple Messages / WhatsApp / Discord / custom apps are all future
// configurations of this vocabulary, selected through data, not branches.
// QtCore-only; no QWidget dependency.

// Which kind of screen a definition describes. Custom covers future and
// app-specific screens via ScreenDefinition::customId.
enum class ScreenType {
    ConversationList,
    Chat,
    Profile,
    Custom
};

enum class NavigationStyle {
    Tabs,
    Sidebar,
    Stack,
    None
};

// Light/dark appearance variant. Skins resolve their palette from this;
// components must never hardwire one appearance's colors.
enum class AppearanceMode {
    Dark,
    Light
};

// Reusable visual-component vocabulary. The render model reports which of
// these a screen actually uses; later milestones style them per app skin.
enum class ComponentType {
    Avatar,
    Header,
    ConversationRow,
    MessageBubble,
    ImageMessage,
    Composer,
    Timestamp,
    TypingIndicator,
    NavigationItem,
    ProfileHeader,
    Custom
};

struct ScreenDefinition {
    ScreenType type = ScreenType::Chat;
    QString title;    // e.g. "Chats", "Profile"
    QString customId; // meaningful only when type == Custom
    bool enabled = true;
};

// Position of a message inside a run of same-author messages. Resolved by
// the renderer from Core order + authorship; never stored in Core data.
enum class MessageGroup {
    Single, // isolated message, or grouping disabled
    First,  // top message of a multi-message run
    Middle, // fully enclosed in a run
    Last    // bottom message of a multi-message run
};

// Per-corner bubble radii in logical px, resolved from the appearance
// radius (outer corners) and the grouped radius (inner corners).
struct BubbleCorners {
    int topLeft = 0;
    int topRight = 0;
    int bottomLeft = 0;
    int bottomRight = 0;
};

struct NavigationDefinition {
    NavigationStyle style = NavigationStyle::Tabs;
    QList<ScreenDefinition> screens;

    static NavigationDefinition defaults();
};

struct HeaderDefinition {
    bool visible = true;
    bool showAvatar = true;
    bool showBackButton = true;
};

struct MessageAreaDefinition {
    bool groupConsecutive = true;
    bool showAuthorNames = true;
    bool showTimestamps = false;
    int maxBubbleWidthPercent = 75; // of the message-area width
    // Inner corner radius where grouped bubbles meet (outer corners use the
    // message appearance corner radius). Generic, serializable, logical px.
    int groupedCornerRadius = 4;
};

struct ComposerDefinition {
    bool visible = true;
    QString placeholder;
    bool showAttachButton = true;
};

struct ChatScreenDefinition {
    HeaderDefinition header;
    MessageAreaDefinition messageArea;
    ComposerDefinition composer;
};

struct ConversationListDefinition {
    bool showAvatars = true;
    bool showPreviewText = true;
    bool showUnreadBadges = false;
};

struct ProfileDefinition {
    bool showAvatar = true;
    bool showStatus = true;
};

// Restrained design tokens: logical units consumed by the renderer, so
// visual sizing does not scatter as magic numbers through rendering code.
// Not a CSS engine; colors live in the existing appearance structs.
struct DesignTokens {
    int spacingSmall = 4;
    int spacingMedium = 8;
    int spacingLarge = 16;
    int cornerRadius = 12;
    int avatarSize = 36;
    int fontSizeSmall = 12;
    int fontSizeBody = 14;
    int fontSizeTitle = 16;
};

// Structural grouping for an app's screens. Kept separate from the
// appearance structs: structure says WHAT the app contains, appearance
// says HOW it looks.
struct AppStructure {
    NavigationDefinition navigation = NavigationDefinition::defaults();
    ChatScreenDefinition chat;
    ConversationListDefinition conversationList;
    ProfileDefinition profile;
};

inline NavigationDefinition NavigationDefinition::defaults()
{
    NavigationDefinition navigation;
    ScreenDefinition list;
    list.type = ScreenType::ConversationList;
    list.title = QStringLiteral("Chats");
    ScreenDefinition chat;
    chat.type = ScreenType::Chat;
    chat.title = QStringLiteral("Chat");
    ScreenDefinition profile;
    profile.type = ScreenType::Profile;
    profile.title = QStringLiteral("Profile");
    navigation.screens.append(list);
    navigation.screens.append(chat);
    navigation.screens.append(profile);
    return navigation;
}

} // namespace Core
} // namespace Konata
