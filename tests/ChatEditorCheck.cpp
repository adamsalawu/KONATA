// KonataChatEditorCheck — direct-chat authoring verification (no framework).
// Drives the real central workspace offscreen: conversation switching,
// row click select, Inspector editing, Add-button creation, context-action
// duplicate/delete/move, order stability, save/load, renderer purity, and
// the single-chat invariant (no switcher, one header). Exit 0 = all pass.
#include <QApplication>
#include <QByteArray>
#include <QDir>
#include <QLabel>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QTemporaryDir>
#include <QTextStream>

#include "core/Id.h"
#include "document/ProjectDocument.h"
#include "serialization/ProjectSerializer.h"
#include "ui/InspectorPanel.h"
#include "ui/PreviewArea.h"
#include "ui/authoring/MessageEditorWidget.h"

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
    g_out->flush();
}

QList<QWidget *> chatRows(PreviewArea *preview)
{
    QList<QWidget *> rows;
    QWidget *container = preview->findChild<QWidget *>(QStringLiteral("RenderRows"));
    if (!container)
        return rows;
    const QList<QWidget *> children =
        container->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly);
    for (QWidget *child : children) {
        if (child->objectName() == QStringLiteral("RenderRow")
            || child->objectName() == QStringLiteral("RenderRowSystem"))
            rows.append(child);
    }
    return rows;
}

QWidget *rowByText(PreviewArea *preview, const QString &text)
{
    for (QWidget *row : chatRows(preview)) {
        QLabel *label = row->findChild<QLabel *>(QStringLiteral("RenderText"));
        if (label && label->text() == text)
            return row;
    }
    return nullptr;
}

void mousePress(QWidget *widget)
{
    // Press only: selecting happens synchronously in the filter, which
    // rebuilds the rows — the release would target a deleted widget.
    QMouseEvent press(QEvent::MouseButtonPress, widget->rect().center(), Qt::LeftButton,
                      Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(widget, &press);
}

QAction *findAction(const QList<QAction *> &actions, const QString &text)
{
    for (QAction *action : actions) {
        if (action->text() == text)
            return action;
    }
    return nullptr;
}
} // namespace

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    QApplication app(argc, argv);

    QTextStream out(stdout);
    g_out = &out;
    IdGenerator::resetForTests();

    ProjectDocument doc;
    doc.newProject(QStringLiteral("Chat Editor Test"));
    const Character alice = doc.addCharacter(QStringLiteral("Alice"), QStringLiteral("alice"));
    const Character bob = doc.addCharacter(QStringLiteral("Bob"), QStringLiteral("bob"));
    Conversation convA = doc.addConversation(QStringLiteral("Chat"), ConversationType::Private);
    convA.setParticipantIds(QStringList{alice.id, bob.id});
    doc.updateConversation(convA);
    Message m1 = doc.createMessage(convA.id(), alice.id, MessageType::Text);
    m1.text = QStringLiteral("One");
    doc.updateMessage(convA.id(), m1);
    Message m2 = doc.createMessage(convA.id(), alice.id, MessageType::Text);
    m2.text = QStringLiteral("Two");
    doc.updateMessage(convA.id(), m2);
    Message m3 = doc.createMessage(convA.id(), bob.id, MessageType::Text);
    m3.text = QStringLiteral("Three");
    doc.updateMessage(convA.id(), m3);
    const Conversation convB = doc.addConversation(QStringLiteral("Solo"),
                                                   ConversationType::Private);

    PreviewArea preview;
    InspectorPanel inspector;
    preview.setDocument(&doc);
    inspector.setDocument(&doc);
    int editRequests = 0;
    QObject::connect(&preview, &PreviewArea::messageEditRequested,
                     [&]() { ++editRequests; });
    // Same wiring as MainWindow: edit requests focus the Inspector field.
    QObject::connect(&preview, &PreviewArea::messageEditRequested, &inspector,
                     &InspectorPanel::focusEditableField);
    preview.resize(500, 800);
    preview.show();
    QApplication::processEvents();

    // 1. Selecting a conversation opens it directly.
    doc.setSelection(ProjectDocument::SelectionType::Conversation, convA.id());
    QApplication::processEvents();
    QLabel *title = preview.findChild<QLabel *>(QStringLiteral("RenderConvTitle"));
    check(title && title->text() == QStringLiteral("Chat") && chatRows(&preview).size() == 3,
          QStringLiteral("1. Conversation selection opens its chat directly"));

    // 2. Selecting another replaces the current chat.
    doc.setSelection(ProjectDocument::SelectionType::Conversation, convB.id());
    QApplication::processEvents();
    check(title && title->text() == QStringLiteral("Solo")
              && chatRows(&preview).isEmpty(),
          QStringLiteral("2. Switching conversations replaces the chat"));
    doc.setSelection(ProjectDocument::SelectionType::Conversation, convA.id());
    QApplication::processEvents();

    // 3. Row click selects the correct stable ID.
    QWidget *firstRow = chatRows(&preview).first();
    const QString firstId = firstRow->property("messageId").toString();
    check(!firstId.isEmpty() && firstId == m1.id,
          QStringLiteral("3a. Rows carry their stable Core IDs"));
    mousePress(firstRow);
    QApplication::processEvents();
    check(doc.selection().type == ProjectDocument::SelectionType::Message
              && doc.selection().id == m1.id && doc.selection().parentId == convA.id(),
          QStringLiteral("3b. Clicking a row selects that exact message"));
    MessageEditorWidget *messageEditor =
        inspector.findChild<MessageEditorWidget *>();
    QStackedWidget *inspectorStack = inspector.findChild<QStackedWidget *>();
    check(messageEditor && inspectorStack && inspectorStack->currentWidget() == messageEditor,
          QStringLiteral("3c. Selection opens the message editor in the Inspector"));

    // Double-click requests Inspector editing for the same message.
    // (Re-resolve: the click above rebuilt the rows.)
    QWidget *freshFirstRow = rowByText(&preview, QStringLiteral("One"));
    QMouseEvent doubleClick(QEvent::MouseButtonDblClick, freshFirstRow->rect().center(),
                            Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(freshFirstRow, &doubleClick);
    QApplication::processEvents();
    check(editRequests == 1 && doc.selection().id == m1.id,
          QStringLiteral("3d. Double-click keeps selection, requests Inspector edit"));

    // 4. Editing through the real Inspector changes Core data + chat.
    QPlainTextEdit *inspectorText = messageEditor->findChild<QPlainTextEdit *>();
    check(inspectorText != nullptr, QStringLiteral("4a. Inspector exposes the text field"));
    inspectorText->setPlainText(QStringLiteral("One (edited)"));
    QApplication::processEvents();
    check(doc.story().findConversation(convA.id())->findMessage(m1.id)->text
                  == QStringLiteral("One (edited)")
              && rowByText(&preview, QStringLiteral("One (edited)")) != nullptr,
          QStringLiteral("4b. Inspector edit updates Core data and the chat row"));

    // 5. Add button creates real Core messages.
    QPushButton *addButton =
        preview.findChild<QPushButton *>(QStringLiteral("ChatAddButton"));
    check(addButton != nullptr, QStringLiteral("5a. Chat offers an Add Message control"));
    const int countBefore = doc.story().findConversation(convA.id())->messageCount();
    addButton->click();
    QApplication::processEvents();
    const Conversation *grown = doc.story().findConversation(convA.id());
    check(grown->messageCount() == countBefore + 1
              && doc.selection().type == ProjectDocument::SelectionType::Message
              && grown->findMessage(doc.selection().id) != nullptr,
          QStringLiteral("5b. Add creates a real message and selects it"));
    const QString addedId = doc.selection().id;
    doc.removeMessage(convA.id(), addedId); // restore fixture shape
    QApplication::processEvents();

    // 6. Context actions duplicate/delete through the document.
    QList<QAction *> actions = preview.rowActions(convA.id(), m2.id);
    QAction *duplicateAction = findAction(actions, QStringLiteral("Duplicate"));
    QAction *deleteAction = findAction(actions, QStringLiteral("Delete"));
    QAction *moveUpAction = findAction(actions, QStringLiteral("Move Up"));
    QAction *moveDownAction = findAction(actions, QStringLiteral("Move Down"));
    check(actions.size() == 6 && duplicateAction && deleteAction && moveUpAction
              && moveDownAction,
          QStringLiteral("6a. Rows offer Edit/Duplicate/Delete/Move/Copy actions"));
    duplicateAction->trigger();
    QApplication::processEvents();
    const Conversation *duplicated = doc.story().findConversation(convA.id());
    check(duplicated->messageCount() == 4
              && duplicated->messages().at(2).text == QStringLiteral("Two")
              && duplicated->messages().at(2).id != m2.id,
          QStringLiteral("6b. Duplicate inserts a fresh-ID copy after the original"));
    const QString duplicateId = duplicated->messages().at(2).id;
    qDeleteAll(actions);
    QList<QAction *> deleteActions = preview.rowActions(convA.id(), duplicateId);
    findAction(deleteActions, QStringLiteral("Delete"))->trigger();
    QApplication::processEvents();
    check(doc.story().findConversation(convA.id())->messageCount() == 3
              && !doc.story().findConversation(convA.id())->findMessage(duplicateId),
          QStringLiteral("6c. Delete removes the message from Core and chat"));
    qDeleteAll(deleteActions);

    // 7. Order stays stable: moves swap positions, IDs survive.
    QList<QAction *> moveActions = preview.rowActions(convA.id(), m1.id);
    check(!findAction(moveActions, QStringLiteral("Move Up"))->isEnabled()
              && findAction(moveActions, QStringLiteral("Move Down"))->isEnabled(),
          QStringLiteral("7a. Move bounds disable correctly at the edges"));
    findAction(moveActions, QStringLiteral("Move Down"))->trigger();
    QApplication::processEvents();
    const QList<Message> reordered = doc.story().findConversation(convA.id())->messages();
    check(reordered.at(0).id == m2.id && reordered.at(1).id == m1.id
              && reordered.at(0).order == 1 && reordered.at(1).order == 2
              && !doc.moveMessage(convA.id(), QStringLiteral("msg_nope"), 1)
              && !doc.moveMessage(convA.id(), m1.id, 5),
          QStringLiteral("7b. Move swaps positions+orders; bad moves fail safely"));
    qDeleteAll(moveActions);
    doc.moveMessage(convA.id(), m1.id, -1); // restore Two, One-edited, Three
    QApplication::processEvents();

    // 8. Save/load preserves the edited conversation.
    QTemporaryDir tempDir;
    check(tempDir.isValid(), QStringLiteral("Temp dir available"));
    const QString file = QDir(tempDir.path()).filePath(QStringLiteral("chat.konata"));
    check(doc.saveAs(file), QStringLiteral("8a. Edited project saves"));
    ProjectDocument reopened;
    check(reopened.open(file), QStringLiteral("8b. Saved project reopens"));
    const Conversation *reloaded = reopened.story().findConversation(convA.id());
    check(reloaded && reloaded->messageCount() == 3
              && reloaded->findMessage(m1.id)->text == QStringLiteral("One (edited)"),
          QStringLiteral("8c. Edited conversation survives the round trip"));

    // 9. Renderer stays non-mutating (serialize compare + dirty flag).
    const QByteArray before =
        Konata::Serialization::ProjectSerializer::serialize(doc.project());
    const bool dirtyBefore = doc.isDirty();
    doc.setSelection(ProjectDocument::SelectionType::Conversation, convB.id());
    doc.setSelection(ProjectDocument::SelectionType::Conversation, convA.id());
    doc.setSelection(ProjectDocument::SelectionType::Message, m3.id, convA.id());
    QApplication::processEvents();
    check(Konata::Serialization::ProjectSerializer::serialize(doc.project()) == before
              && doc.isDirty() == dirtyBefore,
          QStringLiteral("9. Viewing and selecting never mutate or dirty"));

    // 10. Single navigation invariant: no competing chat tabs anywhere.
    check(preview.findChild<QWidget *>(QStringLiteral("ScreenSwitcher")) == nullptr
              && preview.findChildren<QWidget *>(QStringLiteral("RenderListRow")).isEmpty()
              && preview.findChild<QWidget *>(QStringLiteral("RenderProfileCard")) == nullptr
              && preview.findChildren<QWidget *>(QStringLiteral("RenderTitleBar")).size() == 1,
          QStringLiteral("10. One chat editor, one header, no duplicate navigation"));

    out << (g_failures == 0 ? "CHATEDITOR-CHECK: ALL PASS\n"
                            : "CHATEDITOR-CHECK: FAILURES\n");
    out.flush();
    return g_failures == 0 ? 0 : 1;
}
