// KonataMessageEventCheck — 0.1E verification (no test framework).
// Part A drives ProjectDocument message/event CRUD: create/edit/delete,
// ordering, unknown-ID safety, save/reload round trip.
// Part B verifies offscreen wiring with the real widgets: message/event
// lists reflect data, nested selection opens the nested editors.
// Exit 0 = all pass.
#include <QApplication>
#include <QByteArray>
#include <QDir>
#include <QListWidget>
#include <QStackedWidget>
#include <QTemporaryDir>
#include <QTextStream>

#include "core/Id.h"
#include "document/ProjectDocument.h"
#include "ui/InspectorPanel.h"
#include "ui/authoring/ConversationEditorWidget.h"
#include "ui/authoring/MessageEditorWidget.h"
#include "ui/authoring/SceneEditorWidget.h"
#include "ui/authoring/SceneEventEditorWidget.h"

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

// The message list is the SingleSelection list inside a ConversationEditor;
// the participant list uses NoSelection.
QListWidget *messageListOf(ConversationEditorWidget *editor)
{
    const QList<QListWidget *> lists = editor->findChildren<QListWidget *>();
    for (QListWidget *list : lists) {
        if (list->selectionMode() == QAbstractItemView::SingleSelection)
            return list;
    }
    return nullptr;
}

QListWidget *eventListOf(SceneEditorWidget *editor)
{
    return editor->findChild<QListWidget *>();
}
} // namespace

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    QApplication app(argc, argv);

    QTextStream out(stdout);
    g_out = &out;
    IdGenerator::resetForTests();

    // ---------------- Part A: document message/event path ----------------
    ProjectDocument doc;
    check(doc.newProject(QStringLiteral("Message Event Test")),
          QStringLiteral("1. New project"));

    const Character alice = doc.addCharacter(QStringLiteral("Alice"), QStringLiteral("alice"));
    const Character bob = doc.addCharacter(QStringLiteral("Bob"), QStringLiteral("bob"));
    check(!alice.id.isEmpty() && !bob.id.isEmpty(),
          QStringLiteral("2. Two characters created"));

    Conversation conv = doc.addConversation(QStringLiteral("Chat"), ConversationType::Group);
    conv.setParticipantIds(QStringList{alice.id, bob.id});
    check(doc.updateConversation(conv),
          QStringLiteral("3. Participants added by Character ID"));

    // 4. Create message: stamped parent, order 1, default author = first participant
    const Message first = doc.createMessage(conv.id());
    check(!first.id.isEmpty() && first.conversationId == conv.id() && first.order == 1
              && first.authorId == alice.id && first.type == MessageType::Text,
          QStringLiteral("4. Message created with parent, order, default author, Text type"));

    // 5/6/7. Assign author, set type + text
    Message authored = first;
    authored.authorId = bob.id;
    authored.type = MessageType::Image;
    authored.text = QStringLiteral("Look at this");
    check(doc.updateMessage(conv.id(), authored),
          QStringLiteral("5/6. Author/type assignment applies"));
    const Message *back = doc.story().findConversation(conv.id())->findMessage(first.id);
    check(back && back->authorId == bob.id && back->type == MessageType::Image
              && back->text == QStringLiteral("Look at this"),
          QStringLiteral("7. Author, Image type, and caption read back"));

    // 8/9. More messages keep increasing, duplicate-free order
    const Message second = doc.createMessage(conv.id(), alice.id, MessageType::Text);
    const Message third = doc.createMessage(conv.id(), bob.id, MessageType::System);
    const QList<Message> &all =
        doc.story().findConversation(conv.id())->messages();
    check(all.size() == 3 && all.at(0).order == 1 && all.at(1).order == 2
              && all.at(2).order == 3 && second.order == 2 && third.order == 3,
          QStringLiteral("8/9. Additional messages ordered 1-2-3, no duplicates"));

    // 10. Edit message
    Message edited = second;
    edited.text = QStringLiteral("Edited text");
    check(doc.updateMessage(conv.id(), edited)
              && doc.story().findConversation(conv.id())->findMessage(second.id)->text
                  == QStringLiteral("Edited text"),
          QStringLiteral("10. Message edit persists"));

    // 11. Delete middle message: stable remaining orders, no renumbering
    check(doc.removeMessage(conv.id(), second.id),
          QStringLiteral("11. Message deletion succeeds"));
    const QList<Message> &after =
        doc.story().findConversation(conv.id())->messages();
    check(after.size() == 2 && after.at(0).order == 1 && after.at(1).order == 3
              && !doc.story().findConversation(conv.id())->findMessage(second.id),
          QStringLiteral("11b. Remaining orders stable (1,3), deleted gone"));

    // 12. Unknown IDs fail safely
    Message ghost = edited;
    ghost.id = QStringLiteral("msg_999999");
    check(!doc.updateMessage(QStringLiteral("conv_999999"), edited)
              && !doc.updateMessage(conv.id(), ghost)
              && !doc.removeMessage(conv.id(), QStringLiteral("msg_999999"))
              && !doc.removeMessage(QStringLiteral("conv_999999"), first.id)
              && doc.createMessage(QStringLiteral("conv_999999")).id.isEmpty(),
          QStringLiteral("12. Unknown message/conversation IDs fail safely"));

    // 13-18. Scene events of every type
    Scene scene = doc.addScene(QStringLiteral("Morning"));
    const SceneEvent send = doc.createEvent(scene.id(), SceneEventType::SendMessage);
    const SceneEvent wait = doc.createEvent(scene.id(), SceneEventType::Wait);
    const SceneEvent start = doc.createEvent(scene.id(), SceneEventType::StartTyping);
    const SceneEvent stop = doc.createEvent(scene.id(), SceneEventType::StopTyping);
    const SceneEvent note = doc.createEvent(scene.id(), SceneEventType::Notification);
    const QList<SceneEvent> &events = doc.story().findScene(scene.id())->events();
    check(events.size() == 5 && send.type == SceneEventType::SendMessage
              && wait.type == SceneEventType::Wait && wait.waitMs == 1000
              && start.type == SceneEventType::StartTyping
              && stop.type == SceneEventType::StopTyping
              && note.type == SceneEventType::Notification,
          QStringLiteral("13-18. All five event types created with sensible defaults"));

    // 19. Edit events
    SceneEvent waitEdit = wait;
    waitEdit.waitMs = 2500;
    SceneEvent noteEdit = note;
    noteEdit.text = QStringLiteral("Ping!");
    SceneEvent startEdit = start;
    startEdit.characterId = alice.id;
    startEdit.conversationId = conv.id();
    SceneEvent sendEdit = send;
    sendEdit.conversationId = conv.id();
    sendEdit.messageId = first.id;
    check(doc.updateEvent(scene.id(), waitEdit) && doc.updateEvent(scene.id(), noteEdit)
              && doc.updateEvent(scene.id(), startEdit)
              && doc.updateEvent(scene.id(), sendEdit),
          QStringLiteral("19. Event edits apply"));
    const SceneEvent *backWait = doc.story().findScene(scene.id())->findEvent(wait.id);
    const SceneEvent *backSend = doc.story().findScene(scene.id())->findEvent(send.id);
    check(backWait && backWait->waitMs == 2500
              && backSend && backSend->conversationId == conv.id()
              && backSend->messageId == first.id
              && doc.story().findScene(scene.id())->findEvent(note.id)->text
                  == QStringLiteral("Ping!"),
          QStringLiteral("19b. Wait duration, SendMessage refs, Notification text read back"));

    // 20/21. Delete event + unknown IDs
    check(doc.removeEvent(scene.id(), stop.id)
              && doc.story().findScene(scene.id())->eventCount() == 4,
          QStringLiteral("20. Event deletion succeeds"));
    SceneEvent ghostEvent = waitEdit;
    ghostEvent.id = QStringLiteral("evt_999999");
    check(!doc.updateEvent(QStringLiteral("scene_999999"), waitEdit)
              && !doc.updateEvent(scene.id(), ghostEvent)
              && !doc.removeEvent(scene.id(), QStringLiteral("evt_999999"))
              && doc.createEvent(QStringLiteral("scene_999999")).id.isEmpty(),
          QStringLiteral("21. Unknown event/scene IDs fail safely"));

    // 22-25. Save / open / verify field-for-field
    QTemporaryDir tempDir;
    check(tempDir.isValid(), QStringLiteral("Temp dir available"));
    const QString file = QDir(tempDir.path()).filePath(QStringLiteral("content.konata"));
    check(doc.saveAs(file), QStringLiteral("22. Project with content saves"));

    ProjectDocument reopened;
    check(reopened.open(file), QStringLiteral("23. Saved project reopens"));
    const Conversation *rConv = reopened.story().findConversation(conv.id());
    const Scene *rScene = reopened.story().findScene(scene.id());
    const Message *rFirst = rConv ? rConv->findMessage(first.id) : nullptr;
    check(rConv && rConv->messages().size() == 2 && rFirst
              && rFirst->authorId == bob.id && rFirst->type == MessageType::Image
              && rFirst->text == QStringLiteral("Look at this") && rFirst->order == 1,
          QStringLiteral("24. Messages survive save/reload field-for-field"));
    const SceneEvent *rWait = rScene ? rScene->findEvent(wait.id) : nullptr;
    const SceneEvent *rSend = rScene ? rScene->findEvent(send.id) : nullptr;
    check(rScene && rScene->events().size() == 4 && rWait && rWait->waitMs == 2500 && rWait->type
                  == SceneEventType::Wait
              && rSend && rSend->messageId == first.id
              && rScene->findEvent(note.id)->text == QStringLiteral("Ping!"),
          QStringLiteral("25. Events survive save/reload field-for-field"));

    // ---------------- Part B: offscreen widget wiring ----------------
    ProjectDocument uiDoc;
    InspectorPanel inspector;
    inspector.setDocument(&uiDoc);

    const Character c1 = uiDoc.addCharacter(QStringLiteral("C1"), QStringLiteral("c1"));
    Conversation uConv = uiDoc.addConversation(QStringLiteral("UC"), ConversationType::Private);
    uConv.setParticipantIds(QStringList{c1.id});
    uiDoc.updateConversation(uConv);
    const Message m1 = uiDoc.createMessage(uConv.id(), c1.id, MessageType::Text);
    const Message m2 = uiDoc.createMessage(uConv.id(), c1.id, MessageType::Text);
    Scene uScene = uiDoc.addScene(QStringLiteral("US"));
    const SceneEvent e1 = uiDoc.createEvent(uScene.id(), SceneEventType::Wait);

    ConversationEditorWidget *convEd = inspector.findChild<ConversationEditorWidget *>();
    MessageEditorWidget *msgEd = inspector.findChild<MessageEditorWidget *>();
    SceneEditorWidget *sceneEd = inspector.findChild<SceneEditorWidget *>();
    SceneEventEditorWidget *evtEd = inspector.findChild<SceneEventEditorWidget *>();
    QStackedWidget *stack = inspector.findChild<QStackedWidget *>();
    check(convEd && msgEd && sceneEd && evtEd && stack,
          QStringLiteral("B1. Inspector hosts message + event editors"));

    uiDoc.setSelection(ProjectDocument::SelectionType::Conversation, uConv.id());
    QListWidget *msgList = messageListOf(convEd);
    check(stack->currentWidget() == convEd && msgList && msgList->count() == 2,
          QStringLiteral("B2. Conversation shows structured message list (2 rows)"));

    uiDoc.setSelection(ProjectDocument::SelectionType::Message, m1.id, uConv.id());
    check(stack->currentWidget() == msgEd,
          QStringLiteral("B3. Message selection opens the message editor"));

    uiDoc.setSelection(ProjectDocument::SelectionType::Scene, uScene.id());
    QListWidget *evtList = eventListOf(sceneEd);
    check(stack->currentWidget() == sceneEd && evtList && evtList->count() == 1,
          QStringLiteral("B4. Scene shows structured event list (1 row)"));

    uiDoc.setSelection(ProjectDocument::SelectionType::SceneEvent, e1.id, uScene.id());
    check(stack->currentWidget() == evtEd,
          QStringLiteral("B5. Event selection opens the event editor"));

    // Lists track document selection both ways.
    uiDoc.setSelection(ProjectDocument::SelectionType::Message, m2.id, uConv.id());
    check(stack->currentWidget() == msgEd && msgList->selectedItems().size() == 1
              && msgList->selectedItems().first()->data(Qt::UserRole).toString() == m2.id,
          QStringLiteral("B6. Document message selection highlights its row"));

    msgList->setCurrentRow(0);
    check(uiDoc.selection().type == ProjectDocument::SelectionType::Message
              && uiDoc.selection().id == m1.id && stack->currentWidget() == msgEd,
          QStringLiteral("B7. Clicking a message row selects it and opens its editor"));

    uiDoc.removeMessage(uConv.id(), m1.id);
    check(msgList->count() == 1 && stack->currentWidget() != msgEd,
          QStringLiteral("B8. Message delete updates list and leaves the editor"));

    out << (g_failures == 0 ? "MESSAGE-EVENT-CHECK: ALL PASS\n"
                            : "MESSAGE-EVENT-CHECK: FAILURES\n");
    out.flush();
    return g_failures == 0 ? 0 : 1;
}
