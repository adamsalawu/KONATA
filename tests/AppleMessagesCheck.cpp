// KonataAppleMessagesCheck — 0.2C.1 verification (no test framework).
// Part A: Apple Messages skin factory (definition, screens, tokens, palette).
// Part B: renderer through the skin (direction, bubble resolution, token
// sensitivity, determinism, non-mutation, no name branching).
// Part C: real PreviewArea offscreen (bubbles, header, composer) + project
// round trip with the skin applied. No pixel comparisons anywhere.
// Exit 0 = all pass.
#include <QApplication>
#include <QByteArray>
#include <QDir>
#include <QLabel>
#include <QLineEdit>
#include <QTemporaryDir>
#include <QTextStream>

#include "core/AppDefinition.h"
#include "core/Id.h"
#include "document/ProjectDocument.h"
#include "rendering/KonataRenderer.h"
#include "rendering/skins/AppleMessagesSkin.h"
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

QList<QWidget *> bubblesOf(PreviewArea *preview, const QString &name)
{
    QList<QWidget *> found;
    QWidget *container = preview->findChild<QWidget *>(QStringLiteral("RenderRows"));
    if (!container)
        return found;
    const QList<QWidget *> children =
        container->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly);
    for (QWidget *child : children) {
        // Bubbles sit one level inside row frames; accept either depth.
        for (QWidget *bubble : child->findChildren<QWidget *>(name))
            found.append(bubble);
    }
    return found;
}

QString viewSignature(const RenderedAppView &view)
{
    QString signature = QString::number(static_cast<int>(view.screen.type));
    for (const RenderedMessage &row : view.conversation.rows) {
        signature += QChar::fromLatin1('|') + row.messageId
            + (row.outgoing ? QStringLiteral(":out") : QStringLiteral(":in"))
            + QChar::fromLatin1(':') + row.bubbleColor + QChar::fromLatin1(':')
            + QString::number(row.cornerRadius);
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

    // ---------------- Part A: skin definition ----------------
    const AppDefinition skin = AppleMessagesSkin::createAppDefinition();
    check(skin.appName == QStringLiteral("Apple Messages")
              && skin.packageId == QStringLiteral("com.apple.MobileSMS"),
          QStringLiteral("A1. Apple Messages identity configured"));
    check(skin.structure.navigation.style == NavigationStyle::Stack
              && skin.structure.navigation.screens.size() == 3
              && skin.structure.navigation.screens.at(0).title == QStringLiteral("Messages")
              && skin.structure.chat.composer.visible
              && skin.structure.chat.composer.placeholder == QStringLiteral("iMessage")
              && skin.structure.chat.header.showBackButton,
          QStringLiteral("A2. Screens, chat surface, and composer configured"));
    check(skin.tokens.cornerRadius == 18 && skin.tokens.fontSizeBody == 17
              && skin.tokens.fontSizeTitle == 17 && skin.tokens.avatarSize == 30
              && skin.structure.chat.messageArea.maxBubbleWidthPercent == 70
              && skin.structure.chat.messageArea.groupedCornerRadius == 6,
          QStringLiteral("A3. iOS type scale and layout tokens configured (30px message avatar, 6px grouped inner)"));
    check(skin.message.outgoingBubbleColor == QStringLiteral("#0A84FF")
              // Documented dark-received value (0.2C.2 reference grounding).
              && skin.message.incomingBubbleColor == QStringLiteral("#2C2C2E")
              && skin.message.outgoingTextColor == QStringLiteral("#FFFFFF")
              && skin.message.incomingTextColor == QStringLiteral("#FFFFFF")
              && skin.chat.backgroundColor == QStringLiteral("#000000")
              && skin.theme.accentColor == QStringLiteral("#0A84FF")
              && skin.theme.darkMode,
          QStringLiteral("A4. Centralized Apple dark palette, no renderer literals"));
    const AppDefinition neutral = AppleMessagesSkin::createNeutralAppDefinition();
    check(neutral.appName == QStringLiteral("Konata Chat")
              && neutral.message.cornerRadius == 12,
          QStringLiteral("A5. Neutral skin restores stock presentation"));

    // ---------------- Part B: renderer through the skin ----------------
    ProjectDocument doc;
    doc.newProject(QStringLiteral("Apple Test"));
    const Character me = doc.addCharacter(QStringLiteral("Me"), QStringLiteral("me"));
    const Character friend_ =
        doc.addCharacter(QStringLiteral("Friend"), QStringLiteral("friend"));
    Conversation conv = doc.addConversation(QStringLiteral("Friend"), ConversationType::Private);
    conv.setParticipantIds(QStringList{me.id, friend_.id});
    doc.updateConversation(conv);
    Message mine = doc.createMessage(conv.id(), me.id, MessageType::Text);
    mine.text = QStringLiteral("Hey");
    doc.updateMessage(conv.id(), mine);
    Message theirs = doc.createMessage(conv.id(), friend_.id, MessageType::Text);
    theirs.text = QStringLiteral("Hello!");
    doc.updateMessage(conv.id(), theirs);

    const Story &story = doc.story();
    const RenderedAppView view = KonataRenderer::renderAppView(story, skin, conv.id());
    check(view.valid && view.screen.type == ScreenType::Chat
              && view.conversation.rows.size() == 2
              && view.conversation.rows.at(0).outgoing
              && !view.conversation.rows.at(1).outgoing,
          QStringLiteral("B1. First participant renders outgoing, others incoming"));
    check(view.conversation.rows.at(0).bubbleColor == QStringLiteral("#0A84FF")
              && view.conversation.rows.at(1).bubbleColor == QStringLiteral("#2C2C2E")
              && view.conversation.rows.at(0).cornerRadius == 18
              && view.conversation.rows.at(0).bubbleTextColor == QStringLiteral("#FFFFFF"),
          QStringLiteral("B2. Bubble colors and radius resolve from the definition"));
    check(view.tokens.fontSizeBody == 17 && view.composerPlaceholder == QStringLiteral("iMessage"),
          QStringLiteral("B3. Tokens and composer placeholder travel with the view"));

    AppDefinition tweaked = skin;
    tweaked.message.cornerRadius = 8;
    tweaked.tokens.avatarSize = 48;
    const RenderedAppView tweakedView =
        KonataRenderer::renderAppView(story, tweaked, conv.id());
    check(tweakedView.conversation.rows.at(0).cornerRadius == 8
              && tweakedView.tokens.avatarSize == 48,
          QStringLiteral("B4. Token/definition edits change the model, not the Story"));

    check(viewSignature(view)
              == viewSignature(KonataRenderer::renderAppView(story, skin, conv.id())),
          QStringLiteral("B5. Identical input renders deterministically"));

    const QByteArray before =
        Konata::Serialization::ProjectSerializer::serialize(doc.project());
    const bool dirtyBefore = doc.isDirty();
    KonataRenderer::renderAppView(story, skin, conv.id());
    check(Konata::Serialization::ProjectSerializer::serialize(doc.project()) == before
              && doc.isDirty() == dirtyBefore,
          QStringLiteral("B6. Skinned rendering never mutates data or dirty state"));

    AppDefinition renamed = skin;
    renamed.appName = QStringLiteral("Totally Different App");
    const RenderedAppView renamedView =
        KonataRenderer::renderAppView(story, renamed, conv.id());
    check(renamedView.screen.type == view.screen.type
              && renamedView.screen.components == view.screen.components
              && viewSignature(renamedView) == viewSignature(view),
          QStringLiteral("B7. Same definition renders identically under any app name"));

    // ---------------- Part C: preview + persistence ----------------
    PreviewArea preview;
    preview.setDocument(&doc);
    preview.resize(500, 700);
    preview.show();

    QTemporaryDir cleanDir;
    const QString cleanFile =
        QDir(cleanDir.path()).filePath(QStringLiteral("baseline.konata"));
    check(cleanDir.isValid() && doc.saveAs(cleanFile) && !doc.isDirty(),
          QStringLiteral("C0. Clean baseline saved"));
    doc.applyAppDefinition(skin);
    check(doc.isDirty()
              && doc.project().appDefinition.appName == QStringLiteral("Apple Messages"),
          QStringLiteral("C1. Applying the skin is a normal dirty-marking mutation"));

    doc.setSelection(ProjectDocument::SelectionType::Conversation, conv.id());
    const QList<QWidget *> outgoing =
        bubblesOf(&preview, QStringLiteral("RenderBubbleOutgoing"));
    const QList<QWidget *> incoming =
        bubblesOf(&preview, QStringLiteral("RenderBubble"));
    check(outgoing.size() == 1 && incoming.size() == 1,
          QStringLiteral("C2. Preview shows outgoing + incoming bubbles"));
    // Bubbles are really painted now (RoundedBubble): assert the resolved
    // fill property instead of the old inline stylesheet (case-insensitive:
    // QColor::name() normalizes hex case).
    check(outgoing.first()->property("fillColorName").toString().compare(
                  QStringLiteral("#0A84FF"), Qt::CaseInsensitive)
                  == 0
              && incoming.first()->property("fillColorName").toString().compare(
                     QStringLiteral("#2C2C2E"), Qt::CaseInsensitive)
                  == 0,
          QStringLiteral("C3. Bubble fills come from the definition palette"));
    QLabel *back = preview.findChild<QLabel *>(QStringLiteral("RenderBack"));
    QLabel *headerAvatar =
        preview.findChild<QLabel *>(QStringLiteral("RenderHeaderAvatar"));
    QLabel *centerAvatar =
        preview.findChild<QLabel *>(QStringLiteral("RenderCenterAvatar"));
    QWidget *composer = preview.findChild<QWidget *>(QStringLiteral("RenderComposer"));
    QLineEdit *field = preview.findChild<QLineEdit *>(QStringLiteral("RenderComposerField"));
    // Reference: private header shows centered avatar (0.2D.3), group shows stack.
    // Accept either avatar location as long as a header avatar is visible.
    const bool hasHeaderAvatar =
        (headerAvatar && headerAvatar->isVisibleTo(&preview))
        || (centerAvatar && centerAvatar->isVisibleTo(&preview));
    check(back && back->isVisibleTo(&preview) && hasHeaderAvatar && composer
              && composer->isVisibleTo(&preview) && field
              && field->placeholderText() == QStringLiteral("iMessage"),
          QStringLiteral("C4. Header (back, avatar) and iMessage composer render"));

    QTemporaryDir tempDir;
    check(tempDir.isValid(), QStringLiteral("Temp dir available"));
    const QString file = QDir(tempDir.path()).filePath(QStringLiteral("apple.konata"));
    check(doc.saveAs(file), QStringLiteral("C5a. Skinned project saves"));
    ProjectDocument reopened;
    check(reopened.open(file)
              && reopened.project().appDefinition.message.outgoingBubbleColor
                  == QStringLiteral("#0A84FF")
              && reopened.project().appDefinition.tokens.fontSizeBody == 17
              && reopened.project().appDefinition.structure.chat.composer.placeholder
                  == QStringLiteral("iMessage"),
          QStringLiteral("C5b. Skin survives save/load and stays editable data"));

    out << (g_failures == 0 ? "APPLE-CHECK: ALL PASS\n" : "APPLE-CHECK: FAILURES\n");
    out.flush();
    return g_failures == 0 ? 0 : 1;
}
