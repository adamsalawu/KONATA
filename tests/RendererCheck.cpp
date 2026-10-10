// KonataRendererCheck — 0.2A verification (no test framework).
// Part A exercises KonataRenderer as a pure function: empty/ordered/
// resolved/fallback rendering, determinism, non-mutation.
// Part B drives the real PreviewArea offscreen with a ProjectDocument:
// selection switching, live refresh on edits, resize safety, scene
// stability, save/load render identity. No pixel comparisons anywhere.
// Exit 0 = all pass.
#include <QApplication>
#include <QByteArray>
#include <QDir>
#include <QLabel>
#include <QTemporaryDir>
#include <QTextStream>

#include "core/Id.h"
#include "document/ProjectDocument.h"
#include "rendering/KonataRenderer.h"
#include "serialization/ProjectSerializer.h"
#include "ui/PreviewArea.h"

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
    const QList<QWidget *> children = container->findChildren<QWidget *>(
        QString(), Qt::FindDirectChildrenOnly);
    for (QWidget *child : children) {
        if (child->objectName() == QStringLiteral("RenderRow")
            || child->objectName() == QStringLiteral("RenderRowSystem"))
            rows.append(child);
    }
    return rows;
}

QString rowAuthor(QWidget *row)
{
    QLabel *label = row->findChild<QLabel *>(QStringLiteral("RenderAuthor"));
    return label ? label->text() : QStringLiteral("<missing>");
}

QString rowText(QWidget *row)
{
    QLabel *label = row->findChild<QLabel *>(QStringLiteral("RenderText"));
    return label ? label->text() : QStringLiteral("<missing>");
}

QString renderSignature(const RenderedConversation &rendered)
{
    QString signature;
    for (const RenderedMessage &row : rendered.rows) {
        signature += row.messageId + QChar::fromLatin1('|') + row.authorName
            + QChar::fromLatin1('|') + row.text + QChar::fromLatin1('|')
            + QString::number(row.order) + QChar::fromLatin1(';');
    }
    return signature;
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
    doc.newProject(QStringLiteral("Render Test"));
    const Character konata = doc.addCharacter(QStringLiteral("Konata"), QStringLiteral("konata"));
    const Character kagami = doc.addCharacter(QStringLiteral("Kagami"), QStringLiteral("kagami"));
    Conversation conv = doc.addConversation(QStringLiteral("Group Chat"), ConversationType::Group);
    conv.setParticipantIds(QStringList{konata.id, kagami.id});
    doc.updateConversation(conv);

    // ---------------- Part A: pure renderer ----------------
    const RenderedConversation empty =
        KonataRenderer::renderConversation(doc.story(), conv.id());
    check(empty.valid && empty.empty && empty.rows.isEmpty()
              && empty.title == QStringLiteral("Group Chat")
              && empty.participantNames.size() == 2,
          QStringLiteral("1. Empty conversation renders safely with title + participants"));

    const RenderedConversation unknown =
        KonataRenderer::renderConversation(doc.story(), QStringLiteral("conv_999999"));
    check(!unknown.valid && unknown.rows.isEmpty(),
          QStringLiteral("2. Unknown conversation renders invalid, never invented"));

    Message hello = doc.createMessage(conv.id(), konata.id, MessageType::Text);
    hello.text = QStringLiteral("Hello");
    doc.updateMessage(conv.id(), hello);
    const RenderedConversation one =
        KonataRenderer::renderConversation(doc.story(), conv.id());
    check(!one.empty && one.rows.size() == 1 && one.rows.first().authorName == QStringLiteral("Konata")
              && one.rows.first().text == QStringLiteral("Hello")
              && one.rows.first().order == 1 && !one.rows.first().authorMissing,
          QStringLiteral("3. Single Text message renders author, text, order"));

    Message second = doc.createMessage(conv.id(), kagami.id, MessageType::Text);
    second.text = QStringLiteral("What's up?");
    doc.updateMessage(conv.id(), second);
    Message third = doc.createMessage(conv.id(), konata.id, MessageType::Text);
    third.text = QStringLiteral("Nothing much.");
    doc.updateMessage(conv.id(), third);
    const RenderedConversation multi =
        KonataRenderer::renderConversation(doc.story(), conv.id());
    check(multi.rows.size() == 3 && multi.rows.at(0).messageId == hello.id
              && multi.rows.at(1).messageId == second.id
              && multi.rows.at(2).messageId == third.id
              && multi.rows.at(0).order < multi.rows.at(1).order
              && multi.rows.at(1).order < multi.rows.at(2).order,
          QStringLiteral("4. Multiple messages render in Core order"));

    Message orphan = doc.createMessage(conv.id(), QStringLiteral("char_999999"),
                                       MessageType::Text);
    orphan.text = QStringLiteral("Boo");
    doc.updateMessage(conv.id(), orphan);
    const RenderedConversation withOrphan =
        KonataRenderer::renderConversation(doc.story(), conv.id());
    const RenderedMessage &orphanRow = withOrphan.rows.last();
    check(orphanRow.authorMissing && orphanRow.authorId == QStringLiteral("char_999999")
              && !orphanRow.authorName.isEmpty() && orphanRow.text == QStringLiteral("Boo"),
          QStringLiteral("5. Unknown author renders fallback, keeps ID, no crash"));
    doc.removeMessage(conv.id(), orphan.id);

    Message sys = doc.createMessage(conv.id(), QString(), MessageType::System);
    sys.text = QStringLiteral("Kagami joined");
    doc.updateMessage(conv.id(), sys);
    const RenderedConversation withSys =
        KonataRenderer::renderConversation(doc.story(), conv.id());
    check(withSys.rows.last().authorName == QStringLiteral("System")
              && withSys.rows.last().text == QStringLiteral("Kagami joined"),
          QStringLiteral("6. System message renders with System author"));
    doc.removeMessage(conv.id(), sys.id);

    Message pic = doc.createMessage(conv.id(), konata.id, MessageType::Image);
    pic.text = QStringLiteral("Look!");
    Attachment attachment;
    attachment.id = IdGenerator::create(QStringLiteral("att"));
    attachment.type = AttachmentType::Image;
    attachment.fileName = QStringLiteral("photo.png");
    attachment.fileRef = QStringLiteral("media/photo.png");
    pic.attachments.append(attachment);
    doc.updateMessage(conv.id(), pic);
    Message doc2 = doc.createMessage(conv.id(), kagami.id, MessageType::File);
    doc2.text = QStringLiteral("Readme");
    doc.updateMessage(conv.id(), doc2);
    const RenderedConversation withMedia =
        KonataRenderer::renderConversation(doc.story(), conv.id());
    const RenderedMessage &picRow = withMedia.rows.at(3);
    const RenderedMessage &docRow = withMedia.rows.at(4);
    check(picRow.isFallbackType && picRow.attachmentNote == QStringLiteral("photo.png")
              && picRow.text == QStringLiteral("Look!")
              && docRow.isFallbackType && docRow.text == QStringLiteral("Readme"),
          QStringLiteral("14. Image/File render caption + reference fallback, never crash"));
    doc.removeMessage(conv.id(), pic.id);
    doc.removeMessage(conv.id(), doc2.id);

    const QString firstPass = renderSignature(
        KonataRenderer::renderConversation(doc.story(), conv.id()));
    const QString secondPass = renderSignature(
        KonataRenderer::renderConversation(doc.story(), conv.id()));
    check(firstPass == secondPass && !firstPass.isEmpty(),
          QStringLiteral("8. Rendering is deterministic"));

    const QByteArray before = Konata::Serialization::ProjectSerializer::serialize(doc.project());
    const bool dirtyBefore = doc.isDirty();
    for (int i = 0; i < 5; ++i)
        KonataRenderer::renderConversation(doc.story(), conv.id());
    doc.setSelection(ProjectDocument::SelectionType::Conversation, conv.id());
    doc.clearSelection();
    KonataRenderer::renderConversation(doc.story(), QStringLiteral("conv_999999"));
    check(Konata::Serialization::ProjectSerializer::serialize(doc.project()) == before
              && doc.isDirty() == dirtyBefore,
          QStringLiteral("10/11. Rendering never mutates data or dirty state"));

    // ---------------- Part B: PreviewArea integration ----------------
    PreviewArea preview;
    preview.setDocument(&doc);
    preview.resize(500, 700);
    preview.show();

    doc.setSelection(ProjectDocument::SelectionType::Conversation, conv.id());
    {
        const QList<QWidget *> rows = renderedRows(&preview);
        check(rows.size() == 3 && rowAuthor(rows.at(0)) == QStringLiteral("Konata")
                  && rowText(rows.at(1)) == QStringLiteral("What's up?")
                  && rowAuthor(rows.at(2)) == QStringLiteral("Konata"),
              QStringLiteral("B1. Preview renders real messages from real data"));
        QLabel *title = preview.findChild<QLabel *>(QStringLiteral("RenderConvTitle"));
        check(title && title->text() == QStringLiteral("Group Chat"),
              QStringLiteral("B2. Preview shows the real conversation title"));
    }

    Message live = doc.story().findConversation(conv.id())->messages().first();
    live.text = QStringLiteral("Hello!!");
    doc.updateMessage(conv.id(), live);
    check(rowText(renderedRows(&preview).first()) == QStringLiteral("Hello!!"),
          QStringLiteral("7. Message edit refreshes the preview"));

    doc.removeMessage(conv.id(), second.id);
    check(renderedRows(&preview).size() == 2,
          QStringLiteral("7b. Message delete refreshes the preview"));

    const Message added = doc.createMessage(conv.id(), kagami.id, MessageType::Text);
    Message addedEdit = added;
    addedEdit.text = QStringLiteral("New here");
    doc.updateMessage(conv.id(), addedEdit);
    {
        const QList<QWidget *> rows = renderedRows(&preview);
        check(rows.size() == 3 && rowText(rows.last()) == QStringLiteral("New here"),
              QStringLiteral("8. New message appears in the preview"));
    }

    Conversation other = doc.addConversation(QStringLiteral("Side Chat"),
                                             ConversationType::Private);
    doc.setSelection(ProjectDocument::SelectionType::Conversation, other.id());
    {
        QLabel *title = preview.findChild<QLabel *>(QStringLiteral("RenderConvTitle"));
        const QList<QWidget *> rows = renderedRows(&preview);
        check(title && title->text() == QStringLiteral("Side Chat") && rows.isEmpty(),
              QStringLiteral("9. Switching conversations switches content; empty is safe"));
    }
    doc.setSelection(ProjectDocument::SelectionType::Conversation, conv.id());

    preview.resize(900, 400);
    preview.resize(320, 900);
    check(renderedRows(&preview).size() == 3,
          QStringLiteral("13. Preview resize does not crash or lose rows"));

    Scene scene = doc.addScene(QStringLiteral("Morning"));
    doc.createEvent(scene.id(), Konata::Core::SceneEventType::Wait);
    doc.setSelection(ProjectDocument::SelectionType::Scene, scene.id());
    {
        QWidget *scroll = preview.findChild<QWidget *>(QStringLiteral("RenderScroll"));
        const QList<QWidget *> rows = renderedRows(&preview);
        check(scroll && !scroll->isVisibleTo(&preview) && rows.isEmpty(),
              QStringLiteral("15. Scene selection shows no message render, no playback"));
    }

    QTemporaryDir tempDir;
    check(tempDir.isValid(), QStringLiteral("Temp dir available"));
    const QString file = QDir(tempDir.path()).filePath(QStringLiteral("render.konata"));
    check(doc.saveAs(file), QStringLiteral("12a. Project saves"));
    ProjectDocument reopened;
    check(reopened.open(file), QStringLiteral("12b. Project reopens"));
    const QString originalSig =
        renderSignature(KonataRenderer::renderConversation(doc.story(), conv.id()));
    const QString reloadedSig =
        renderSignature(KonataRenderer::renderConversation(reopened.story(), conv.id()));
    check(originalSig == reloadedSig && !originalSig.isEmpty(),
          QStringLiteral("12c. Save/load renders identically"));

    const bool dirtyBeforeView = reopened.isDirty();
    PreviewArea secondPreview;
    secondPreview.setDocument(&reopened);
    secondPreview.resize(600, 500);
    secondPreview.show();
    reopened.setSelection(ProjectDocument::SelectionType::Conversation, conv.id());
    reopened.setSelection(ProjectDocument::SelectionType::Scene, scene.id());
    reopened.clearSelection();
    check(reopened.isDirty() == dirtyBeforeView,
          QStringLiteral("17. Viewing/selecting never touches dirty state"));

    out << (g_failures == 0 ? "RENDERER-CHECK: ALL PASS\n" : "RENDERER-CHECK: FAILURES\n");
    out.flush();
    return g_failures == 0 ? 0 : 1;
}
