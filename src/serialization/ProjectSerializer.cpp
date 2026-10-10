#include "serialization/ProjectSerializer.h"

#include <exception>

#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <nlohmann/json.hpp>

#include "core/Id.h"

namespace Konata {
namespace Serialization {

using nlohmann::json;

namespace {

// FieldError is thrown by parse helpers and converted to LoadResult by deserialize().
struct FieldError {
    QString message;
};

inline std::string toStd(const QString &s)
{
    return s.toUtf8().toStdString();
}

inline QString toQ(const std::string &s)
{
    return QString::fromUtf8(s.c_str(), static_cast<qsizetype>(s.size()));
}

const json &req(const json &obj, const char *field, const QString &ctx)
{
    auto it = obj.find(field);
    if (it == obj.end() || it->is_null())
        throw FieldError{ctx + QStringLiteral(": missing required field '")
                         + QString::fromLatin1(field) + QStringLiteral("'")};
    return *it;
}

QString reqString(const json &obj, const char *field, const QString &ctx)
{
    const json &v = req(obj, field, ctx);
    if (!v.is_string())
        throw FieldError{ctx + QStringLiteral(": field '") + QString::fromLatin1(field)
                         + QStringLiteral("' must be a string")};
    return toQ(v.get<std::string>());
}

QString optString(const json &obj, const char *field, const QString &fallback = QString())
{
    auto it = obj.find(field);
    if (it == obj.end() || it->is_null())
        return fallback;
    if (!it->is_string())
        throw FieldError{QStringLiteral("field '") + QString::fromLatin1(field)
                         + QStringLiteral("' must be a string")};
    return toQ(it->get<std::string>());
}

qint64 optInt(const json &obj, const char *field, qint64 fallback = 0)
{
    auto it = obj.find(field);
    if (it == obj.end() || it->is_null())
        return fallback;
    if (!it->is_number_integer())
        throw FieldError{QStringLiteral("field '") + QString::fromLatin1(field)
                         + QStringLiteral("' must be an integer")};
    return it->get<qint64>();
}

int optIntValue(const json &obj, const char *field, int fallback)
{
    return static_cast<int>(optInt(obj, field, fallback));
}

double optDouble(const json &obj, const char *field, double fallback)
{
    auto it = obj.find(field);
    if (it == obj.end() || it->is_null())
        return fallback;
    if (!it->is_number())
        throw FieldError{QStringLiteral("field '") + QString::fromLatin1(field)
                         + QStringLiteral("' must be a number")};
    return it->get<double>();
}

QString dateTimeToString(const QDateTime &dt)
{
    if (!dt.isValid())
        return QString();
    return dt.toUTC().toString(Qt::ISODateWithMs);
}

QDateTime dateTimeFromString(const QString &s, const QString &ctx, const char *field)
{
    if (s.isEmpty())
        return QDateTime();
    QDateTime dt = QDateTime::fromString(s, Qt::ISODateWithMs);
    if (!dt.isValid())
        dt = QDateTime::fromString(s, Qt::ISODate);
    if (!dt.isValid())
        throw FieldError{ctx + QStringLiteral(": field '") + QString::fromLatin1(field)
                         + QStringLiteral("' is not a valid ISO-8601 date/time")};
    return dt.toUTC();
}

// --- Enum mappings (unknown values are errors, never silent) ---

QString presenceToString(Core::Presence p)
{
    switch (p) {
    case Core::Presence::Online:
        return QStringLiteral("online");
    case Core::Presence::Away:
        return QStringLiteral("away");
    case Core::Presence::Busy:
        return QStringLiteral("busy");
    case Core::Presence::Offline:
        break;
    }
    return QStringLiteral("offline");
}

Core::Presence presenceFromString(const QString &s, const QString &ctx)
{
    if (s == QStringLiteral("online"))
        return Core::Presence::Online;
    if (s == QStringLiteral("away"))
        return Core::Presence::Away;
    if (s == QStringLiteral("busy"))
        return Core::Presence::Busy;
    if (s == QStringLiteral("offline"))
        return Core::Presence::Offline;
    throw FieldError{ctx + QStringLiteral(": invalid presence '") + s + QStringLiteral("'")};
}

QString messageTypeToString(Core::MessageType t)
{
    switch (t) {
    case Core::MessageType::Text:
        return QStringLiteral("text");
    case Core::MessageType::Image:
        return QStringLiteral("image");
    case Core::MessageType::File:
        return QStringLiteral("file");
    case Core::MessageType::System:
        return QStringLiteral("system");
    }
    return QStringLiteral("text");
}

Core::MessageType messageTypeFromString(const QString &s, const QString &ctx)
{
    if (s == QStringLiteral("text"))
        return Core::MessageType::Text;
    if (s == QStringLiteral("image"))
        return Core::MessageType::Image;
    if (s == QStringLiteral("file"))
        return Core::MessageType::File;
    if (s == QStringLiteral("system"))
        return Core::MessageType::System;
    throw FieldError{ctx + QStringLiteral(": invalid message type '") + s + QStringLiteral("'")};
}

QString attachmentTypeToString(Core::AttachmentType t)
{
    switch (t) {
    case Core::AttachmentType::Image:
        return QStringLiteral("image");
    case Core::AttachmentType::File:
        return QStringLiteral("file");
    case Core::AttachmentType::Audio:
        return QStringLiteral("audio");
    case Core::AttachmentType::Video:
        return QStringLiteral("video");
    case Core::AttachmentType::Other:
        return QStringLiteral("other");
    }
    return QStringLiteral("other");
}

Core::AttachmentType attachmentTypeFromString(const QString &s, const QString &ctx)
{
    if (s == QStringLiteral("image"))
        return Core::AttachmentType::Image;
    if (s == QStringLiteral("file"))
        return Core::AttachmentType::File;
    if (s == QStringLiteral("audio"))
        return Core::AttachmentType::Audio;
    if (s == QStringLiteral("video"))
        return Core::AttachmentType::Video;
    if (s == QStringLiteral("other"))
        return Core::AttachmentType::Other;
    throw FieldError{ctx + QStringLiteral(": invalid attachment type '") + s + QStringLiteral("'")};
}

QString conversationTypeToString(Core::ConversationType t)
{
    return t == Core::ConversationType::Group ? QStringLiteral("group") : QStringLiteral("private");
}

Core::ConversationType conversationTypeFromString(const QString &s, const QString &ctx)
{
    if (s == QStringLiteral("private"))
        return Core::ConversationType::Private;
    if (s == QStringLiteral("group"))
        return Core::ConversationType::Group;
    throw FieldError{ctx + QStringLiteral(": invalid conversation type '") + s + QStringLiteral("'")};
}

QString sceneEventTypeToString(Core::SceneEventType t)
{
    switch (t) {
    case Core::SceneEventType::SendMessage:
        return QStringLiteral("sendMessage");
    case Core::SceneEventType::Wait:
        return QStringLiteral("wait");
    case Core::SceneEventType::StartTyping:
        return QStringLiteral("startTyping");
    case Core::SceneEventType::StopTyping:
        return QStringLiteral("stopTyping");
    case Core::SceneEventType::Notification:
        return QStringLiteral("notification");
    }
    return QStringLiteral("sendMessage");
}

Core::SceneEventType sceneEventTypeFromString(const QString &s, const QString &ctx)
{
    if (s == QStringLiteral("sendMessage"))
        return Core::SceneEventType::SendMessage;
    if (s == QStringLiteral("wait"))
        return Core::SceneEventType::Wait;
    if (s == QStringLiteral("startTyping"))
        return Core::SceneEventType::StartTyping;
    if (s == QStringLiteral("stopTyping"))
        return Core::SceneEventType::StopTyping;
    if (s == QStringLiteral("notification"))
        return Core::SceneEventType::Notification;
    throw FieldError{ctx + QStringLiteral(": invalid scene event type '") + s + QStringLiteral("'")};
}

// --- Per-type converters ---

json characterToJson(const Core::Character &c)
{
    return json{{"id", toStd(c.id)},
                {"displayName", toStd(c.displayName)},
                {"username", toStd(c.username)},
                {"avatarRef", toStd(c.avatarRef)},
                {"status", toStd(c.status)},
                {"presence", toStd(presenceToString(c.presence))}};
}

Core::Character characterFromJson(const json &j, const QString &ctx)
{
    if (!j.is_object())
        throw FieldError{ctx + QStringLiteral(": character must be an object")};
    Core::Character c;
    c.id = reqString(j, "id", ctx);
    c.displayName = reqString(j, "displayName", ctx);
    c.username = reqString(j, "username", ctx);
    c.avatarRef = optString(j, "avatarRef");
    c.status = optString(j, "status");
    c.presence = presenceFromString(optString(j, "presence", QStringLiteral("offline")), ctx);
    Core::IdGenerator::noteLoadedId(c.id);
    return c;
}

json reactionToJson(const Core::Reaction &r)
{
    return json{{"emoji", toStd(r.emoji)},
                {"characterId", toStd(r.characterId)},
                {"createdAt", toStd(dateTimeToString(r.createdAt))}};
}

Core::Reaction reactionFromJson(const json &j, const QString &ctx)
{
    if (!j.is_object())
        throw FieldError{ctx + QStringLiteral(": reaction must be an object")};
    Core::Reaction r;
    r.emoji = reqString(j, "emoji", ctx);
    r.characterId = reqString(j, "characterId", ctx);
    r.createdAt = dateTimeFromString(optString(j, "createdAt"), ctx, "createdAt");
    return r;
}

json attachmentToJson(const Core::Attachment &a)
{
    return json{{"id", toStd(a.id)},
                {"type", toStd(attachmentTypeToString(a.type))},
                {"fileName", toStd(a.fileName)},
                {"fileRef", toStd(a.fileRef)},
                {"mimeType", toStd(a.mimeType)},
                {"sizeBytes", a.sizeBytes},
                {"anchorX", Core::Attachment::clampAnchor(a.anchorX)},
                {"anchorY", Core::Attachment::clampAnchor(a.anchorY)}};
}

Core::Attachment attachmentFromJson(const json &j, const QString &ctx)
{
    if (!j.is_object())
        throw FieldError{ctx + QStringLiteral(": attachment must be an object")};
    Core::Attachment a;
    a.id = reqString(j, "id", ctx);
    a.type = attachmentTypeFromString(reqString(j, "type", ctx), ctx);
    a.fileName = optString(j, "fileName");
    a.fileRef = optString(j, "fileRef");
    a.mimeType = optString(j, "mimeType");
    a.sizeBytes = optInt(j, "sizeBytes", -1);
    // Absent in pre-0.2A.1 files: deterministic center default.
    a.anchorX = Core::Attachment::clampAnchor(optDouble(j, "anchorX", 0.5));
    a.anchorY = Core::Attachment::clampAnchor(optDouble(j, "anchorY", 0.5));
    Core::IdGenerator::noteLoadedId(a.id);
    return a;
}

json messageToJson(const Core::Message &m)
{
    json reactions = json::array();
    for (const Core::Reaction &r : m.reactions)
        reactions.push_back(reactionToJson(r));
    json attachments = json::array();
    for (const Core::Attachment &a : m.attachments)
        attachments.push_back(attachmentToJson(a));
    return json{{"id", toStd(m.id)},
                {"conversationId", toStd(m.conversationId)},
                {"authorId", toStd(m.authorId)},
                {"type", toStd(messageTypeToString(m.type))},
                {"text", toStd(m.text)},
                {"createdAt", toStd(dateTimeToString(m.createdAt))},
                {"order", m.order},
                {"reactions", std::move(reactions)},
                {"attachments", std::move(attachments)}};
}

Core::Message messageFromJson(const json &j, const QString &ctx)
{
    if (!j.is_object())
        throw FieldError{ctx + QStringLiteral(": message must be an object")};
    Core::Message m;
    m.id = reqString(j, "id", ctx);
    m.conversationId = optString(j, "conversationId");
    m.authorId = optString(j, "authorId");
    m.type = messageTypeFromString(reqString(j, "type", ctx), ctx);
    m.text = optString(j, "text");
    m.createdAt = dateTimeFromString(optString(j, "createdAt"), ctx, "createdAt");
    m.order = optInt(j, "order", 0);
    auto rit = j.find("reactions");
    if (rit != j.end()) {
        if (!rit->is_array())
            throw FieldError{ctx + QStringLiteral(": 'reactions' must be an array")};
        int i = 0;
        for (const json &r : *rit)
            m.reactions.append(
                reactionFromJson(r, ctx + QStringLiteral(".reactions[%1]").arg(i++)));
    }
    auto ait = j.find("attachments");
    if (ait != j.end()) {
        if (!ait->is_array())
            throw FieldError{ctx + QStringLiteral(": 'attachments' must be an array")};
        int i = 0;
        for (const json &a : *ait)
            m.attachments.append(
                attachmentFromJson(a, ctx + QStringLiteral(".attachments[%1]").arg(i++)));
    }
    Core::IdGenerator::noteLoadedId(m.id);
    return m;
}

json conversationToJson(const Core::Conversation &c)
{
    json participants = json::array();
    for (const QString &id : c.participantIds())
        participants.push_back(toStd(id));
    json messages = json::array();
    for (const Core::Message &m : c.messages())
        messages.push_back(messageToJson(m));
    return json{{"id", toStd(c.id())},
                {"title", toStd(c.title())},
                {"type", toStd(conversationTypeToString(c.type()))},
                {"participants", std::move(participants)},
                {"messages", std::move(messages)}};
}

Core::Conversation conversationFromJson(const json &j, const QString &ctx)
{
    if (!j.is_object())
        throw FieldError{ctx + QStringLiteral(": conversation must be an object")};
    Core::Conversation c = Core::Conversation::create(
        reqString(j, "title", ctx), conversationTypeFromString(reqString(j, "type", ctx), ctx));
    c.setId(reqString(j, "id", ctx));
    Core::IdGenerator::noteLoadedId(c.id());

    auto pit = j.find("participants");
    if (pit != j.end()) {
        if (!pit->is_array())
            throw FieldError{ctx + QStringLiteral(": 'participants' must be an array")};
        for (const json &p : *pit) {
            if (!p.is_string())
                throw FieldError{ctx + QStringLiteral(": participants must be strings")};
            c.addParticipant(toQ(p.get<std::string>()));
        }
    }
    auto mit = j.find("messages");
    if (mit != j.end()) {
        if (!mit->is_array())
            throw FieldError{ctx + QStringLiteral(": 'messages' must be an array")};
        int i = 0;
        for (const json &m : *mit) {
            Core::Message msg =
                messageFromJson(m, ctx + QStringLiteral(".messages[%1]").arg(i++));
            // addMessage only fills in an empty conversationId / zero order,
            // so fully persisted values survive the round trip unchanged.
            c.addMessage(msg);
        }
    }
    return c;
}

json sceneEventToJson(const Core::SceneEvent &e)
{
    return json{{"id", toStd(e.id)},
                {"type", toStd(sceneEventTypeToString(e.type))},
                {"characterId", toStd(e.characterId)},
                {"messageId", toStd(e.messageId)},
                {"conversationId", toStd(e.conversationId)},
                {"waitMs", e.waitMs},
                {"text", toStd(e.text)}};
}

Core::SceneEvent sceneEventFromJson(const json &j, const QString &ctx)
{
    if (!j.is_object())
        throw FieldError{ctx + QStringLiteral(": scene event must be an object")};
    Core::SceneEvent e;
    e.id = reqString(j, "id", ctx);
    e.type = sceneEventTypeFromString(reqString(j, "type", ctx), ctx);
    e.characterId = optString(j, "characterId");
    e.messageId = optString(j, "messageId");
    e.conversationId = optString(j, "conversationId");
    e.waitMs = optIntValue(j, "waitMs", 0);
    e.text = optString(j, "text");
    Core::IdGenerator::noteLoadedId(e.id);
    return e;
}

json sceneToJson(const Core::Scene &s)
{
    json events = json::array();
    for (const Core::SceneEvent &e : s.events())
        events.push_back(sceneEventToJson(e));
    return json{{"id", toStd(s.id())},
                {"name", toStd(s.name())},
                {"description", toStd(s.description())},
                {"events", std::move(events)}};
}

Core::Scene sceneFromJson(const json &j, const QString &ctx)
{
    if (!j.is_object())
        throw FieldError{ctx + QStringLiteral(": scene must be an object")};
    Core::Scene s =
        Core::Scene::create(reqString(j, "name", ctx), optString(j, "description"));
    s.setId(reqString(j, "id", ctx));
    Core::IdGenerator::noteLoadedId(s.id());
    auto eit = j.find("events");
    if (eit != j.end()) {
        if (!eit->is_array())
            throw FieldError{ctx + QStringLiteral(": 'events' must be an array")};
        int i = 0;
        for (const json &e : *eit)
            s.addEvent(sceneEventFromJson(e, ctx + QStringLiteral(".events[%1]").arg(i++)));
    }
    return s;
}

json storyToJson(const Core::Story &s)
{
    json characters = json::array();
    for (const Core::Character &c : s.characters())
        characters.push_back(characterToJson(c));
    json conversations = json::array();
    for (const Core::Conversation &c : s.conversations())
        conversations.push_back(conversationToJson(c));
    json scenes = json::array();
    for (const Core::Scene &sc : s.scenes())
        scenes.push_back(sceneToJson(sc));
    return json{{"id", toStd(s.id())},
                {"title", toStd(s.title())},
                {"description", toStd(s.description())},
                {"createdAt", toStd(dateTimeToString(s.createdAt()))},
                {"characters", std::move(characters)},
                {"conversations", std::move(conversations)},
                {"scenes", std::move(scenes)}};
}

Core::Story storyFromJson(const json &j, const QString &ctx)
{
    if (!j.is_object())
        throw FieldError{ctx + QStringLiteral(": story must be an object")};
    Core::Story s =
        Core::Story::create(reqString(j, "title", ctx), optString(j, "description"));
    s.setId(reqString(j, "id", ctx));
    s.setCreatedAt(dateTimeFromString(optString(j, "createdAt"), ctx, "createdAt"));
    Core::IdGenerator::noteLoadedId(s.id());

    auto cit = j.find("characters");
    if (cit != j.end()) {
        if (!cit->is_array())
            throw FieldError{ctx + QStringLiteral(": 'characters' must be an array")};
        int i = 0;
        for (const json &c : *cit)
            s.addCharacter(characterFromJson(c, ctx + QStringLiteral(".characters[%1]").arg(i++)));
    }
    auto coit = j.find("conversations");
    if (coit != j.end()) {
        if (!coit->is_array())
            throw FieldError{ctx + QStringLiteral(": 'conversations' must be an array")};
        int i = 0;
        for (const json &c : *coit)
            s.addConversation(
                conversationFromJson(c, ctx + QStringLiteral(".conversations[%1]").arg(i++)));
    }
    auto sit = j.find("scenes");
    if (sit != j.end()) {
        if (!sit->is_array())
            throw FieldError{ctx + QStringLiteral(": 'scenes' must be an array")};
        int i = 0;
        for (const json &sc : *sit)
            s.addScene(sceneFromJson(sc, ctx + QStringLiteral(".scenes[%1]").arg(i++)));
    }
    return s;
}

bool optBool(const json &obj, const char *field, bool fallback)
{
    auto it = obj.find(field);
    if (it == obj.end() || it->is_null())
        return fallback;
    if (!it->is_boolean())
        throw FieldError{QStringLiteral("field '") + QString::fromLatin1(field)
                         + QStringLiteral("' must be a boolean")};
    return it->get<bool>();
}

QString screenTypeToString(Core::ScreenType t)
{
    switch (t) {
    case Core::ScreenType::ConversationList:
        return QStringLiteral("conversationList");
    case Core::ScreenType::Chat:
        return QStringLiteral("chat");
    case Core::ScreenType::Profile:
        return QStringLiteral("profile");
    case Core::ScreenType::Custom:
        return QStringLiteral("custom");
    }
    return QStringLiteral("custom");
}

Core::ScreenType screenTypeFromString(const QString &s, const QString &ctx)
{
    if (s == QStringLiteral("conversationList"))
        return Core::ScreenType::ConversationList;
    if (s == QStringLiteral("chat"))
        return Core::ScreenType::Chat;
    if (s == QStringLiteral("profile"))
        return Core::ScreenType::Profile;
    if (s == QStringLiteral("custom"))
        return Core::ScreenType::Custom;
    throw FieldError{ctx + QStringLiteral(": invalid screen type '") + s + QStringLiteral("'")};
}

QString navigationStyleToString(Core::NavigationStyle s)
{
    switch (s) {
    case Core::NavigationStyle::Tabs:
        return QStringLiteral("tabs");
    case Core::NavigationStyle::Sidebar:
        return QStringLiteral("sidebar");
    case Core::NavigationStyle::Stack:
        return QStringLiteral("stack");
    case Core::NavigationStyle::None:
        return QStringLiteral("none");
    }
    return QStringLiteral("tabs");
}

Core::NavigationStyle navigationStyleFromString(const QString &s, const QString &ctx)
{
    if (s == QStringLiteral("tabs"))
        return Core::NavigationStyle::Tabs;
    if (s == QStringLiteral("sidebar"))
        return Core::NavigationStyle::Sidebar;
    if (s == QStringLiteral("stack"))
        return Core::NavigationStyle::Stack;
    if (s == QStringLiteral("none"))
        return Core::NavigationStyle::None;
    throw FieldError{ctx + QStringLiteral(": invalid navigation style '") + s
                     + QStringLiteral("'")};
}

json screenToJson(const Core::ScreenDefinition &s)
{
    return json{{"type", toStd(screenTypeToString(s.type))},
                {"title", toStd(s.title)},
                {"customId", toStd(s.customId)},
                {"enabled", s.enabled}};
}

Core::ScreenDefinition screenFromJson(const json &j, const QString &ctx)
{
    if (!j.is_object())
        throw FieldError{ctx + QStringLiteral(": screen must be an object")};
    Core::ScreenDefinition s;
    s.type = screenTypeFromString(reqString(j, "type", ctx), ctx);
    s.title = optString(j, "title");
    s.customId = optString(j, "customId");
    s.enabled = optBool(j, "enabled", true);
    return s;
}

json structureToJson(const Core::AppStructure &s)
{
    json screens = json::array();
    for (const Core::ScreenDefinition &screen : s.navigation.screens)
        screens.push_back(screenToJson(screen));
    return json{
        {"navigation",
         {{"style", toStd(navigationStyleToString(s.navigation.style))},
          {"screens", std::move(screens)}}},
        {"chat",
         {{"header",
           {{"visible", s.chat.header.visible},
            {"showAvatar", s.chat.header.showAvatar},
            {"showBackButton", s.chat.header.showBackButton}}},
          {"messageArea",
           {{"groupConsecutive", s.chat.messageArea.groupConsecutive},
            {"showAuthorNames", s.chat.messageArea.showAuthorNames},
            {"showTimestamps", s.chat.messageArea.showTimestamps},
            {"maxBubbleWidthPercent", s.chat.messageArea.maxBubbleWidthPercent},
            {"groupedCornerRadius", s.chat.messageArea.groupedCornerRadius}}},
          {"composer",
           {{"visible", s.chat.composer.visible},
            {"placeholder", toStd(s.chat.composer.placeholder)},
            {"showAttachButton", s.chat.composer.showAttachButton}}}}},
        {"conversationList",
         {{"showAvatars", s.conversationList.showAvatars},
          {"showPreviewText", s.conversationList.showPreviewText},
          {"showUnreadBadges", s.conversationList.showUnreadBadges}}},
        {"profile",
         {{"showAvatar", s.profile.showAvatar}, {"showStatus", s.profile.showStatus}}}};
}

void structureFromJson(const json &j, const QString &ctx, Core::AppStructure &s)
{
    auto nit = j.find("navigation");
    if (nit != j.end()) {
        if (!nit->is_object())
            throw FieldError{ctx + QStringLiteral(": 'navigation' must be an object")};
        s.navigation.style = navigationStyleFromString(
            optString(*nit, "style",
                      navigationStyleToString(s.navigation.style)),
            ctx + QStringLiteral(".navigation"));
        auto sit = nit->find("screens");
        if (sit != nit->end()) {
            if (!sit->is_array())
                throw FieldError{ctx + QStringLiteral(": 'navigation.screens' must be an array")};
            s.navigation.screens.clear();
            int i = 0;
            for (const json &screen : *sit)
                s.navigation.screens.append(screenFromJson(
                    screen, ctx + QStringLiteral(".navigation.screens[%1]").arg(i++)));
        }
    }
    auto chit = j.find("chat");
    if (chit != j.end()) {
        if (!chit->is_object())
            throw FieldError{ctx + QStringLiteral(": 'chat' must be an object")};
        const QString chatCtx = ctx + QStringLiteral(".chat");
        auto hit = chit->find("header");
        if (hit != chit->end()) {
            if (!hit->is_object())
                throw FieldError{chatCtx + QStringLiteral(": 'header' must be an object")};
            s.chat.header.visible = optBool(*hit, "visible", s.chat.header.visible);
            s.chat.header.showAvatar = optBool(*hit, "showAvatar", s.chat.header.showAvatar);
            s.chat.header.showBackButton =
                optBool(*hit, "showBackButton", s.chat.header.showBackButton);
        }
        auto mait = chit->find("messageArea");
        if (mait != chit->end()) {
            if (!mait->is_object())
                throw FieldError{chatCtx + QStringLiteral(": 'messageArea' must be an object")};
            s.chat.messageArea.groupConsecutive =
                optBool(*mait, "groupConsecutive", s.chat.messageArea.groupConsecutive);
            s.chat.messageArea.showAuthorNames =
                optBool(*mait, "showAuthorNames", s.chat.messageArea.showAuthorNames);
            s.chat.messageArea.showTimestamps =
                optBool(*mait, "showTimestamps", s.chat.messageArea.showTimestamps);
            s.chat.messageArea.maxBubbleWidthPercent =
                optIntValue(*mait, "maxBubbleWidthPercent",
                            s.chat.messageArea.maxBubbleWidthPercent);
            s.chat.messageArea.groupedCornerRadius =
                optIntValue(*mait, "groupedCornerRadius",
                            s.chat.messageArea.groupedCornerRadius);
        }
        auto coit = chit->find("composer");
        if (coit != chit->end()) {
            if (!coit->is_object())
                throw FieldError{chatCtx + QStringLiteral(": 'composer' must be an object")};
            s.chat.composer.visible = optBool(*coit, "visible", s.chat.composer.visible);
            s.chat.composer.placeholder =
                optString(*coit, "placeholder", s.chat.composer.placeholder);
            s.chat.composer.showAttachButton =
                optBool(*coit, "showAttachButton", s.chat.composer.showAttachButton);
        }
    }
    auto clit = j.find("conversationList");
    if (clit != j.end()) {
        if (!clit->is_object())
            throw FieldError{ctx + QStringLiteral(": 'conversationList' must be an object")};
        s.conversationList.showAvatars =
            optBool(*clit, "showAvatars", s.conversationList.showAvatars);
        s.conversationList.showPreviewText =
            optBool(*clit, "showPreviewText", s.conversationList.showPreviewText);
        s.conversationList.showUnreadBadges =
            optBool(*clit, "showUnreadBadges", s.conversationList.showUnreadBadges);
    }
    auto pit = j.find("profile");
    if (pit != j.end()) {
        if (!pit->is_object())
            throw FieldError{ctx + QStringLiteral(": 'profile' must be an object")};
        s.profile.showAvatar = optBool(*pit, "showAvatar", s.profile.showAvatar);
        s.profile.showStatus = optBool(*pit, "showStatus", s.profile.showStatus);
    }
}

json tokensToJson(const Core::DesignTokens &t)
{
    return json{{"spacingSmall", t.spacingSmall},
                {"spacingMedium", t.spacingMedium},
                {"spacingLarge", t.spacingLarge},
                {"cornerRadius", t.cornerRadius},
                {"avatarSize", t.avatarSize},
                {"fontSizeSmall", t.fontSizeSmall},
                {"fontSizeBody", t.fontSizeBody},
                {"fontSizeTitle", t.fontSizeTitle}};
}

void tokensFromJson(const json &j, const QString &ctx, Core::DesignTokens &t)
{
    if (!j.is_object())
        throw FieldError{ctx + QStringLiteral(": 'tokens' must be an object")};
    t.spacingSmall = optIntValue(j, "spacingSmall", t.spacingSmall);
    t.spacingMedium = optIntValue(j, "spacingMedium", t.spacingMedium);
    t.spacingLarge = optIntValue(j, "spacingLarge", t.spacingLarge);
    t.cornerRadius = optIntValue(j, "cornerRadius", t.cornerRadius);
    t.avatarSize = optIntValue(j, "avatarSize", t.avatarSize);
    t.fontSizeSmall = optIntValue(j, "fontSizeSmall", t.fontSizeSmall);
    t.fontSizeBody = optIntValue(j, "fontSizeBody", t.fontSizeBody);
    t.fontSizeTitle = optIntValue(j, "fontSizeTitle", t.fontSizeTitle);
}

QString canvasPresetToString(Core::CanvasSettings::Preset preset)
{
    return Core::CanvasSettings::presetLabel(preset);
}

Core::CanvasSettings::Preset canvasPresetFromString(const QString &s, const QString &ctx)
{
    bool ok = false;
    const auto preset = Core::CanvasSettings::presetFromLabel(s, &ok);
    if (!ok)
        throw FieldError{ctx + QStringLiteral(": invalid canvas preset '") + s + QStringLiteral("'")};
    return preset;
}

json canvasToJson(const Core::CanvasSettings &c)
{
    return json{{"preset", toStd(canvasPresetToString(c.preset))},
                {"customWidth", c.customWidth},
                {"customHeight", c.customHeight}};
}

Core::CanvasSettings canvasFromJson(const json &j, const QString &ctx)
{
    if (!j.is_object())
        throw FieldError{ctx + QStringLiteral(": canvas must be an object")};
    Core::CanvasSettings canvas;
    canvas.preset = canvasPresetFromString(
        optString(j, "preset", canvasPresetToString(canvas.preset)), ctx);
    canvas.customWidth = optIntValue(j, "customWidth", canvas.customWidth);
    canvas.customHeight = optIntValue(j, "customHeight", canvas.customHeight);
    if (!canvas.isValid())
        throw FieldError{ctx + QStringLiteral(": invalid canvas dimensions")};
    return canvas;
}

json appToJson(const Core::AppDefinition &a)
{
    return json{{"id", toStd(a.id)},
                {"appName", toStd(a.appName)},
                {"packageId", toStd(a.packageId)},
                {"theme",
                 {{"themeName", toStd(a.theme.themeName)},
                  {"accentColor", toStd(a.theme.accentColor)},
                  {"darkMode", a.theme.darkMode}}},
                {"navigation",
                 {{"barColor", toStd(a.navigation.barColor)},
                  {"activeTint", toStd(a.navigation.activeTint)},
                  {"inactiveTint", toStd(a.navigation.inactiveTint)},
                  {"barHeight", a.navigation.barHeight}}},
                {"chat",
                 {{"backgroundColor", toStd(a.chat.backgroundColor)},
                  {"datePillColor", toStd(a.chat.datePillColor)},
                  {"fontSize", a.chat.fontSize}}},
                {"message",
                 {{"incomingBubbleColor", toStd(a.message.incomingBubbleColor)},
                  {"outgoingBubbleColor", toStd(a.message.outgoingBubbleColor)},
                  {"incomingTextColor", toStd(a.message.incomingTextColor)},
                  {"outgoingTextColor", toStd(a.message.outgoingTextColor)},
                  {"cornerRadius", a.message.cornerRadius},
                  {"fontSize", a.message.fontSize}}},
                {"notification",
                 {{"enabled", a.notification.enabled},
                  {"soundRef", toStd(a.notification.soundRef)},
                  {"bannerColor", toStd(a.notification.bannerColor)}}},
                {"structure", structureToJson(a.structure)},
                {"tokens", tokensToJson(a.tokens)}};
}

void appearanceFromJson(const json &j, const QString &ctx, Core::AppDefinition &a)
{
    auto tit = j.find("theme");
    if (tit != j.end()) {
        if (!tit->is_object())
            throw FieldError{ctx + QStringLiteral(": 'theme' must be an object")};
        a.theme.themeName = optString(*tit, "themeName", a.theme.themeName);
        a.theme.accentColor = optString(*tit, "accentColor", a.theme.accentColor);
        auto dit = tit->find("darkMode");
        if (dit != tit->end()) {
            if (!dit->is_boolean())
                throw FieldError{ctx + QStringLiteral(": 'theme.darkMode' must be a boolean")};
            a.theme.darkMode = dit->get<bool>();
        }
    }
    auto nit = j.find("navigation");
    if (nit != j.end()) {
        if (!nit->is_object())
            throw FieldError{ctx + QStringLiteral(": 'navigation' must be an object")};
        a.navigation.barColor = optString(*nit, "barColor", a.navigation.barColor);
        a.navigation.activeTint = optString(*nit, "activeTint", a.navigation.activeTint);
        a.navigation.inactiveTint = optString(*nit, "inactiveTint", a.navigation.inactiveTint);
        a.navigation.barHeight = optIntValue(*nit, "barHeight", a.navigation.barHeight);
    }
    auto cit = j.find("chat");
    if (cit != j.end()) {
        if (!cit->is_object())
            throw FieldError{ctx + QStringLiteral(": 'chat' must be an object")};
        a.chat.backgroundColor = optString(*cit, "backgroundColor", a.chat.backgroundColor);
        a.chat.datePillColor = optString(*cit, "datePillColor", a.chat.datePillColor);
        a.chat.fontSize = optIntValue(*cit, "fontSize", a.chat.fontSize);
    }
    auto mit = j.find("message");
    if (mit != j.end()) {
        if (!mit->is_object())
            throw FieldError{ctx + QStringLiteral(": 'message' must be an object")};
        a.message.incomingBubbleColor =
            optString(*mit, "incomingBubbleColor", a.message.incomingBubbleColor);
        a.message.outgoingBubbleColor =
            optString(*mit, "outgoingBubbleColor", a.message.outgoingBubbleColor);
        a.message.incomingTextColor =
            optString(*mit, "incomingTextColor", a.message.incomingTextColor);
        a.message.outgoingTextColor =
            optString(*mit, "outgoingTextColor", a.message.outgoingTextColor);
        a.message.cornerRadius = optIntValue(*mit, "cornerRadius", a.message.cornerRadius);
        a.message.fontSize = optIntValue(*mit, "fontSize", a.message.fontSize);
    }
    auto noit = j.find("notification");
    if (noit != j.end()) {
        if (!noit->is_object())
            throw FieldError{ctx + QStringLiteral(": 'notification' must be an object")};
        auto eit = noit->find("enabled");
        if (eit != noit->end()) {
            if (!eit->is_boolean())
                throw FieldError{ctx + QStringLiteral(": 'notification.enabled' must be a boolean")};
            a.notification.enabled = eit->get<bool>();
        }
        a.notification.soundRef = optString(*noit, "soundRef", a.notification.soundRef);
        a.notification.bannerColor =
            optString(*noit, "bannerColor", a.notification.bannerColor);
    }
    // 0.2B blocks. Entirely optional: pre-0.2B files load with defaults.
    auto stit = j.find("structure");
    if (stit != j.end()) {
        if (!stit->is_object())
            throw FieldError{ctx + QStringLiteral(": 'structure' must be an object")};
        structureFromJson(*stit, ctx + QStringLiteral(".structure"), a.structure);
    }
    auto toit = j.find("tokens");
    if (toit != j.end())
        tokensFromJson(*toit, ctx + QStringLiteral(".tokens"), a.tokens);
}

Core::AppDefinition appFromJson(const json &j, const QString &ctx)
{
    if (!j.is_object())
        throw FieldError{ctx + QStringLiteral(": appDefinition must be an object")};
    Core::AppDefinition a = Core::AppDefinition::defaults(reqString(j, "appName", ctx));
    a.id = reqString(j, "id", ctx);
    a.packageId = optString(j, "packageId", a.packageId);
    appearanceFromJson(j, ctx, a);
    Core::IdGenerator::noteLoadedId(a.id);
    return a;
}

} // namespace

// --- Public API ---

SaveResult ProjectSerializer::saveToFile(const QString &filePath, const KonataProject &project)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        return SaveResult{false, QStringLiteral("Cannot open file for writing: %1 (%2)")
                                        .arg(filePath, file.errorString())};
    const QByteArray data = serialize(project);
    if (file.write(data) != data.size())
        return SaveResult{false, QStringLiteral("Failed to write file: %1 (%2)")
                                        .arg(filePath, file.errorString())};
    return SaveResult{true, QString()};
}

LoadResult ProjectSerializer::loadFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.exists())
        return LoadResult{false, QStringLiteral("Project file does not exist: %1").arg(filePath),
                          KonataProject{}};
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return LoadResult{false, QStringLiteral("Cannot open file for reading: %1 (%2)")
                                        .arg(filePath, file.errorString()),
                          KonataProject{}};
    return deserialize(file.readAll());
}

QByteArray ProjectSerializer::serialize(const KonataProject &project)
{
    json root;
    root["format"] = kFormatName;
    root["formatVersion"] = kFormatVersion;
    root["story"] = storyToJson(project.story);
    root["appDefinition"] = appToJson(project.appDefinition);
    root["canvas"] = canvasToJson(project.canvas);
    {
        QJsonObject qo = project.uiDesign.serialize();
        QJsonDocument qdoc(qo);
        root["uiDesign"] = json::parse(qdoc.toJson().toStdString());
    }
    const std::string text = root.dump(2);
    return QByteArray(text.c_str(), static_cast<qsizetype>(text.size()));
}

LoadResult ProjectSerializer::deserialize(const QByteArray &data)
{
    json root;
    try {
        const char *begin = data.constData();
        root = json::parse(begin, begin + data.size());
    } catch (const json::parse_error &e) {
        return LoadResult{false,
                          QStringLiteral("Invalid project JSON: %1").arg(QString::fromLatin1(e.what())),
                          KonataProject{}};
    } catch (const std::exception &e) {
        return LoadResult{false,
                          QStringLiteral("Invalid project JSON: %1").arg(QString::fromLatin1(e.what())),
                          KonataProject{}};
    }

    try {
        if (!root.is_object())
            throw FieldError{QStringLiteral("Project root must be a JSON object")};
        auto fit = root.find("format");
        if (fit != root.end() && fit->is_string()
            && fit->get<std::string>() != kFormatName)
            throw FieldError{QStringLiteral("Not a KONATA project file (format '%1')")
                             .arg(toQ(fit->get<std::string>()))};
        auto vit = root.find("formatVersion");
        if (vit == root.end() || !vit->is_number_integer())
            throw FieldError{QStringLiteral("Missing or invalid required field 'formatVersion'")};
        const int version = vit->get<int>();
        if (version != kFormatVersion)
            throw FieldError{QStringLiteral("Unsupported format version %1 (expected %2)")
                             .arg(version)
                             .arg(kFormatVersion)};

        KonataProject project;
        project.story = storyFromJson(req(root, "story", QStringLiteral("project")),
                                      QStringLiteral("story"));
        project.appDefinition =
            appFromJson(req(root, "appDefinition", QStringLiteral("project")),
                        QStringLiteral("appDefinition"));
        auto cit = root.find("canvas");
        if (cit != root.end()) {
            project.canvas = canvasFromJson(*cit, QStringLiteral("canvas"));
        } else {
            project.canvas = Core::CanvasSettings();
        }
        auto uit = root.find("uiDesign");
        if (uit != root.end() && uit->is_object()) {
            QByteArray qa = QByteArray::fromStdString(uit->dump());
            QJsonDocument qdoc = QJsonDocument::fromJson(qa);
            project.uiDesign = Core::UiDesign::deserialize(qdoc.object());
        }
        return LoadResult{true, QString(), project};
    } catch (const FieldError &e) {
        return LoadResult{false, e.message, KonataProject{}};
    } catch (const json::exception &e) {
        return LoadResult{false,
                          QStringLiteral("Invalid project JSON: %1").arg(QString::fromLatin1(e.what())),
                          KonataProject{}};
    } catch (const std::exception &e) {
        return LoadResult{false,
                          QStringLiteral("Failed to load project: %1").arg(QString::fromLatin1(e.what())),
                          KonataProject{}};
    }
}

} // namespace Serialization
} // namespace Konata
