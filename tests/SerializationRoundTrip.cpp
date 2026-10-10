// KonataSerializationCheck — 0.1C verification (no test framework).
// Builds a representative project, round-trips it through ProjectSerializer
// (memory + .konata file via ProjectManager), verifies data survival, checks
// ID continuity, and exercises error paths. Exit 0 = all pass.
#include <QByteArray>
#include <QDir>
#include <QTemporaryDir>
#include <QTextStream>

#include "core/Id.h"
#include "serialization/KonataProject.h"
#include "serialization/ProjectManager.h"
#include "serialization/ProjectSerializer.h"

using namespace Konata;
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

Serialization::KonataProject buildRepresentativeProject()
{
    Serialization::KonataProject project;
    project.story = Story::create(QStringLiteral("Round Trip"),
                                  QStringLiteral("0.1C verification story"));

    Character alice = Character::create(QStringLiteral("Alice"), QStringLiteral("alice"),
                                        QStringLiteral("avatars/alice.png"),
                                        QStringLiteral("Here!"), Presence::Online);
    Character bob = Character::create(QStringLiteral("Bob"), QStringLiteral("bob"),
                                      QString(), QStringLiteral("Away"),
                                      Presence::Away);
    Character cara = Character::create(QStringLiteral("Cara"), QStringLiteral("cara"),
                                       QStringLiteral("avatars/cara.png"), QString(),
                                       Presence::Busy);
    project.story.addCharacter(alice);
    project.story.addCharacter(bob);
    project.story.addCharacter(cara);

    Conversation dm = Conversation::create(QStringLiteral("Alice + Bob"),
                                           ConversationType::Private);
    dm.addParticipant(alice.id);
    dm.addParticipant(bob.id);

    Message hello = Message::createText(alice.id, QStringLiteral("Hello, Bob!"));
    hello.addReaction(Reaction{QStringLiteral("\U0001F44D"), bob.id, hello.createdAt});
    hello.addReaction(Reaction{QStringLiteral("\u2764\uFE0F"), alice.id, hello.createdAt});
    dm.addMessage(hello);

    Message pic = Message::createMedia(alice.id, MessageType::Image, QStringLiteral("Look!"));
    Attachment photo;
    photo.id = IdGenerator::create(QStringLiteral("att"));
    photo.type = AttachmentType::Image;
    photo.fileName = QStringLiteral("photo.png");
    photo.fileRef = QStringLiteral("media/photo.png");
    photo.mimeType = QStringLiteral("image/png");
    photo.sizeBytes = 12345;
    pic.addAttachment(photo);
    dm.addMessage(pic);
    dm.addMessage(Message::createSystem(QStringLiteral("Bob joined the chat")));
    project.story.addConversation(dm);

    Conversation squad = Conversation::create(QStringLiteral("Squad"), ConversationType::Group);
    squad.addParticipant(alice.id);
    squad.addParticipant(bob.id);
    squad.addParticipant(cara.id);
    squad.addMessage(Message::createText(cara.id, QStringLiteral("Squad up.")));
    Message plan = Message::createMedia(bob.id, MessageType::File, QStringLiteral("Plan"));
    Attachment doc;
    doc.id = IdGenerator::create(QStringLiteral("att"));
    doc.type = AttachmentType::File;
    doc.fileName = QStringLiteral("plan.txt");
    doc.fileRef = QStringLiteral("media/plan.txt");
    doc.mimeType = QStringLiteral("text/plain");
    doc.sizeBytes = 512;
    plan.addAttachment(doc);
    squad.addMessage(plan);
    project.story.addConversation(squad);

    Scene opening = Scene::create(QStringLiteral("Opening"), QStringLiteral("Intro"));
    opening.addEvent(SceneEvent::sendMessage(dm.id(), hello.id));
    opening.addEvent(SceneEvent::wait(750));
    opening.addEvent(SceneEvent::typing(bob.id, dm.id(), true));
    opening.addEvent(SceneEvent::typing(bob.id, dm.id(), false));
    opening.addEvent(SceneEvent::notification(QStringLiteral("Ping!")));
    project.story.addScene(opening);

    project.appDefinition = AppDefinition::defaults(QStringLiteral("Konata Chat"));
    project.appDefinition.packageId = QStringLiteral("tools.konata.chat");
    project.appDefinition.theme.themeName = QStringLiteral("Konata Dark");
    project.appDefinition.theme.accentColor = QStringLiteral("#4a90a4");
    project.appDefinition.theme.darkMode = true;
    project.appDefinition.navigation.barColor = QStringLiteral("#2d2d30");
    project.appDefinition.navigation.barHeight = 56;
    project.appDefinition.chat.backgroundColor = QStringLiteral("#1e1e1e");
    project.appDefinition.chat.fontSize = 15;
    project.appDefinition.message.outgoingBubbleColor = QStringLiteral("#094771");
    project.appDefinition.message.cornerRadius = 12;
    project.appDefinition.notification.enabled = true;
    project.appDefinition.notification.soundRef = QStringLiteral("sounds/ping.wav");

    return project;
}

bool messageEqual(const Message &a, const Message &b)
{
    return a.id == b.id && a.conversationId == b.conversationId && a.authorId == b.authorId
        && a.type == b.type && a.text == b.text
        && a.createdAt.toMSecsSinceEpoch() == b.createdAt.toMSecsSinceEpoch() && a.order == b.order
        && a.reactions.size() == b.reactions.size()
        && a.attachments.size() == b.attachments.size();
}
} // namespace

int main()
{
    QTextStream out(stdout);
    g_out = &out;

    IdGenerator::resetForTests();
    const Serialization::KonataProject original = buildRepresentativeProject();

    // --- In-memory round trip ---
    const QByteArray bytes = Serialization::ProjectSerializer::serialize(original);
    check(bytes.contains("\"formatVersion\": 1"), QStringLiteral("JSON carries format version"));

    const Serialization::LoadResult loaded =
        Serialization::ProjectSerializer::deserialize(bytes);
    check(loaded.ok, QStringLiteral("Round-trip deserializes: ")
                         + (loaded.ok ? QStringLiteral("ok") : loaded.error));
    if (!loaded.ok) {
        out << "SERIALIZATION-CHECK: FAILURES\n";
        out.flush();
        return 1;
    }

    const Story &a = original.story;
    const Story &b = loaded.project.story;
    check(b.id() == a.id() && b.title() == a.title() && b.description() == a.description(),
          QStringLiteral("Story identity/metadata survives"));
    check(b.createdAt().toMSecsSinceEpoch() == a.createdAt().toMSecsSinceEpoch(),
          QStringLiteral("Story createdAt survives"));
    check(b.characterCount() == 3, QStringLiteral("All characters survive"));
    const Character *loadedAlice = b.findCharacter(a.characters().at(0).id);
    check(loadedAlice && loadedAlice->displayName == QStringLiteral("Alice")
              && loadedAlice->username == QStringLiteral("alice")
              && loadedAlice->avatarRef == QStringLiteral("avatars/alice.png")
              && loadedAlice->status == QStringLiteral("Here!")
              && loadedAlice->presence == Presence::Online,
          QStringLiteral("Character fields incl. presence survive"));

    check(b.conversationCount() == 2, QStringLiteral("Both conversations survive"));
    const Conversation *loadedDm = b.findConversation(a.conversations().at(0).id());
    check(loadedDm && loadedDm->type() == ConversationType::Private
              && loadedDm->participantIds().size() == 2 && loadedDm->messageCount() == 3,
          QStringLiteral("Private conversation keeps type/participants/messages"));
    const Conversation *loadedSquad = b.findConversation(a.conversations().at(1).id());
    check(loadedSquad && loadedSquad->type() == ConversationType::Group
              && loadedSquad->participantIds().size() == 3,
          QStringLiteral("Group conversation keeps type/participants"));

    const Message &origHello = a.conversations().at(0).messages().at(0);
    const Message &backHello = loadedDm->messages().at(0);
    check(messageEqual(origHello, backHello), QStringLiteral("Message core fields survive"));
    check(backHello.reactions.size() == 2
              && backHello.reactions.at(0).emoji == QStringLiteral("\U0001F44D")
              && backHello.reactions.at(0).characterId
                  == a.characters().at(1).id // Bob's thumbs-up
              && backHello.reactions.at(1).emoji == QStringLiteral("\u2764\uFE0F"),
          QStringLiteral("Reactions survive with emoji + reactor"));
    const Message &backPic = loadedDm->messages().at(1);
    check(backPic.type == MessageType::Image && backPic.attachments.size() == 1
              && backPic.attachments.first().type == AttachmentType::Image
              && backPic.attachments.first().fileName == QStringLiteral("photo.png")
              && backPic.attachments.first().fileRef == QStringLiteral("media/photo.png")
              && backPic.attachments.first().mimeType == QStringLiteral("image/png")
              && backPic.attachments.first().sizeBytes == 12345,
          QStringLiteral("Image attachment fields survive"));
    check(loadedDm->messages().at(2).type == MessageType::System
              && loadedSquad->messages().at(1).type == MessageType::File,
          QStringLiteral("System/File message types survive"));

    check(b.sceneCount() == 1 && b.scenes().at(0).eventCount() == 5,
          QStringLiteral("Scene with 5 events survives"));
    const QList<SceneEvent> &ev = b.scenes().at(0).events();
    const QList<SceneEvent> &origEv = a.scenes().at(0).events();
    check(ev.at(0).type == SceneEventType::SendMessage && ev.at(0).messageId == origHello.id
              && ev.at(1).type == SceneEventType::Wait && ev.at(1).waitMs == 750
              && ev.at(2).type == SceneEventType::StartTyping
              && ev.at(3).type == SceneEventType::StopTyping
              && ev.at(4).type == SceneEventType::Notification
              && ev.at(4).text == QStringLiteral("Ping!")
              && ev.at(0).id == origEv.at(0).id,
          QStringLiteral("All scene event types + payloads survive"));

    const AppDefinition &appA = original.appDefinition;
    const AppDefinition &appB = loaded.project.appDefinition;
    check(appB.id == appA.id && appB.appName == appA.appName
              && appB.packageId == QStringLiteral("tools.konata.chat"),
          QStringLiteral("AppDefinition identity survives"));
    check(appB.theme.themeName == appA.theme.themeName
              && appB.theme.accentColor == QStringLiteral("#4a90a4") && appB.theme.darkMode
              && appB.navigation.barHeight == 56 && appB.chat.fontSize == 15
              && appB.message.outgoingBubbleColor == QStringLiteral("#094771")
              && appB.message.cornerRadius == 12 && appB.notification.enabled
              && appB.notification.soundRef == QStringLiteral("sounds/ping.wav"),
          QStringLiteral("AppDefinition theme/appearance survives"));

    // --- IDs stay unique after loading ---
    QStringList knownIds;
    knownIds << b.id();
    for (const Character &c : b.characters())
        knownIds << c.id;
    const Character fresh = Character::create(QStringLiteral("New"), QStringLiteral("new"));
    check(!knownIds.contains(fresh.id), QStringLiteral("New IDs do not collide after load"));

    // --- File round trip through ProjectManager ---
    QTemporaryDir tempDir;
    check(tempDir.isValid(), QStringLiteral("Temp dir available for file test"));
    const QString konataFile =
        QDir(tempDir.path()).filePath(QStringLiteral("roundtrip.konata"));

    Serialization::ProjectManager manager;
    check(manager.newProject().ok && !manager.isDirty() && !manager.hasFile(),
          QStringLiteral("ProjectManager starts a clean default project"));
    manager.setProject(original);
    check(manager.isDirty(), QStringLiteral("setProject marks dirty"));
    check(manager.saveAs(konataFile).ok && !manager.isDirty()
              && manager.filePath() == konataFile,
          QStringLiteral("ProjectManager Save As writes .konata"));
    check(manager.save().ok, QStringLiteral("ProjectManager Save reuses path"));

    Serialization::ProjectManager reopened;
    check(reopened.open(konataFile).ok && !reopened.isDirty()
              && reopened.filePath() == konataFile
              && reopened.project().story.characterCount() == 3
              && reopened.project().story.title() == QStringLiteral("Round Trip"),
          QStringLiteral("ProjectManager Open restores project"));

    Serialization::ProjectManager empty;
    empty.newProject();
    check(!empty.save().ok && !empty.lastError().isEmpty(),
          QStringLiteral("Save without path fails with an error"));

    // --- Error paths ---
    const Serialization::LoadResult missing =
        Serialization::ProjectSerializer::loadFromFile(
            QDir(tempDir.path()).filePath(QStringLiteral("does-not-exist.konata")));
    check(!missing.ok && !missing.error.isEmpty(),
          QStringLiteral("Nonexistent file is an error, not a crash"));

    const Serialization::LoadResult garbage =
        Serialization::ProjectSerializer::deserialize(QByteArray("{not json!!"));
    check(!garbage.ok && !garbage.error.isEmpty(),
          QStringLiteral("Invalid JSON is an error, not a crash"));

    QByteArray badVersion = bytes;
    badVersion.replace("\"formatVersion\": 1", "\"formatVersion\": 999");
    const Serialization::LoadResult versioned =
        Serialization::ProjectSerializer::deserialize(badVersion);
    check(!versioned.ok && versioned.error.contains(QStringLiteral("version"), Qt::CaseInsensitive),
          QStringLiteral("Unsupported format version is rejected"));

    const QByteArray missingStory =
        R"({"format":"konata-project","formatVersion":1,"appDefinition":{"id":"app_1","appName":"Y"}})";
    check(!Serialization::ProjectSerializer::deserialize(missingStory).ok,
          QStringLiteral("Missing story block is rejected"));

    const QByteArray missingId =
        R"({"format":"konata-project","formatVersion":1,)"
        R"("story":{"id":"story_1","title":"X","characters":[{"displayName":"No Id","username":"noid"}]},)"
        R"("appDefinition":{"id":"app_1","appName":"Y"}})";
    const Serialization::LoadResult noId =
        Serialization::ProjectSerializer::deserialize(missingId);
    check(!noId.ok && noId.error.contains(QStringLiteral("id")),
          QStringLiteral("Missing required character id is rejected"));

    const QByteArray badEnum =
        R"({"format":"konata-project","formatVersion":1,)"
        R"("story":{"id":"story_1","title":"X","conversations":[)"
        R"({"id":"conv_1","title":"C","type":"private","messages":[{"id":"msg_1","type":"telepathy"}]}]},)"
        R"("appDefinition":{"id":"app_1","appName":"Y"}})";
    const Serialization::LoadResult enumResult =
        Serialization::ProjectSerializer::deserialize(badEnum);
    check(!enumResult.ok && !enumResult.error.isEmpty(),
          QStringLiteral("Invalid enum value is rejected, not defaulted"));

    QByteArray badPresence = bytes;
    badPresence.replace("\"presence\": \"online\"", "\"presence\": \"levitating\"");
    check(badPresence != bytes
              && !Serialization::ProjectSerializer::deserialize(badPresence).ok,
          QStringLiteral("Invalid presence value is rejected"));

    out << (g_failures == 0 ? "SERIALIZATION-CHECK: ALL PASS\n" : "SERIALIZATION-CHECK: FAILURES\n");
    out.flush();
    return g_failures == 0 ? 0 : 1;
}
