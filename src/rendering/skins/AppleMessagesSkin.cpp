#include "rendering/skins/AppleMessagesSkin.h"

#include "rendering/skins/AppleGeometry.h"
#include "rendering/skins/ApplePalette.h"
#include "rendering/skins/AppleTypography.h"

using namespace Konata::Core;

AppDefinition AppleMessagesSkin::createAppDefinition(AppearanceMode mode)
{
    const ApplePalette palette =
        (mode == AppearanceMode::Light) ? ApplePalette::light() : ApplePalette::dark();

    AppDefinition app = AppDefinition::defaults(QStringLiteral("Apple Messages"));
    app.packageId = QStringLiteral("com.apple.MobileSMS");

    // Every color resolves from the centralized palette — never literals here.
    app.theme.themeName = (mode == AppearanceMode::Light)
        ? QStringLiteral("Apple Messages Light")
        : QStringLiteral("Apple Messages Dark");
    app.theme.accentColor = palette.accent;
    app.theme.darkMode = (mode == AppearanceMode::Dark);

    app.navigation.barColor = palette.background;
    app.navigation.activeTint = palette.accent;
    app.navigation.inactiveTint = palette.secondaryText;
    app.navigation.barHeight = 56;

    app.chat.backgroundColor = palette.background;
    app.chat.datePillColor = palette.incomingBubble;
    app.chat.fontSize = AppleTypography::bodySize();

    app.message.incomingBubbleColor = palette.incomingBubble;
    app.message.outgoingBubbleColor = palette.outgoingBubble;
    app.message.incomingTextColor = palette.incomingText;
    app.message.outgoingTextColor = palette.outgoingText;
    app.message.cornerRadius = AppleGeometry::bubbleRadiusLarge;
    app.message.fontSize = AppleTypography::bodySize();

    app.notification.bannerColor = palette.secondaryBackground;

    // Generic structure: Stack navigation, full chat surface.
    app.structure.navigation.style = NavigationStyle::Stack;
    ScreenDefinition list;
    list.type = ScreenType::ConversationList;
    list.title = QStringLiteral("Messages");
    ScreenDefinition chat;
    chat.type = ScreenType::Chat;
    chat.title = QStringLiteral("Chat");
    ScreenDefinition profile;
    profile.type = ScreenType::Profile;
    profile.title = QStringLiteral("Contact");
    app.structure.navigation.screens.clear();
    app.structure.navigation.screens.append(list);
    app.structure.navigation.screens.append(chat);
    app.structure.navigation.screens.append(profile);

    app.structure.chat.header.visible = true;
    app.structure.chat.header.showAvatar = true;
    app.structure.chat.header.showBackButton = true;
    app.structure.chat.messageArea.groupConsecutive = true;
    app.structure.chat.messageArea.showAuthorNames = false;
    app.structure.chat.messageArea.showTimestamps = false;
    app.structure.chat.messageArea.maxBubbleWidthPercent = AppleGeometry::bubbleMaxWidthPercent;
    app.structure.chat.messageArea.groupedCornerRadius = AppleGeometry::bubbleRadiusSmall;
    app.structure.chat.composer.visible = true;
    app.structure.chat.composer.placeholder = QStringLiteral("iMessage");
    app.structure.chat.composer.showAttachButton = true;

    app.structure.conversationList.showAvatars = true;
    app.structure.conversationList.showPreviewText = true;
    app.structure.profile.showAvatar = true;
    app.structure.profile.showStatus = true;

    // Tokens mirror the geometry/typography spec: single source of truth.
    app.tokens.spacingSmall = 4;
    app.tokens.spacingMedium = 8;
    app.tokens.spacingLarge = 16;
    app.tokens.cornerRadius = AppleGeometry::bubbleRadiusLarge;
    app.tokens.avatarSize = AppleGeometry::chatAvatarSize;
    app.tokens.fontSizeSmall = AppleTypography::metadataSize();
    app.tokens.fontSizeBody = AppleTypography::bodySize();
    app.tokens.fontSizeTitle = AppleTypography::titleSize();

    return app;
}

AppDefinition AppleMessagesSkin::createNeutralAppDefinition(const QString &appName)
{
    // Stock defaults() is the neutral skin: generic structure, KONATA tokens.
    return AppDefinition::defaults(appName);
}
