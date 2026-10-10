// KonataRendererRefinementCheck — 0.2A.1 verification (no test framework).
// Part A: avatar resolution + anchor data through the renderer model,
// clamp rules, save/load round trip.
// Part B: real PreviewArea offscreen — avatar pixmaps/fallbacks, image box
// placement via proportional stretches, edit/resize behavior, dirty safety.
// Part C: real MessageEditorWidget exposes position controls for Image
// messages only. Test images are generated at runtime; no binary fixtures.
// Exit 0 = all pass.
#include <QApplication>
#include <QBoxLayout>
#include <QByteArray>
#include <QDir>
#include <QDoubleSpinBox>
#include <QImage>
#include <QLabel>
#include <QLayout>
#include <QTemporaryDir>
#include <QTextStream>

#include "core/Id.h"
#include "document/ProjectDocument.h"
#include "rendering/KonataRenderer.h"
#include "serialization/ProjectSerializer.h"
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
}

QList<QWidget *> renderedRows(PreviewArea *preview)
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

QWidget *findRowByText(PreviewArea *preview, const QString &text)
{
    for (QWidget *row : renderedRows(preview)) {
        QLabel *label = row->findChild<QLabel *>(QStringLiteral("RenderText"));
        if (label && label->text() == text)
            return row;
    }
    return nullptr;
}

// Horizontal spacer stretches around the thumbnail encode the authored X.
bool horizontalStretches(QWidget *row, int &left, int &right)
{
    QWidget *thumbRow = row->findChild<QWidget *>(QStringLiteral("RenderImageRow"));
    QBoxLayout *layout =
        thumbRow ? qobject_cast<QBoxLayout *>(thumbRow->layout()) : nullptr;
    if (!layout || layout->count() != 3)
        return false;
    left = layout->stretch(0);
    right = layout->stretch(2);
    return true;
}
} // namespace

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    QApplication app(argc, argv);

    QTextStream out(stdout);
    g_out = &out;
    IdGenerator::resetForTests();

    // Runtime-generated fixtures: solid red avatar, solid blue photo.
    QTemporaryDir tempDir;
    check(tempDir.isValid(), QStringLiteral("Temp dir available"));
    const QString avatarPath = QDir(tempDir.path()).filePath(QStringLiteral("avatar.png"));
    const QString photoPath = QDir(tempDir.path()).filePath(QStringLiteral("photo.png"));
    QImage avatarImage(64, 64, QImage::Format_RGB32);
    avatarImage.fill(Qt::red);
    QImage photoImage(200, 100, QImage::Format_RGB32);
    photoImage.fill(Qt::blue);
    check(avatarImage.save(avatarPath) && photoImage.save(photoPath),
          QStringLiteral("Runtime fixtures generated"));

    ProjectDocument doc;
    doc.newProject(QStringLiteral("Refinement Test"));
    Character alice = doc.addCharacter(QStringLiteral("Alice"), QStringLiteral("alice"));
    alice.avatarRef = avatarPath;
    check(doc.updateCharacter(alice), QStringLiteral("Avatar reference authored on Character"));
    const Character bob = doc.addCharacter(QStringLiteral("Bob"), QStringLiteral("bob"));

    Conversation conv = doc.addConversation(QStringLiteral("Chat"), ConversationType::Group);
    conv.setParticipantIds(QStringList{alice.id, bob.id});
    doc.updateConversation(conv);

    Message text = doc.createMessage(conv.id(), alice.id, MessageType::Text);
    text.text = QStringLiteral("Hi");
    doc.updateMessage(conv.id(), text);

    Message image = doc.createMessage(conv.id(), alice.id, MessageType::Image);
    image.text = QStringLiteral("Look!");
    Attachment attachment;
    attachment.id = IdGenerator::create(QStringLiteral("att"));
    attachment.type = AttachmentType::Image;
    attachment.fileName = QStringLiteral("photo.png");
    attachment.fileRef = photoPath;
    attachment.anchorX = 0.0;
    attachment.anchorY = 1.0;
    image.attachments.append(attachment);
    doc.updateMessage(conv.id(), image);

    Message legacy = doc.createMessage(conv.id(), bob.id, MessageType::Image);
    legacy.text = QStringLiteral("Old");
    doc.updateMessage(conv.id(), legacy); // no attachment: pre-0.2A.1 shape

    Message sys = doc.createMessage(conv.id(), QString(), MessageType::System);
    sys.text = QStringLiteral("Joined");
    doc.updateMessage(conv.id(), sys);

    Message orphan = doc.createMessage(conv.id(), QStringLiteral("char_999999"),
                                       MessageType::Text);
    orphan.text = QStringLiteral("Boo");
    doc.updateMessage(conv.id(), orphan);

    // ---------------- Part A: renderer model ----------------
    const RenderedConversation rendered =
        KonataRenderer::renderConversation(doc.story(), conv.id());
    check(rendered.valid && rendered.rows.size() == 5,
          QStringLiteral("Fixture conversation renders"));
    const RenderedMessage &textRow = rendered.rows.at(0);
    const RenderedMessage &imageRow = rendered.rows.at(1);
    const RenderedMessage &legacyRow = rendered.rows.at(2);
    const RenderedMessage &sysRow = rendered.rows.at(3);
    const RenderedMessage &orphanRow = rendered.rows.at(4);

    check(textRow.avatarRef == avatarPath && textRow.hasAvatar,
          QStringLiteral("1/2. Avatar reference resolves to the author's row"));
    check(!sysRow.hasAvatar && sysRow.avatarRef.isEmpty()
              && sysRow.authorName == QStringLiteral("System"),
          QStringLiteral("5. System messages skip avatar resolution"));
    check(orphanRow.authorMissing && !orphanRow.hasAvatar,
          QStringLiteral("4. Unknown author keeps fallback, no avatar, no crash"));

    const Message *backImage =
        doc.story().findConversation(conv.id())->findMessage(image.id);
    check(backImage && backImage->attachments.size() == 1
              && backImage->attachments.first().anchorX == 0.0
              && backImage->attachments.first().anchorY == 1.0,
          QStringLiteral("6. Image anchor created with the attachment"));
    Message moved = *backImage;
    moved.attachments.first().anchorX = 0.25;
    moved.attachments.first().anchorY = 0.75;
    check(doc.updateMessage(conv.id(), moved)
              && doc.story().findConversation(conv.id())->findMessage(image.id)->attachments.first()
                         .anchorX
                  == 0.25,
          QStringLiteral("7/8. Anchor X/Y edits persist"));

    check(Attachment::clampAnchor(qSNaN()) == 0.5
              && Attachment::clampAnchor(qInf()) == 0.5
              && Attachment::clampAnchor(-qInf()) == 0.5
              && Attachment::clampAnchor(-2.0) == 0.0
              && Attachment::clampAnchor(1.7) == 1.0
              && Attachment::clampAnchor(0.3) == 0.3,
          QStringLiteral("9. Invalid anchors handled: NaN/inf centered, rest clamped"));

    const RenderedConversation rerendered =
        KonataRenderer::renderConversation(doc.story(), conv.id());
    check(rerendered.rows.at(1).imageRef == photoPath
              && rerendered.rows.at(1).imageAnchorX == 0.25
              && rerendered.rows.at(1).imageAnchorY == 0.75,
          QStringLiteral("11. Renderer consumes the stored reference + position"));
    check(rerendered.rows.at(2).imageAnchorX == 0.5
              && rerendered.rows.at(2).imageAnchorY == 0.5
              && rerendered.rows.at(2).imageRef.isEmpty(),
          QStringLiteral("15. Legacy images get deterministic center default"));

    const QString file = QDir(tempDir.path()).filePath(QStringLiteral("refine.konata"));
    check(doc.saveAs(file), QStringLiteral("10a. Project saves"));
    ProjectDocument reopened;
    check(reopened.open(file), QStringLiteral("10b. Project reopens"));
    const Message *rImage =
        reopened.story().findConversation(conv.id())->findMessage(image.id);
    const Character *rAlice = reopened.story().findCharacter(alice.id);
    check(rImage && rImage->attachments.size() == 1
              && rImage->attachments.first().fileRef == photoPath
              && rImage->attachments.first().anchorX == 0.25
              && rImage->attachments.first().anchorY == 0.75 && rAlice
              && rAlice->avatarRef == avatarPath,
          QStringLiteral("10c. Reference + position + avatar survive save/load"));

    // ---------------- Part B: PreviewArea integration ----------------
    PreviewArea preview;
    preview.setDocument(&doc);
    preview.resize(500, 700);
    preview.show();
    doc.setSelection(ProjectDocument::SelectionType::Conversation, conv.id());

    QWidget *textWidget = findRowByText(&preview, QStringLiteral("Hi"));
    QLabel *aliceAvatar =
        textWidget ? textWidget->findChild<QLabel *>(QStringLiteral("RenderAvatar")) : nullptr;
    // Outgoing messages (first participant) have no per-message avatar in the
    // reference — header avatar represents the conversation. In the Group
    // fixture, "Hi" from alice (outgoing) should have no avatar.
    check(aliceAvatar == nullptr,
          QStringLiteral("18. Outgoing message correctly has no per-message avatar"));

    QWidget *bobWidget = findRowByText(&preview, QStringLiteral("Old"));
    QLabel *bobAvatar =
        bobWidget ? bobWidget->findChild<QLabel *>(QStringLiteral("RenderAvatar")) : nullptr;
    check(bobAvatar && !bobAvatar->pixmap(Qt::ReturnByValue).isNull(),
          QStringLiteral("3. Incoming group message renders circular avatar fallback"));

    QWidget *imageWidget = findRowByText(&preview, QStringLiteral("Look!"));
    QWidget *imageBox =
        imageWidget ? imageWidget->findChild<QWidget *>(QStringLiteral("RenderImageBox"))
                    : nullptr;
    QLabel *thumb = imageBox
        ? imageBox->findChild<QLabel *>(QStringLiteral("RenderImageThumb"))
        : nullptr;
    int left = -1;
    int right = -1;
    check(imageBox && thumb && thumb->text().isEmpty()
              && horizontalStretches(imageWidget, left, right) && left == 25 && right == 75,
          QStringLiteral("17. Positioned image renders loaded pixmap at authored anchor"));

    // Edit position through the document: preview follows, authoring untouched.
    Message shifted = *doc.story().findConversation(conv.id())->findMessage(image.id);
    shifted.attachments.first().anchorX = 1.0;
    shifted.attachments.first().anchorY = 0.0;
    check(doc.updateMessage(conv.id(), shifted),
          QStringLiteral("12a. Position edit applies"));
    int leftAfter = -1;
    int rightAfter = -1;
    check(horizontalStretches(findRowByText(&preview, QStringLiteral("Look!")), leftAfter,
                              rightAfter)
              && leftAfter == 100 && rightAfter == 0,
          QStringLiteral("12b. Edited position refreshes placement"));

    const QByteArray before =
        Konata::Serialization::ProjectSerializer::serialize(doc.project());
    const bool dirtyBefore = doc.isDirty();
    preview.resize(900, 400);
    preview.resize(320, 900);
    const Message *anchored =
        doc.story().findConversation(conv.id())->findMessage(image.id);
    int leftResized = -1;
    int rightResized = -1;
    check(anchored && anchored->attachments.first().anchorX == 1.0
              && horizontalStretches(findRowByText(&preview, QStringLiteral("Look!")),
                                     leftResized, rightResized)
              && leftResized == 100 && rightResized == 0
              && Konata::Serialization::ProjectSerializer::serialize(doc.project()) == before
              && doc.isDirty() == dirtyBefore,
          QStringLiteral("13/14. Resize preserves logical position, data, and dirty state"));

    QWidget *textRowWidget = findRowByText(&preview, QStringLiteral("Hi"));
    check(textRowWidget && !textRowWidget->findChild<QWidget *>(QStringLiteral("RenderImageBox")),
          QStringLiteral("19. Text messages unchanged: no image box"));
    QWidget *sysWidget = findRowByText(&preview, QStringLiteral("Joined"));
    check(sysWidget && !sysWidget->findChild<QLabel *>(QStringLiteral("RenderAvatar")),
          QStringLiteral("20. System rows stay avatar-free and safe"));

    // ---------------- Part C: editor controls ----------------
    MessageEditorWidget editor;
    editor.setDocument(&doc);
    editor.resize(300, 500);
    editor.show(); // offscreen: makes visibility assertions meaningful
    editor.setMessage(conv.id(), image.id);
    const QList<QDoubleSpinBox *> spins = editor.findChildren<QDoubleSpinBox *>();
    check(spins.size() == 2, QStringLiteral("16a. Image message exposes X/Y position controls"));
    editor.setMessage(conv.id(), text.id);
    bool positionVisible = false;
    for (QDoubleSpinBox *spin : editor.findChildren<QDoubleSpinBox *>()) {
        if (spin->isVisibleTo(&editor))
            positionVisible = true;
    }
    check(!positionVisible,
          QStringLiteral("16b. Text messages are not cluttered with position controls"));

    out << (g_failures == 0 ? "REFINEMENT-CHECK: ALL PASS\n"
                            : "REFINEMENT-CHECK: FAILURES\n");
    out.flush();
    return g_failures == 0 ? 0 : 1;
}
