// KonataCoreCheck — lightweight 0.1B verification (no test framework).
// Builds Characters/Messages/Conversations/Scenes/Stories/AppDefinitions
// and checks ID generation. Prints PASS/FAIL lines; exit 0 = all pass.
#include <QDateTime>
#include <QTextStream>

#include "core/AppDefinition.h"
#include "core/Character.h"
#include "core/Conversation.h"
#include "core/Id.h"
#include "core/Message.h"
#include "core/Scene.h"
#include "core/Story.h"

using namespace Konata::Core;

namespace {
int g_failures = 0;
QTextStream *g_out = nullptr;

void check(bool condition, const QString &label)
{
    if (condition) {
        *g_out << "[PASS] " << label << "\n";
    } else {
        *g_out << "[FAIL] " << label << "\n";
        ++g_failures;
    }
}
} // namespace

int main()
{
    QTextStream out(stdout);
    g_out = &out;

    IdGenerator::resetForTests();

    // --- IDs ---
    const QString idA = IdGenerator::create(QStringLiteral("char"));
    const QString idB = IdGenerator::create(QStringLiteral("char"));
    check(!idA.isEmpty() && !idB.isEmpty(), QStringLiteral("IDs are generated"));
    check(idA != idB, QStringLiteral("IDs are unique"));
    check(idA.startsWith(QStringLiteral("char_")), QStringLiteral("IDs carry prefix"));

    // --- Characters ---
    const Character alice = Character::create(
        QStringLiteral("Alice"), QStringLiteral("alice"),
        QStringLiteral("avatars/alice.png"), QStringLiteral("Online"),
        Presence::Online);
    const Character bob = Character::create(
        QStringLiteral("Bob"), QStringLiteral("bob"));
    check(!alice.id.isEmpty() && alice.displayName == QStringLiteral("Alice"),
          QStringLiteral("Character constructs with name"));
    check(alice.username == QStringLiteral("alice")
              && alice.avatarRef == QStringLiteral("avatars/alice.png"),
          QStringLiteral("Character keeps username/avatarRef"));
    check(alice.id != bob.id, QStringLiteral("Characters get distinct IDs"));

    // --- Messages (+reactions/attachments) ---
    Message hello = Message::createText(alice.id, QStringLiteral("Hello!"));
    check(hello.type == MessageType::Text && !hello.id.isEmpty(),
          QStringLiteral("Text message constructs"));
    check(hello.authorId == alice.id && hello.text == QStringLiteral("Hello!"),
          QStringLiteral("Message keeps author/text"));

    Reaction thumbsUp{QStringLiteral("\U0001F44D"), bob.id,
                      QDateTime::currentDateTimeUtc()};
    Reaction heart{QStringLiteral("\u2764\uFE0F"), alice.id,
                   QDateTime::currentDateTimeUtc()};
    hello.addReaction(thumbsUp);
    hello.addReaction(heart);
    check(hello.reactions.size() == 2, QStringLiteral("Message holds multiple reactions"));

    Attachment photo;
    photo.id = IdGenerator::create(QStringLiteral("att"));
    photo.type = AttachmentType::Image;
    photo.fileName = QStringLiteral("photo.png");
    photo.fileRef = QStringLiteral("media/photo.png");
    photo.mimeType = QStringLiteral("image/png");
    Message pic = Message::createMedia(alice.id, MessageType::Image,
                                       QStringLiteral("Look!"));
    pic.addAttachment(photo);
    check(pic.type == MessageType::Image && pic.attachments.size() == 1
              && pic.attachments.first().type == AttachmentType::Image,
          QStringLiteral("Image message holds attachment"));

    Attachment doc;
    doc.id = IdGenerator::create(QStringLiteral("att"));
    doc.type = AttachmentType::File;
    doc.fileName = QStringLiteral("notes.txt");
    Message fileMsg = Message::createMedia(bob.id, MessageType::File);
    fileMsg.addAttachment(doc);
    const Message sys = Message::createSystem(QStringLiteral("Bob joined"));
    check(fileMsg.type == MessageType::File && sys.type == MessageType::System
              && sys.authorId.isEmpty(),
          QStringLiteral("File/System message types construct"));

    // --- Conversations ---
    Conversation dm = Conversation::create(QStringLiteral("Alice + Bob"),
                                           ConversationType::Private);
    dm.addParticipant(alice.id);
    dm.addParticipant(bob.id);
    dm.addParticipant(alice.id); // duplicate must be ignored
    check(dm.participantIds().size() == 2 && dm.hasParticipant(alice.id),
          QStringLiteral("Conversation holds participants"));
    dm.addMessage(hello);
    dm.addMessage(pic);
    check(dm.messageCount() == 2, QStringLiteral("Conversation holds ordered messages"));
    check(dm.messages().at(0).order < dm.messages().at(1).order,
          QStringLiteral("Conversation assigns increasing order"));
    check(dm.findMessage(hello.id) != nullptr,
          QStringLiteral("Conversation finds message by ID"));

    Conversation group = Conversation::create(QStringLiteral("Squad"),
                                              ConversationType::Group);
    check(group.type() == ConversationType::Group,
          QStringLiteral("Group conversation type supported"));

    // --- Scenes ---
    Scene scene = Scene::create(QStringLiteral("Opening"), QStringLiteral("Intro"));
    scene.addEvent(SceneEvent::sendMessage(dm.id(), hello.id));
    scene.addEvent(SceneEvent::wait(500));
    scene.addEvent(SceneEvent::typing(alice.id, dm.id(), true));
    scene.addEvent(SceneEvent::typing(alice.id, dm.id(), false));
    scene.addEvent(SceneEvent::notification(QStringLiteral("Ping!")));
    check(scene.eventCount() == 5, QStringLiteral("Scene holds 5 events"));
    check(scene.events().at(0).type == SceneEventType::SendMessage
              && scene.events().at(1).type == SceneEventType::Wait
              && scene.events().at(2).type == SceneEventType::StartTyping
              && scene.events().at(3).type == SceneEventType::StopTyping
              && scene.events().at(4).type == SceneEventType::Notification,
          QStringLiteral("Scene covers all event types"));

    // --- Story ---
    Story story = Story::create(QStringLiteral("Demo"), QStringLiteral("0.1B check"));
    story.addCharacter(alice);
    story.addCharacter(bob);
    story.addConversation(dm);
    story.addConversation(group);
    story.addScene(scene);
    check(story.characterCount() == 2, QStringLiteral("Story holds characters"));
    check(story.conversationCount() == 2, QStringLiteral("Story holds conversations"));
    check(story.sceneCount() == 1, QStringLiteral("Story holds scenes"));
    check(story.findCharacter(alice.id) != nullptr
              && story.findConversation(dm.id()) != nullptr
              && story.findScene(scene.id()) != nullptr,
          QStringLiteral("Story lookups resolve collections"));

    // --- AppDefinition ---
    const AppDefinition app = AppDefinition::defaults();
    check(!app.id.isEmpty() && !app.appName.isEmpty(),
          QStringLiteral("AppDefinition has identity"));
    check(!app.theme.themeName.isEmpty() && !app.theme.accentColor.isEmpty(),
          QStringLiteral("AppDefinition carries theme info"));
    check(app.chat.fontSize > 0 && app.message.cornerRadius >= 0,
          QStringLiteral("AppDefinition carries chat/message appearance"));
    check(!app.navigation.barColor.isEmpty()
              && !app.notification.bannerColor.isEmpty(),
          QStringLiteral("AppDefinition carries navigation/notification appearance"));

    out << (g_failures == 0 ? "CORE-CHECK: ALL PASS\n" : "CORE-CHECK: FAILURES\n");
    out.flush();
    return g_failures == 0 ? 0 : 1;
}
