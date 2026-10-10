#include "rendering/KonataRenderer.h"

#include <QDate>
#include <QLocale>

using namespace Konata::Core;

namespace {
QString characterLabel(const Character &c)
{
    if (!c.displayName.isEmpty())
        return c.displayName;
    if (!c.username.isEmpty())
        return QStringLiteral("@") + c.username;
    return QStringLiteral("(unnamed)");
}

// Groupable messages share an author and are not System rows.
bool isGroupable(const Message &message)
{
    return message.type != MessageType::System && !message.authorId.isEmpty();
}

QString previewSnippet(const QString &text)
{
    const QString first = text.split(QChar::fromLatin1('\n')).first().left(64);
    return first.isEmpty() ? QStringLiteral("(No messages)") : first;
}
} // namespace

RenderedConversation KonataRenderer::renderConversation(const Story &story,
                                                        const QString &conversationId)
{
    return renderConversation(story, AppDefinition::defaults(), conversationId);
}

RenderedConversation KonataRenderer::renderConversation(const Story &story,
                                                        const AppDefinition &app,
                                                        const QString &conversationId)
{
    RenderedConversation result;
    if (conversationId.isEmpty())
        return result;
    const Conversation *conversation = story.findConversation(conversationId);
    if (!conversation)
        return result;

    result.valid = true;
    result.conversationId = conversation->id();
    result.title = conversation->title().isEmpty() ? QStringLiteral("(untitled conversation)")
                                                   : conversation->title();
    result.showAuthorNames = app.structure.chat.messageArea.showAuthorNames;
    result.maxBubbleWidthPercent = app.structure.chat.messageArea.maxBubbleWidthPercent;
    result.isGroup = conversation->type() == ConversationType::Group;
    for (const QString &participantId : conversation->participantIds()) {
        const Character *c = story.findCharacter(participantId);
        result.participantNames.append(c ? characterLabel(*c) : QStringLiteral("(unknown)"));
        // Header group avatars: first 4 participants
        if (result.groupAvatarRefs.size() < 4) {
            if (c) {
                result.groupAvatarRefs.append(c->avatarRef);
                result.groupAvatarHasAvatar.append(!c->avatarRef.isEmpty());
            } else {
                result.groupAvatarRefs.append(QString());
                result.groupAvatarHasAvatar.append(false);
            }
        }
    }

    for (const Message &message : conversation->messages()) {
        RenderedMessage row;
        row.messageId = message.id;
        row.authorId = message.authorId;
        row.authorName = authorDisplayName(story, message, &row.authorMissing);
        if (message.type != MessageType::System && !message.authorId.isEmpty()
            && !conversation->participantIds().isEmpty()
            && message.authorId == conversation->participantIds().first())
            row.outgoing = true;
        if (!row.authorMissing && message.type != MessageType::System) {
            if (const Character *author = story.findCharacter(message.authorId)) {
                row.avatarRef = author->avatarRef;
                row.hasAvatar = !author->avatarRef.isEmpty();
            }
        }
        row.text = message.text;
        row.textEmpty = message.text.isEmpty();
        row.type = message.type;
        row.order = message.order;
        if (app.structure.chat.messageArea.showTimestamps)
            row.timeText = shortTime(message.createdAt);
        // Bubble presentation from the definition's message appearance,
        // selected by direction. Centralized colors in, no renderer literals.
        if (row.outgoing) {
            row.bubbleColor = app.message.outgoingBubbleColor;
            row.bubbleTextColor = app.message.outgoingTextColor;
        } else {
            row.bubbleColor = app.message.incomingBubbleColor;
            row.bubbleTextColor = app.message.incomingTextColor;
        }
        row.cornerRadius = app.message.cornerRadius;
        if (message.type == MessageType::Image || message.type == MessageType::File) {
            row.isFallbackType = true;
            if (!message.attachments.isEmpty() && !message.attachments.first().fileName.isEmpty())
                row.attachmentNote = message.attachments.first().fileName;
            else if (!message.attachments.isEmpty()
                     && !message.attachments.first().fileRef.isEmpty())
                row.attachmentNote = message.attachments.first().fileRef;
            else
                row.attachmentNote = QString();
        }
        if (message.type == MessageType::Image && !message.attachments.isEmpty()) {
            const Attachment &attachment = message.attachments.first();
            row.imageRef = attachment.fileRef;
            // Clamped defensively: the model must never carry NaN/out-of-range
            // anchors regardless of what was persisted or authored.
            row.imageAnchorX = Attachment::clampAnchor(attachment.anchorX);
            row.imageAnchorY = Attachment::clampAnchor(attachment.anchorY);
        }
        result.rows.append(row);
    }
    resolveGrouping(conversation->messages(), app, result.rows);
    // Post-grouping: sender name / avatar / tail depend on final group.
    for (RenderedMessage &row : result.rows) {
        if (result.isGroup && !row.outgoing && row.type != MessageType::System
            && !row.authorMissing) {
            row.senderName = row.authorName;
            row.showSenderName = true;
            row.showAvatar = true;
        } else if (!result.isGroup && row.type != MessageType::System && !row.outgoing) {
            row.showAvatar = false;
            row.showSenderName = false;
        } else {
            row.showAvatar = false;
            row.showSenderName = false;
        }
        row.hasTail = (row.type != MessageType::System)
            && (row.group == MessageGroup::Single || row.group == MessageGroup::Last);
    }
    resolveDaySeparators(conversation->messages(), result.rows);
    result.empty = result.rows.isEmpty();
    return result;
}

void KonataRenderer::resolveGrouping(const QList<Message> &messages, const AppDefinition &app,
                                     QList<RenderedMessage> &rows)
{
    Q_ASSERT(messages.size() == rows.size());
    const int outer = qMax(0, app.message.cornerRadius);
    const int inner =
        qMax(0, app.structure.chat.messageArea.groupedCornerRadius);
    const bool grouping = app.structure.chat.messageArea.groupConsecutive;
    for (int i = 0; i < rows.size(); ++i) {
        const Message &message = messages.at(i);
        RenderedMessage &row = rows[i];
        row.corners = BubbleCorners{outer, outer, outer, outer};
        row.group = MessageGroup::Single;
        if (!grouping || !isGroupable(message))
            continue;
        const bool previousSame = i > 0 && isGroupable(messages.at(i - 1))
            && messages.at(i - 1).authorId == message.authorId;
        const bool nextSame = i + 1 < messages.size() && isGroupable(messages.at(i + 1))
            && messages.at(i + 1).authorId == message.authorId;
        if (!previousSame && !nextSame)
            continue;
        if (!previousSame)
            row.group = MessageGroup::First;
        else if (!nextSame)
            row.group = MessageGroup::Last;
        else
            row.group = MessageGroup::Middle;
        // Inner corners face the run: left side for incoming, right for out.
        if (row.outgoing) {
            if (row.group == MessageGroup::First || row.group == MessageGroup::Middle)
                row.corners.bottomRight = inner;
            if (row.group == MessageGroup::Last || row.group == MessageGroup::Middle)
                row.corners.topRight = inner;
        } else {
            if (row.group == MessageGroup::First || row.group == MessageGroup::Middle)
                row.corners.bottomLeft = inner;
            if (row.group == MessageGroup::Last || row.group == MessageGroup::Middle)
                row.corners.topLeft = inner;
        }
    }
}

void KonataRenderer::resolveDaySeparators(const QList<Message> &messages,
                                          QList<RenderedMessage> &rows)
{
    Q_ASSERT(messages.size() == rows.size());
    QString previousDay;
    bool havePrevious = false;
    for (int i = 0; i < rows.size(); ++i) {
        const QDateTime when = messages.at(i).createdAt;
        RenderedMessage &row = rows[i];
        if (!when.isValid()) {
            row.startsNewDay = !havePrevious;
            row.dayLabel = havePrevious ? QString() : QStringLiteral("Undated");
            havePrevious = true;
            previousDay = QString();
            continue;
        }
        const QString day = when.toUTC().date().toString(Qt::ISODate);
        row.startsNewDay = !havePrevious || day != previousDay;
        row.dayLabel = row.startsNewDay ? dayLabel(when) : QString();
        previousDay = day;
        havePrevious = true;
    }
}

QString KonataRenderer::authorDisplayName(const Story &story, const Message &message,
                                           bool *missing)
{
    if (missing)
        *missing = false;
    if (message.type == MessageType::System)
        return QStringLiteral("System");
    if (message.authorId.isEmpty()) {
        if (missing)
            *missing = true;
        return QStringLiteral("(unknown author)");
    }
    const Character *author = story.findCharacter(message.authorId);
    if (!author) {
        if (missing)
            *missing = true;
        return QStringLiteral("(unknown author)");
    }
    return characterLabel(*author);
}

namespace {
// Screen title lookup: first enabled matching definition wins, caller's
// fallback otherwise. No app-name knowledge anywhere in this file.
QString screenTitle(const AppStructure &structure, ScreenType wanted, const QString &fallback)
{
    for (const ScreenDefinition &screen : structure.navigation.screens) {
        if (screen.enabled && screen.type == wanted)
            return screen.title.isEmpty() ? fallback : screen.title;
    }
    return fallback;
}
} // namespace

RenderedAppView KonataRenderer::renderAppView(const Story &story, const AppDefinition &app,
                                              const QString &conversationId)
{
    RenderedAppView view;
    view.valid = true;
    view.appName = app.appName;
    view.tokens = app.tokens;
    view.composerPlaceholder = app.structure.chat.composer.placeholder;

    const RenderedConversation conversation = renderConversation(story, app, conversationId);
    if (conversation.valid) {
        view.screen.type = ScreenType::Chat;
        view.screen.title =
            screenTitle(app.structure, ScreenType::Chat, conversation.title);
        if (app.structure.chat.header.visible)
            view.screen.components.append(ComponentType::Header);
        bool needsAvatar = false;
        bool needsBubble = false;
        bool needsImage = false;
        for (const RenderedMessage &row : conversation.rows) {
            if (row.hasAvatar)
                needsAvatar = true;
            if (row.type == MessageType::Image)
                needsImage = true;
            else
                needsBubble = true;
        }
        if (needsAvatar)
            view.screen.components.append(ComponentType::Avatar);
        if (needsBubble)
            view.screen.components.append(ComponentType::MessageBubble);
        if (needsImage)
            view.screen.components.append(ComponentType::ImageMessage);
        if (app.structure.chat.composer.visible)
            view.screen.components.append(ComponentType::Composer);
        view.conversation = conversation;
        return view;
    }

    view.screen.type = ScreenType::ConversationList;
    view.screen.title =
        screenTitle(app.structure, ScreenType::ConversationList, QStringLiteral("Chats"));
    if (app.structure.conversationList.showAvatars)
        view.screen.components.append(ComponentType::Avatar);
    view.screen.components.append(ComponentType::ConversationRow);
    return view;
}

RenderedConversationList KonataRenderer::renderConversationList(const Story &story,
                                                                const AppDefinition &app)
{
    RenderedConversationList list;
    list.valid = true;
    list.title = screenTitle(app.structure, ScreenType::ConversationList,
                             QStringLiteral("Messages"));
    list.showAvatars = app.structure.conversationList.showAvatars;
    list.showPreviewText = app.structure.conversationList.showPreviewText;
    for (const Conversation &conversation : story.conversations()) {
        RenderedConversationRow row;
        row.conversationId = conversation.id();
        row.title = conversation.title().isEmpty() ? QStringLiteral("(untitled conversation)")
                                                   : conversation.title();
        if (!conversation.participantIds().isEmpty()) {
            if (const Character *first = story.findCharacter(conversation.participantIds().first())) {
                row.avatarRef = first->avatarRef;
                row.hasAvatar = !first->avatarRef.isEmpty();
            }
        }
        row.messageCount = conversation.messageCount();
        if (!conversation.messages().isEmpty()) {
            const Message &latest = conversation.messages().last();
            row.previewText = previewSnippet(latest.text);
            row.timestampText = shortTime(latest.createdAt);
        } else {
            row.previewText = QStringLiteral("(No messages)");
        }
        // No Core unread state exists: always false, never faked true.
        row.hasUnread = false;
        list.rows.append(row);
    }
    list.empty = list.rows.isEmpty();
    return list;
}

RenderedProfile KonataRenderer::renderProfile(const Story &story, const QString &characterId)
{
    RenderedProfile profile;
    if (characterId.isEmpty())
        return profile;
    const Character *character = story.findCharacter(characterId);
    if (!character)
        return profile;
    profile.valid = true;
    profile.characterId = character->id;
    profile.displayName =
        character->displayName.isEmpty() ? QStringLiteral("(unnamed)") : character->displayName;
    profile.username = character->username;
    profile.status = character->status;
    profile.avatarRef = character->avatarRef;
    profile.hasAvatar = !character->avatarRef.isEmpty();
    // Structural action labels only — no behavior in this milestone.
    profile.actions.append(QStringLiteral("Message"));
    profile.actions.append(QStringLiteral("Call"));
    profile.actions.append(QStringLiteral("FaceTime"));
    return profile;
}

QString KonataRenderer::shortTime(const QDateTime &when)
{
    if (!when.isValid())
        return QString();
    return QLocale::system().toString(when.toLocalTime().time(), QStringLiteral("h:mm AP"));
}

QString KonataRenderer::dayLabel(const QDateTime &when)
{
    if (!when.isValid())
        return QString();
    const QDate date = when.toLocalTime().date();
    const QDate today = QDate::currentDate();
    const int daysAgo = date.daysTo(today);
    if (daysAgo == 0)
        return QStringLiteral("Today");
    if (daysAgo == 1)
        return QStringLiteral("Yesterday");
    return QLocale::system().toString(date, QLocale::ShortFormat);
}
