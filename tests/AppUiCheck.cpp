// KonataAppUiCheck — 0.2B verification (no test framework).
// Part A: App UI model creation and configuration (screens, navigation,
// chat layout, tokens).
// Part B: serialization round trip, backward compatibility with pre-0.2B
// files, malformed-value rejection.
// Part C: renderer integration — screen resolution, derived components,
// determinism, non-mutation, and proof that screen choice never depends on
// application names.
// Part D: PreviewArea pipeline glue offscreen.
// Exit 0 = all pass.
#include <QApplication>
#include <QByteArray>
#include <QDir>
#include <QLabel>
#include <QTemporaryDir>
#include <QTextStream>

#include "core/AppDefinition.h"
#include "core/AppUi.h"
#include "core/Id.h"
#include "document/ProjectDocument.h"
#include "rendering/KonataRenderer.h"
#include "serialization/ProjectSerializer.h"
#include "ui/PreviewArea.h"

using namespace Konata::Core;
using Konata::Serialization::KonataProject;

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

bool hasComponent(const RenderedScreen &screen, ComponentType component)
{
    return screen.components.contains(component);
}

QString viewSignature(const RenderedAppView &view)
{
    QString signature = QString::number(static_cast<int>(view.screen.type))
        + QChar::fromLatin1('|') + view.screen.title + QChar::fromLatin1('|');
    for (ComponentType component : view.screen.components)
        signature += QString::number(static_cast<int>(component)) + QChar::fromLatin1(',');
    signature += QChar::fromLatin1('|') + view.conversation.conversationId;
    for (const RenderedMessage &row : view.conversation.rows)
        signature += QChar::fromLatin1('|') + row.messageId + QChar::fromLatin1(':')
            + row.authorName + QChar::fromLatin1(':') + row.text;
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

    // ---------------- Part A: model creation ----------------
    AppDefinition fresh = AppDefinition::defaults(QStringLiteral("TestApp"));
    check(fresh.structure.navigation.style == NavigationStyle::Tabs
              && fresh.structure.navigation.screens.size() == 3
              && fresh.structure.navigation.screens.at(0).type
                  == ScreenType::ConversationList
              && fresh.structure.navigation.screens.at(1).type == ScreenType::Chat
              && fresh.structure.navigation.screens.at(2).type == ScreenType::Profile,
          QStringLiteral("A1. Default navigation offers list, chat, profile tabs"));
    check(fresh.structure.chat.header.showAvatar
              && fresh.structure.chat.messageArea.groupConsecutive
              && fresh.structure.chat.composer.visible
              && fresh.structure.conversationList.showAvatars
              && fresh.structure.profile.showStatus,
          QStringLiteral("A2. Chat, list, and profile layouts have sane defaults"));
    check(fresh.tokens.spacingSmall == 4 && fresh.tokens.spacingLarge == 16
              && fresh.tokens.cornerRadius == 12 && fresh.tokens.avatarSize == 36
              && fresh.tokens.fontSizeBody == 14,
          QStringLiteral("A3. Design tokens carry restrained defaults"));

    AppDefinition configured = AppDefinition::defaults(QStringLiteral("Configured"));
    configured.structure.navigation.style = NavigationStyle::Sidebar;
    ScreenDefinition custom;
    custom.type = ScreenType::Custom;
    custom.title = QStringLiteral("Search");
    custom.customId = QStringLiteral("search");
    configured.structure.navigation.screens.append(custom);
    configured.structure.navigation.screens[1].enabled = false; // hide stock Chat
    configured.structure.chat.messageArea.groupConsecutive = false;
    configured.structure.chat.messageArea.maxBubbleWidthPercent = 60;
    configured.structure.chat.composer.placeholder = QStringLiteral("Type here");
    configured.tokens.avatarSize = 48;
    configured.tokens.cornerRadius = 8;
    check(configured.structure.navigation.screens.size() == 4
              && configured.structure.navigation.screens.last().customId
                  == QStringLiteral("search")
              && !configured.structure.navigation.screens.at(1).enabled
              && configured.structure.chat.messageArea.maxBubbleWidthPercent == 60
              && configured.tokens.avatarSize == 48,
          QStringLiteral("A4. Navigation, chat layout, and tokens are configurable"));

    // ---------------- Part B: serialization ----------------
    KonataProject project;
    project.story = Story::create(QStringLiteral("App UI Test"));
    project.appDefinition = configured;
    const QByteArray bytes =
        Konata::Serialization::ProjectSerializer::serialize(project);
    check(bytes.contains("\"structure\"") && bytes.contains("\"tokens\"")
              && bytes.contains("\"conversationList\"") && bytes.contains("\"customId\""),
          QStringLiteral("B1. Structure and tokens persist with explicit field names"));

    const Konata::Serialization::LoadResult loaded =
        Konata::Serialization::ProjectSerializer::deserialize(bytes);
    check(loaded.ok, QStringLiteral("B2. Extended project deserializes"));
    const AppDefinition &back = loaded.project.appDefinition;
    check(back.structure.navigation.style == NavigationStyle::Sidebar
              && back.structure.navigation.screens.size() == 4
              && back.structure.navigation.screens.last().customId
                  == QStringLiteral("search")
              && !back.structure.navigation.screens.at(1).enabled
              && back.structure.chat.messageArea.groupConsecutive == false
              && back.structure.chat.messageArea.maxBubbleWidthPercent == 60
              && back.structure.chat.composer.placeholder == QStringLiteral("Type here")
              && back.tokens.avatarSize == 48 && back.tokens.cornerRadius == 8
              && back.theme.themeName == configured.theme.themeName
              && back.message.outgoingBubbleColor
                  == configured.message.outgoingBubbleColor,
          QStringLiteral("B3. Structure, tokens, and legacy appearance survive field-for-field"));

    // Pre-0.2B file: no structure/tokens blocks at all.
    const QByteArray legacyProject =
        R"({"format":"konata-project","formatVersion":1,)"
        R"("story":{"id":"story_000001","title":"Old"},)"
        R"("appDefinition":{"id":"app_000002","appName":"Old","theme":{"themeName":"Custom"}}})";
    const Konata::Serialization::LoadResult legacy =
        Konata::Serialization::ProjectSerializer::deserialize(legacyProject);
    check(legacy.ok && legacy.project.appDefinition.structure.navigation.screens.size() == 3
              && legacy.project.appDefinition.tokens.avatarSize == 36
              && legacy.project.appDefinition.theme.themeName == QStringLiteral("Custom"),
          QStringLiteral("B4. Older files load with defaults, existing data kept"));

    const QByteArray badScreen =
        R"({"format":"konata-project","formatVersion":1,)"
        R"("story":{"id":"story_1","title":"X"},)"
        R"("appDefinition":{"id":"app_1","appName":"Y",)"
        R"("structure":{"navigation":{"screens":[{"type":"teleport"}]}}}})";
    const QByteArray badStyle =
        R"({"format":"konata-project","formatVersion":1,)"
        R"("story":{"id":"story_1","title":"X"},)"
        R"("appDefinition":{"id":"app_1","appName":"Y",)"
        R"("structure":{"navigation":{"style":"maze"}}}})";
    const QByteArray badBlock =
        R"({"format":"konata-project","formatVersion":1,)"
        R"("story":{"id":"story_1","title":"X"},)"
        R"("appDefinition":{"id":"app_1","appName":"Y","tokens":[1,2]}})";
    check(!Konata::Serialization::ProjectSerializer::deserialize(badScreen).ok
              && !Konata::Serialization::ProjectSerializer::deserialize(badStyle).ok
              && !Konata::Serialization::ProjectSerializer::deserialize(badBlock).ok,
          QStringLiteral("B5. Malformed screen type, style, and blocks are rejected"));

    // ---------------- Part C: renderer integration ----------------
    ProjectDocument doc;
    doc.newProject(QStringLiteral("App View Test"));
    const Character alice = doc.addCharacter(QStringLiteral("Alice"), QStringLiteral("alice"));
    Conversation conv = doc.addConversation(QStringLiteral("Chat"), ConversationType::Private);
    conv.setParticipantIds(QStringList{alice.id});
    doc.updateConversation(conv);
    Message hello = doc.createMessage(conv.id(), alice.id, MessageType::Text);
    hello.text = QStringLiteral("Hi");
    doc.updateMessage(conv.id(), hello);
    Message pic = doc.createMessage(conv.id(), alice.id, MessageType::Image);
    doc.updateMessage(conv.id(), pic);

    KonataProject renderProject;
    renderProject.story = doc.story();
    renderProject.appDefinition = AppDefinition::defaults(QStringLiteral("Neutral"));
    const Story &story = renderProject.story;
    const AppDefinition &renderApp = renderProject.appDefinition;

    const RenderedAppView chatView =
        KonataRenderer::renderAppView(story, renderApp, conv.id());
    check(chatView.valid && chatView.appName == QStringLiteral("Neutral")
              && chatView.screen.type == ScreenType::Chat
              && chatView.conversation.valid && chatView.conversation.rows.size() == 2
              && hasComponent(chatView.screen, ComponentType::Header)
              && hasComponent(chatView.screen, ComponentType::MessageBubble)
              && hasComponent(chatView.screen, ComponentType::ImageMessage)
              && hasComponent(chatView.screen, ComponentType::Composer),
          QStringLiteral("C1. Chat screen resolves with payload-derived components"));
    check(chatView.tokens.avatarSize == 36 && chatView.tokens.fontSizeBody == 14,
          QStringLiteral("C2. Resolved tokens travel with the view"));

    AppDefinition noComposer = renderApp;
    noComposer.structure.chat.composer.visible = false;
    const RenderedAppView quietView =
        KonataRenderer::renderAppView(story, noComposer, conv.id());
    check(!hasComponent(quietView.screen, ComponentType::Composer)
              && hasComponent(quietView.screen, ComponentType::Header),
          QStringLiteral("C3. Component list follows configuration, not assumptions"));

    const RenderedAppView listView =
        KonataRenderer::renderAppView(story, renderApp, QStringLiteral("conv_999999"));
    check(listView.valid && listView.screen.type == ScreenType::ConversationList
              && hasComponent(listView.screen, ComponentType::ConversationRow)
              && listView.conversation.rows.isEmpty(),
          QStringLiteral("C4. Unresolvable conversation falls back to the list screen"));

    check(viewSignature(chatView)
              == viewSignature(KonataRenderer::renderAppView(story, renderApp, conv.id())),
          QStringLiteral("C5. Same Story + AppDefinition renders deterministically"));

    const QByteArray before =
        Konata::Serialization::ProjectSerializer::serialize(doc.project());
    const bool dirtyBefore = doc.isDirty();
    KonataRenderer::renderAppView(story, renderApp, conv.id());
    KonataRenderer::renderAppView(story, renderApp, QString());
    check(Konata::Serialization::ProjectSerializer::serialize(doc.project()) == before
              && doc.isDirty() == dirtyBefore,
          QStringLiteral("C6. Presentation never mutates data or dirty state"));

    AppDefinition brandedA = renderApp;
    brandedA.appName = QStringLiteral("Apple Messages");
    AppDefinition brandedB = renderApp;
    brandedB.appName = QStringLiteral("WhatsTrack");
    const RenderedAppView viewA = KonataRenderer::renderAppView(story, brandedA, conv.id());
    const RenderedAppView viewB = KonataRenderer::renderAppView(story, brandedB, conv.id());
    check(viewA.screen.type == viewB.screen.type
              && viewA.screen.components == viewB.screen.components
              && viewA.conversation.rows.size() == viewB.conversation.rows.size(),
          QStringLiteral("C7. Screen resolution is definition-driven, never name-branched"));

    // ---------------- Part D: preview glue ----------------
    PreviewArea preview;
    preview.setDocument(&doc);
    preview.resize(500, 700);
    preview.show();
    doc.setSelection(ProjectDocument::SelectionType::Conversation, conv.id());
    QLabel *badge = preview.findChild<QLabel *>(QStringLiteral("RenderScreenBadge"));
    QLabel *title = preview.findChild<QLabel *>(QStringLiteral("RenderConvTitle"));
    check(badge && badge->text().contains(QStringLiteral("Chat"))
              && badge->text().contains(doc.project().appDefinition.appName)
              && title && title->text() == QStringLiteral("Chat"),
          QStringLiteral("D1. Preview badge proves the Story + App pipeline live"));
    doc.clearSelection();
    // View memory (0.2C.3): clearing the selection keeps the open chat
    // instead of blanking the workspace; neutral appears only with no chat.
    check(badge && badge->text().contains(QStringLiteral("Chat")) && title
              && title->text() == QStringLiteral("Chat"),
          QStringLiteral("D2. Clearing selection preserves the open chat"));
    ProjectDocument freshDoc;
    PreviewArea freshPreview;
    freshPreview.setDocument(&freshDoc);
    QLabel *freshBadge =
        freshPreview.findChild<QLabel *>(QStringLiteral("RenderScreenBadge"));
    check(freshBadge
              && freshBadge->text().contains(QStringLiteral("Conversation list")),
          QStringLiteral("D2b. Untouched workspace reports the list screen"));

    out << (g_failures == 0 ? "APPUI-CHECK: ALL PASS\n" : "APPUI-CHECK: FAILURES\n");
    out.flush();
    return g_failures == 0 ? 0 : 1;
}
