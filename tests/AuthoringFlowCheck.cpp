// KonataAuthoringCheck — 0.1D verification (no test framework).
// Part A drives ProjectDocument (the real authoring path): create/edit/
// delete through CRUD, selection, dirty tracking, save/reload round trip.
// Part B instantiates the real panels offscreen and verifies the signal
// wiring: tree reflects data, Inspector switches editors on selection.
// Exit 0 = all pass.
#include <QApplication>
#include <QByteArray>
#include <QDir>
#include <QStackedWidget>
#include <QTemporaryDir>
#include <QTextStream>
#include <QTreeWidget>

#include "core/Id.h"
#include "document/ProjectDocument.h"
#include "ui/InspectorPanel.h"
#include "ui/PreviewArea.h"
#include "ui/ProjectPanel.h"
#include "ui/authoring/CharacterEditorWidget.h"
#include "ui/authoring/ConversationEditorWidget.h"
#include "ui/authoring/SceneEditorWidget.h"

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

int countTopLevelItems(QTreeWidget *tree)
{
    return tree ? tree->topLevelItemCount() : -1;
}

int countGroupChildren(QTreeWidget *tree, const QString &group)
{
    if (!tree)
        return -1;
    const QList<QTreeWidgetItem *> found =
        tree->findItems(group, Qt::MatchExactly | Qt::MatchRecursive, 0);
    if (found.isEmpty())
        return -1;
    return found.first()->childCount();
}
} // namespace

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    QApplication app(argc, argv);

    QTextStream out(stdout);
    g_out = &out;
    IdGenerator::resetForTests();

    // ---------------- Part A: document authoring path ----------------
    ProjectDocument doc;
    int projectChangedCount = 0;
    int dirtyChangedCount = 0;
    int selectionChangedCount = 0;
    QObject::connect(&doc, &ProjectDocument::projectChanged, [&]() { ++projectChangedCount; });
    QObject::connect(&doc, &ProjectDocument::dirtyChanged, [&](bool) { ++dirtyChangedCount; });
    QObject::connect(&doc, &ProjectDocument::selectionChanged, [&]() { ++selectionChangedCount; });

    // 1. New project
    check(doc.newProject(QStringLiteral("Authoring Test")) && !doc.isDirty()
              && doc.story().characterCount() == 0,
          QStringLiteral("1. New project starts clean and empty"));

    // 2. Create character
    const Character alice = doc.addCharacter(QStringLiteral("Alice"), QStringLiteral("alice"));
    check(!alice.id.isEmpty() && doc.story().characterCount() == 1 && doc.isDirty(),
          QStringLiteral("2. Created character gets ID, story grows, dirty set"));

    // 3. Edit character
    Character renamed = alice;
    renamed.displayName = QStringLiteral("Alice Liddell");
    renamed.status = QStringLiteral("Curiouser");
    renamed.presence = Presence::Online;
    check(doc.updateCharacter(renamed),
          QStringLiteral("3. Character update applies"));
    const Character *back = doc.story().findCharacter(alice.id);
    check(back && back->displayName == QStringLiteral("Alice Liddell")
              && back->status == QStringLiteral("Curiouser")
              && back->presence == Presence::Online,
          QStringLiteral("3b. Edited fields read back from Story"));

    // 4. Second character
    const Character bob = doc.addCharacter(QStringLiteral("Bob"), QStringLiteral("bob"));
    check(doc.story().characterCount() == 2 && bob.id != alice.id,
          QStringLiteral("4. Second character with distinct ID"));

    // 5/6. Create conversation, add both as participants
    Conversation conv = doc.addConversation(QStringLiteral("Alice + Bob"),
                                            ConversationType::Private);
    conv.setParticipantIds(QStringList{alice.id, bob.id});
    check(doc.updateConversation(conv), QStringLiteral("5/6. Conversation created, update applies"));
    const Conversation *backConv = doc.story().findConversation(conv.id());
    check(backConv && backConv->type() == ConversationType::Private
              && backConv->hasParticipant(alice.id) && backConv->hasParticipant(bob.id),
          QStringLiteral("6b. Existing Character IDs stored as participants"));

    // 7. Remove a participant
    Conversation trimmed = *backConv;
    trimmed.removeParticipant(bob.id);
    check(doc.updateConversation(trimmed)
              && !doc.story().findConversation(conv.id())->hasParticipant(bob.id)
              && doc.story().findConversation(conv.id())->hasParticipant(alice.id),
          QStringLiteral("7. Participant removal persists"));

    // 8/9. Create + edit scene
    Scene scene = doc.addScene(QStringLiteral("Opening"));
    Scene editedScene = scene;
    editedScene.setName(QStringLiteral("Opening Act"));
    editedScene.setDescription(QStringLiteral("It begins."));
    check(doc.updateScene(editedScene), QStringLiteral("8/9. Scene created and edited"));
    const Scene *backScene = doc.story().findScene(scene.id());
    check(backScene && backScene->name() == QStringLiteral("Opening Act")
              && backScene->description() == QStringLiteral("It begins."),
          QStringLiteral("9b. Scene fields read back"));

    // 10. Selection
    doc.setSelection(ProjectDocument::SelectionType::Character, alice.id);
    check(doc.selection().type == ProjectDocument::SelectionType::Character
              && doc.selection().id == alice.id && selectionChangedCount == 1,
          QStringLiteral("10. Selection changes and notifies once"));

    // 11. Dirty transitions issued signals
    check(dirtyChangedCount >= 1 && projectChangedCount >= 8,
          QStringLiteral("11. projectChanged/dirtyChanged emitted along the way"));

    // 12/13. Save + reopen round trip
    QTemporaryDir tempDir;
    check(tempDir.isValid(), QStringLiteral("Temp dir available"));
    const QString file = QDir(tempDir.path()).filePath(QStringLiteral("authored.konata"));
    check(doc.saveAs(file) && !doc.isDirty(), QStringLiteral("12. Save As writes file, clears dirty"));

    ProjectDocument reopened;
    check(reopened.open(file), QStringLiteral("13. Saved project reopens"));
    const Character *rAlice = reopened.story().findCharacter(alice.id);
    const Conversation *rConv = reopened.story().findConversation(conv.id());
    const Scene *rScene = reopened.story().findScene(scene.id());
    check(rAlice && rAlice->displayName == QStringLiteral("Alice Liddell")
              && rConv && rConv->participantIds() == QStringList{alice.id}
              && rScene && rScene->description() == QStringLiteral("It begins."),
          QStringLiteral("13b. Authored data survives save/reload"));

    // 14/15. Delete character referenced nowhere now; then delete referenced one
    check(reopened.removeCharacter(bob.id) && reopened.story().characterCount() == 1,
          QStringLiteral("14. Delete removes the character"));
    reopened.setSelection(ProjectDocument::SelectionType::Character, alice.id);
    check(reopened.removeCharacter(alice.id)
              && reopened.selection().type == ProjectDocument::SelectionType::None,
          QStringLiteral("15. Deleting the selected object clears selection"));
    const Conversation *afterDelete = reopened.story().findConversation(conv.id());
    check(afterDelete && !afterDelete->hasParticipant(alice.id),
          QStringLiteral("15b. Removed IDs stripped from participants, stays consistent"));
    check(!reopened.updateCharacter(alice) && !reopened.removeScene(QStringLiteral("nope")),
          QStringLiteral("15c. Updating/removing unknown IDs fails cleanly"));

    // ---------------- Part B: real panel wiring (offscreen) ----------------
    ProjectDocument uiDoc;
    ProjectPanel panel;
    InspectorPanel inspector;
    PreviewArea preview;
    panel.setDocument(&uiDoc);
    inspector.setDocument(&uiDoc);
    preview.setDocument(&uiDoc);

    QTreeWidget *tree = panel.findChild<QTreeWidget *>();
    check(tree && countTopLevelItems(tree) == 1
              && countGroupChildren(tree, QStringLiteral("Characters")) == 0,
          QStringLiteral("B1. Empty project renders groups, no items"));

    const Character w1 = uiDoc.addCharacter(QStringLiteral("W1"), QStringLiteral("w1"));
    const Character w2 = uiDoc.addCharacter(QStringLiteral("W2"), QStringLiteral("w2"));
    const Conversation wc = uiDoc.addConversation(QStringLiteral("WC"), ConversationType::Group);
    const Scene ws = uiDoc.addScene(QStringLiteral("WS"));
    check(countGroupChildren(tree, QStringLiteral("Characters")) == 2
              && countGroupChildren(tree, QStringLiteral("Conversations")) == 1
              && countGroupChildren(tree, QStringLiteral("Scenes")) == 1,
          QStringLiteral("B2. Tree reflects created objects live"));

    QStackedWidget *stack = inspector.findChild<QStackedWidget *>();
    CharacterEditorWidget *charEd = inspector.findChild<CharacterEditorWidget *>();
    ConversationEditorWidget *convEd = inspector.findChild<ConversationEditorWidget *>();
    SceneEditorWidget *sceneEd = inspector.findChild<SceneEditorWidget *>();
    check(stack && charEd && convEd && sceneEd,
          QStringLiteral("B3. Inspector hosts all three editors"));

    uiDoc.setSelection(ProjectDocument::SelectionType::Conversation, wc.id());
    check(stack->currentWidget() == convEd,
          QStringLiteral("B4. Conversation selection opens conversation editor"));
    uiDoc.setSelection(ProjectDocument::SelectionType::Scene, ws.id());
    check(stack->currentWidget() == sceneEd,
          QStringLiteral("B5. Scene selection opens scene editor"));
    uiDoc.setSelection(ProjectDocument::SelectionType::Character, w1.id);
    check(stack->currentWidget() == charEd,
          QStringLiteral("B6. Character selection opens character editor"));
    uiDoc.clearSelection();
    check(stack->currentWidget() != charEd && stack->currentWidget() != convEd
              && stack->currentWidget() != sceneEd,
          QStringLiteral("B7. Cleared selection returns to empty state"));

    // Tree highlights follow selection without user clicks.
    uiDoc.setSelection(ProjectDocument::SelectionType::Character, w2.id);
    const QList<QTreeWidgetItem *> sel = tree->selectedItems();
    check(sel.size() == 1 && sel.first()->text(0) == QStringLiteral("W2"),
          QStringLiteral("B8. Tree selection tracks document selection"));

    uiDoc.removeCharacter(w2.id);
    check(countGroupChildren(tree, QStringLiteral("Characters")) == 1
              && stack->currentWidget() != charEd,
          QStringLiteral("B9. Delete updates tree and Inspector together"));

    out << (g_failures == 0 ? "AUTHORING-CHECK: ALL PASS\n" : "AUTHORING-CHECK: FAILURES\n");
    out.flush();
    return g_failures == 0 ? 0 : 1;
}
