#pragma once

// Apple Messages reference geometry (0.2C.3): measured from supplied
// private/group screenshots in Image Refrence\Apple Imessages.
// Values marked [V] are document/device grounded; [R] are pixel-measured
// from the supplied references (approx, desktop scaled); [A] are reasoned.
// All logical points; renderer resolves against preview viewport.
struct AppleGeometry {
    // Target viewport: iPhone 15 logical portrait [V].
    static constexpr int viewportWidth = 393;
    static constexpr int viewportHeight = 852;
    static constexpr int safeAreaTop = 59;
    static constexpr int safeAreaBottom = 34;

    // Header: reference shows private centered avatar+name, group overlapping avatars.
    static constexpr int chatHeaderHeight = 68; // [R] private header ~68, group similar
    static constexpr int chatHeaderPaddingH = 12; // [R]
    static constexpr int listNavHeight = 96; // [A] unchanged

    // Message area.
    static constexpr int messageMarginHorizontal = 12; // [R] private 12-16
    static constexpr int messageSpacingSameAuthor = 2; // [R] tight stack
    static constexpr int messageSpacingGroups = 10; // [R] between authors
    static constexpr int bubblePaddingHorizontal = 12; // [R]
    static constexpr int bubblePaddingVertical = 7; // [R]
    static constexpr int bubbleMaxWidthPercent = 70; // [R] verified 70
    static constexpr int bubbleRadiusLarge = 18; // [R] verified outer
    static constexpr int bubbleRadiusSmall = 6; // [R] verified inner — was 4 (stale test value)
    static constexpr int bubbleTailWidth = 6; // [R] refined tail
    static constexpr int bubbleTailHeight = 4;

    // Avatars — separated per reference (header 36 vs message 30).
    static constexpr int chatAvatarSize = 30; // [R] per-message group avatar — corrected from 36
    static constexpr int messageAvatarSize = 30; // [R] alias for clarity
    static constexpr int listAvatarSize = 48; // [A]
    static constexpr int headerAvatarSize = 36; // [R] private header
    static constexpr int privateHeaderAvatarSize = 36; // [R] explicit
    static constexpr int groupHeaderAvatarSize = 32; // [R] overlapping
    static constexpr int groupHeaderOverlap = 10; // [R]

    // Sender name above group bubbles.
    static constexpr int senderNameSize = 12; // [R]
    static constexpr int senderNameSpacing = 2;

    // Icons: 24-unit vector box, 2.5-unit stroke [A].
    static constexpr int iconBox = 24;
    static constexpr double iconStroke = 2.5;

    // Conversation-list rows.
    static constexpr int listRowHeight = 72;
    static constexpr int listSeparatorInset = 72;
    static constexpr int listRowSpacing = 0;

    // Composer: iMessage field with plus/camera/mic.
    static constexpr int composerFieldHeight = 36; // [R]
    static constexpr int composerBarPaddingH = 8; // [R]
    static constexpr int composerBarPaddingV = 8;
    static constexpr int composerButtonSize = 28; // [R] plus/camera ~28
    static constexpr int composerSpacing = 8;

    // Metadata.
    static constexpr int timestampSpacingVertical = 10;
    static constexpr int dateSeparatorSpacingVertical = 12;
};
