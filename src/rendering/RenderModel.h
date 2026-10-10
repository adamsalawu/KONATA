#pragma once

#include <QList>
#include <QString>
#include <QStringList>
#include <QtGlobal>

#include "core/AppUi.h"
#include "core/Message.h"

// 0.2A render model: plain data describing what to show for a conversation.
// QtCore-only. Produced by KonataRenderer from read-only Core data and
// materialized into widgets by PreviewArea. Holds IDs for traceability but
// never Core pointers.
struct RenderedMessage {
    QString messageId;
    QString authorId;
    QString authorName;   // resolved display name, or a safe fallback
    bool authorMissing = false;
    // Resolved Character.avatarRef for the author. Empty for System messages,
    // unknown authors, and authors without an avatar. Reference only — the
    // view decides how (and whether) to load it.
    QString avatarRef;
    bool hasAvatar = false;
    QString text;         // as authored (may be empty)
    bool textEmpty = false;
    // Presentation direction (0.2C.1): the conversation's first participant
    // renders as outgoing (self), everyone else as incoming. A documented
    // data-driven convention — no app-name branching, no Core changes.
    bool outgoing = false;
    // Bubble presentation resolved by the renderer from the AppDefinition's
    // message appearance (incoming vs outgoing by direction). The view
    // applies these verbatim; changing the definition changes the model.
    QString bubbleColor;
    QString bubbleTextColor;
    int cornerRadius = 0;
    Konata::Core::MessageType type = Konata::Core::MessageType::Text;
    // Non-text types carry a reference note instead of rendered content:
    // file name when an attachment exists, otherwise a generic fallback.
    QString attachmentNote;
    bool isFallbackType = false; // Image/File: no asset pipeline in 0.2A    // Image rows: reference + normalized anchor (0..1) from the first
    // attachment, or deterministic center when absent (pre-0.2A.1 data).
    QString imageRef;
    double imageAnchorX = 0.5;
    double imageAnchorY = 0.5;
    qint64 order = 0;
    // Group chat specifics: sender name shown above incoming bubbles when
    // the conversation has >2 participants and showAuthorNames enabled.
    QString senderName;
    bool showSenderName = false;
    bool showAvatar = false;
    bool hasTail = true;
    // Grouping position within a run of same-author messages (0.2C.2).
    // System messages and isolated messages are Single.
    Konata::Core::MessageGroup group = Konata::Core::MessageGroup::Single;
    // Resolved per-corner bubble radii (logical px): outer corners use the
    // message appearance radius, grouped inner corners the grouped radius.
    Konata::Core::BubbleCorners corners;
    // Day separator: true when this message starts a new calendar day
    // relative to the previous rendered message. Static presentation data.
    bool startsNewDay = false;
    QString dayLabel;
    // Static timestamp text (empty unless the definition shows timestamps).
    // No live behavior; delivery/read state is NOT IMPLEMENTED (no Core).
    QString timeText;
};

struct RenderedConversation {
    bool valid = false; // false when the conversation ID resolves to nothing
    QString conversationId;
    QString title;
    QStringList participantNames; // display names, "(unknown)" for orphans
    QList<RenderedMessage> rows;  // Core message order, never re-sorted
    bool empty = true;
    // Message-area presentation resolved from the AppDefinition.
    bool showAuthorNames = true;
    int maxBubbleWidthPercent = 75; // of the message-area width
    bool isGroup = false;
    // Header group avatars (first N participant avatars)
    QStringList groupAvatarRefs;
    QList<bool> groupAvatarHasAvatar;
};

// Conversation-list screen model (0.2C.2): one row per conversation from
// real Story data. hasUnread is structurally present but resolves false:
// Core carries no unread state and the renderer never fakes it.
struct RenderedConversationRow {
    QString conversationId;
    QString title;
    QString avatarRef;
    bool hasAvatar = false;
    QString previewText;
    QString timestampText;
    bool hasUnread = false;
    int messageCount = 0;
};

struct RenderedConversationList {
    bool valid = false;
    QString title;
    bool showAvatars = true;
    bool showPreviewText = true;
    QList<RenderedConversationRow> rows;
    bool empty = true;
};

// Contact/profile screen model (0.2C.2): visual presentation of one
// Character. Action rows are structural labels only — no behavior.
struct RenderedProfile {
    bool valid = false;
    QString characterId;
    QString displayName;
    QString username;
    QString status;
    QString avatarRef;
    bool hasAvatar = false;
    QStringList actions;
};
// Generic screen representation (0.2B). Answers "what screen am I rendering"
// through data: the type comes from the resolved context, never from an
// application-name branch. Custom screens carry their definition's customId.
struct RenderedScreen {
    Konata::Core::ScreenType type = Konata::Core::ScreenType::ConversationList;
    QString screenId; // customId when type == Custom, otherwise empty
    QString title;
    // Component vocabulary actually used by this screen, derived from the
    // payload (e.g. ImageMessage only when an image row exists).
    QList<Konata::Core::ComponentType> components;
};

// Presentation model: Story + AppDefinition resolved into one deterministic,
// read-only structure. Holds IDs and display strings, never Core pointers.
struct RenderedAppView {
    bool valid = false;
    QString appName;
    RenderedScreen screen;
    // Populated only for the Chat screen with a resolvable conversation.
    RenderedConversation conversation;
    Konata::Core::DesignTokens tokens;
    // Composer presentation resolved from the chat definition.
    QString composerPlaceholder;
};
