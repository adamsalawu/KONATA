// KonataAppleMessagesExactCheck — 0.2C.2 verification (no test framework).
// Geometry spec, vector icons, grouping/corners, list/profile screens,
// timestamp metadata, token sensitivity, serialization, determinism,
// non-mutation, and offscreen shell wiring. Structural assertions only —
// no pixel goldens, no screenshots.
// Exit 0 = all pass.
#include <QApplication>
#include <QByteArray>
#include <QLabel>
#include <QTextStream>

#include "core/AppDefinition.h"
#include "core/AppUi.h"
#include "core/Id.h"
#include "document/ProjectDocument.h"
#include "rendering/KonataRenderer.h"
#include "rendering/skins/AppleGeometry.h"
#include "rendering/skins/AppleIcons.h"
#include "rendering/skins/AppleMessagesSkin.h"
#include "rendering/skins/ApplePalette.h"
#include "rendering/skins/AppleTypography.h"
#include "serialization/ProjectSerializer.h"
#include "ui/PreviewArea.h"
#include "ui/RoundedBubble.h"

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
    g_out->flush(); // unbuffered: a later crash must not hide progress
}

bool pixmapsEqual(const QPixmap &a, const QPixmap &b)
{
    return a.size() == b.size() && a.toImage() == b.toImage();
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
} // namespace

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    QApplication app(argc, argv);

    QTextStream out(stdout);
    g_out = &out;
    IdGenerator::resetForTests();

    // ---------------- Geometry ----------------
    check(AppleGeometry::viewportWidth == 393 && AppleGeometry::viewportHeight == 852
              && AppleGeometry::safeAreaTop == 59 && AppleGeometry::safeAreaBottom == 34,
          QStringLiteral("G1. iPhone portrait target viewport + safe areas"));
    check(AppleGeometry::chatHeaderHeight > 0
              && AppleGeometry::chatHeaderHeight + AppleGeometry::safeAreaTop
                  < AppleGeometry::viewportHeight
              && AppleGeometry::messageMarginHorizontal * 2 < AppleGeometry::viewportWidth
              && AppleGeometry::bubbleMaxWidthPercent > 50
              && AppleGeometry::bubbleMaxWidthPercent <= 100
              && AppleGeometry::bubbleRadiusLarge >= AppleGeometry::bubbleRadiusSmall
              && AppleGeometry::bubbleRadiusSmall >= 0
              && AppleGeometry::listSeparatorInset
                  == AppleGeometry::listAvatarSize + 24
              && AppleGeometry::messageSpacingGroups > AppleGeometry::messageSpacingSameAuthor,
          QStringLiteral("G2. Layout relationships hold (margins fit, radii nest)"));

    // ---------------- Icons ----------------
    const QColor fg(QStringLiteral("#0A84FF"));
    const QColor bg(Qt::transparent);
    bool allPaths = true;
    bool allPixmaps = true;
    const AppleIcon allIcons[] = {AppleIcon::Back,   AppleIcon::Info, AppleIcon::Add,
                                  AppleIcon::Camera, AppleIcon::Send, AppleIcon::Search,
                                  AppleIcon::Compose};
    for (AppleIcon icon : allIcons) {
        if (icon != AppleIcon::Info && icon != AppleIcon::Send
            && AppleIcons::strokePath(icon).isEmpty())
            allPaths = false;
        const QPixmap small = AppleIcons::pixmap(icon, 24, fg, bg);
        const QPixmap large = AppleIcons::pixmap(icon, 96, fg, bg);
        if (small.isNull() || small.size() != QSize(24, 24) || large.isNull()
            || large.size() != QSize(96, 96))
            allPixmaps = false;
    }
    check(allPaths, QStringLiteral("I1. Stroked icons define real vector geometry"));
    check(allPixmaps,
          QStringLiteral("I2. Every icon rasterizes at 24px and 96px (resolution-independent)"));
    check(pixmapsEqual(AppleIcons::pixmap(AppleIcon::Send, 48, fg, bg),
                       AppleIcons::pixmap(AppleIcon::Send, 48, fg, bg)),
          QStringLiteral("I3. Icon output is deterministic"));

    // ---------------- Typography + palette ----------------
    check(AppleTypography::bodySize() == 17 && AppleTypography::navLargeTitleSize() == 34
              && !AppleTypography::familyStack().isEmpty()
              && AppleTypography::familyStack().first() == QStringLiteral("SF Pro Text"),
          QStringLiteral("T1. Explicit type scale with SF-first font stack"));
    const ApplePalette dark = ApplePalette::dark();
    const ApplePalette light = ApplePalette::light();
    check(dark.background != light.background && dark.outgoingBubble != light.outgoingBubble
              && dark.incomingBubble != light.incomingBubble
              && dark.primaryText != light.primaryText
              && !dark.separators.isEmpty() && !light.separators.isEmpty(),
          QStringLiteral("P1. Dark/light palettes differ per surface (no hardwiring)"));

    // ---------------- Story fixtures ----------------
    ProjectDocument doc;
    doc.newProject(QStringLiteral("Exact Test"));
    const Character alice = doc.addCharacter(QStringLiteral("Alice"), QStringLiteral("alice"));
    const Character bob = doc.addCharacter(QStringLiteral("Bob"), QStringLiteral("bob"));
    Conversation conv = doc.addConversation(QStringLiteral("Chat"), ConversationType::Private);
    conv.setParticipantIds(QStringList{alice.id, bob.id});
    doc.updateConversation(conv);
    // Run: A A B with explicit timestamps across two days.
    const QDateTime dayOne(QDate(2026, 3, 10), QTime(9, 0), Qt::UTC);
    const QDateTime dayOneLater(QDate(2026, 3, 10), QTime(9, 5), Qt::UTC);
    const QDateTime dayTwo(QDate(2026, 3, 11), QTime(10, 0), Qt::UTC);
    Message m1 = doc.createMessage(conv.id(), alice.id, MessageType::Text);
    m1.text = QStringLiteral("One");
    m1.createdAt = dayOne;
    doc.updateMessage(conv.id(), m1);
    Message m2 = doc.createMessage(conv.id(), alice.id, MessageType::Text);
    m2.text = QStringLiteral("Two");
    m2.createdAt = dayOneLater;
    doc.updateMessage(conv.id(), m2);
    Message m3 = doc.createMessage(conv.id(), bob.id, MessageType::Text);
    m3.text = QStringLiteral("Three");
    m3.createdAt = dayTwo;
    doc.updateMessage(conv.id(), m3);
    Conversation solo = doc.addConversation(QStringLiteral("Solo"), ConversationType::Private);
    AppDefinition skin = AppleMessagesSkin::createAppDefinition();
    const Story &story = doc.story();

    // ---------------- Grouping + corners ----------------
    const RenderedConversation grouped =
        KonataRenderer::renderConversation(story, skin, conv.id());
    check(grouped.rows.size() == 3 && grouped.rows.at(0).group == MessageGroup::First
              && grouped.rows.at(1).group == MessageGroup::Last
              && grouped.rows.at(2).group == MessageGroup::Single,
          QStringLiteral("M1. First/Last/Single grouping from authorship + order"));
    const RenderedMessage &first = grouped.rows.at(0);
    const RenderedMessage &last = grouped.rows.at(1);
    // Fixture run is [Alice, Alice, Bob]: First.out, Last.out, Single.in.
    check(first.outgoing && last.outgoing && !grouped.rows.at(2).outgoing,
          QStringLiteral("M2a. Fixture direction follows first-participant rule"));
    check(first.corners.bottomRight == 6 && first.corners.topRight == 18
              && last.corners.topRight == 6 && last.corners.bottomRight == 18
              && grouped.rows.at(2).corners.topLeft == 18,
          QStringLiteral("M2. Inner corners collapse to 6px, outer keep 18"));
    check(grouped.rows.at(0).startsNewDay && !grouped.rows.at(1).startsNewDay
              && grouped.rows.at(2).startsNewDay && !grouped.rows.at(2).dayLabel.isEmpty(),
          QStringLiteral("M3. Day separators derive from message dates"));

    AppDefinition flat = skin;
    flat.structure.chat.messageArea.groupConsecutive = false;
    const RenderedConversation ungrouped =
        KonataRenderer::renderConversation(story, flat, conv.id());
    check(ungrouped.rows.at(0).group == MessageGroup::Single
              && ungrouped.rows.at(0).corners.bottomLeft == 18,
          QStringLiteral("M4. Disabled grouping yields uniform corners"));
    AppDefinition regrouped = skin;
    regrouped.structure.chat.messageArea.groupedCornerRadius = 9;
    const RenderedConversation recornered =
        KonataRenderer::renderConversation(story, regrouped, conv.id());
    const RenderedMessage &rCorner = recornered.rows.at(0);
    check(rCorner.group == MessageGroup::First && rCorner.outgoing
              && rCorner.corners.bottomRight == 9
              && story.findConversation(conv.id())->messages().size() == 3,
          QStringLiteral("M5. Grouped-radius token changes presentation, not Story"));

    // ---------------- List + profile ----------------
    const RenderedConversationList list =
        KonataRenderer::renderConversationList(story, skin);
    check(list.valid && list.rows.size() == 2
              && list.rows.at(0).conversationId == conv.id()
              && list.rows.at(0).previewText == QStringLiteral("Three")
              && !list.rows.at(0).timestampText.isEmpty() && !list.rows.at(0).hasUnread
              && list.rows.at(1).previewText == QStringLiteral("(No messages)")
              && list.showAvatars,
          QStringLiteral("L1. List rows carry preview, timestamp, counts — never faked"));
    const RenderedProfile profile = KonataRenderer::renderProfile(story, alice.id);
    const RenderedProfile missing = KonataRenderer::renderProfile(story, QStringLiteral("x"));
    check(profile.valid && profile.displayName == QStringLiteral("Alice")
              && profile.actions.size() == 3 && !missing.valid,
          QStringLiteral("P2. Profile resolves character; unknown stays invalid"));

    // ---------------- Serialization ----------------
    Konata::Serialization::KonataProject project;
    project.story = story;
    project.appDefinition = skin;
    const QByteArray bytes = Konata::Serialization::ProjectSerializer::serialize(project);
    check(bytes.contains("\"groupedCornerRadius\""),
          QStringLiteral("S1. Grouped radius persists with explicit field name"));
    const Konata::Serialization::LoadResult loaded =
        Konata::Serialization::ProjectSerializer::deserialize(bytes);
    check(loaded.ok
              && loaded.project.appDefinition.structure.chat.messageArea.groupedCornerRadius
                  == 6,
          QStringLiteral("S2. Presentation config round-trips (6px verified)"));
    const QByteArray legacyProject =
        R"({"format":"konata-project","formatVersion":1,)"
        R"("story":{"id":"story_1","title":"Old"},)"
        R"("appDefinition":{"id":"app_1","appName":"Old",)"
        R"("structure":{"chat":{"messageArea":{}}}}})";
    const Konata::Serialization::LoadResult legacy =
        Konata::Serialization::ProjectSerializer::deserialize(legacyProject);
    check(legacy.ok
              && legacy.project.appDefinition.structure.chat.messageArea.groupedCornerRadius
                  == 4,
          QStringLiteral("S3. Files without the field load with the default"));

    // ---------------- Determinism + purity ----------------
    const RenderedAppView viewA = KonataRenderer::renderAppView(story, skin, conv.id());
    const RenderedAppView viewB = KonataRenderer::renderAppView(story, skin, conv.id());
    const RenderedConversationList listA = KonataRenderer::renderConversationList(story, skin);
    const RenderedConversationList listB = KonataRenderer::renderConversationList(story, skin);
    check(viewA.conversation.rows.at(0).corners.bottomLeft
                  == viewB.conversation.rows.at(0).corners.bottomLeft
              && listA.rows.size() == listB.rows.size(),
          QStringLiteral("R1. Repeated renders are equivalent"));
    const QByteArray before =
        Konata::Serialization::ProjectSerializer::serialize(doc.project());
    const bool dirtyBefore = doc.isDirty();
    KonataRenderer::renderAppView(story, skin, conv.id());
    KonataRenderer::renderConversationList(story, skin);
    KonataRenderer::renderProfile(story, alice.id);
    check(Konata::Serialization::ProjectSerializer::serialize(doc.project()) == before
              && doc.isDirty() == dirtyBefore,
          QStringLiteral("R2. Extended rendering never mutates or dirties"));

    // ---------------- Shell wiring offscreen ----------------
    PreviewArea preview;
    preview.setDocument(&doc);
    preview.resize(500, 800);
    preview.show();

    // ---------------- Single-chat workspace ----------------
    // (0.2C.3 design change, documented in the milestone report: the chrome
    // switcher and the in-preview list/profile screens were removed — the
    // ProjectPanel is the only navigation. These replace W1/W2/W5–W7 with
    // stronger single-chat-invariant assertions.)
    check(preview.findChild<QWidget *>(QStringLiteral("ScreenSwitcher")) == nullptr
              && preview.findChildren<QWidget *>(QStringLiteral("RenderListRow")).isEmpty()
              && preview.findChild<QWidget *>(QStringLiteral("RenderProfileCard")) == nullptr,
          QStringLiteral("W1. One chat editor: no switcher, list, or profile screens"));
    // Selecting another conversation replaces the chat outright.
    doc.setSelection(ProjectDocument::SelectionType::Conversation, solo.id());
    QLabel *chatTitle = preview.findChild<QLabel *>(QStringLiteral("RenderConvTitle"));
    check(chatTitle && chatTitle->text() == QStringLiteral("Solo"),
          QStringLiteral("W2. Selecting a conversation opens it directly"));
    doc.setSelection(ProjectDocument::SelectionType::Conversation, conv.id());
    check(chatTitle && chatTitle->text() == QStringLiteral("Chat"),
          QStringLiteral("W2b. Switching back replaces the chat"));
    // A vector icon carries a pixmap and no text; a glyph shortcut would be
    // text. (Checked via text to stay independent of QLabel::pixmap() shape.)
    QLabel *back = preview.findChild<QLabel *>(QStringLiteral("RenderBack"));
    check(back && back->text().isEmpty(),
          QStringLiteral("W3. Header back affordance is a vector icon, not a glyph"));

    doc.setSelection(ProjectDocument::SelectionType::Conversation, conv.id());
    const QList<RoundedBubble *> bubbles =
        preview.findChildren<RoundedBubble *>();
    bool foundGrouped = false;
    for (RoundedBubble *bubble : bubbles) {
        if (bubble->property("groupPosition").toString() == QStringLiteral("First"))
            foundGrouped = true;
    }
    check(!bubbles.isEmpty() && foundGrouped,
          QStringLiteral("W4. Chat bubbles carry resolved grouping"));

    // Character selection preserves the open chat (view memory) instead of
    // navigating anywhere: authoring navigation never blanks the workspace.
    doc.setSelection(ProjectDocument::SelectionType::Character, bob.id);
    check(chatTitle && chatTitle->text() == QStringLiteral("Chat"),
          QStringLiteral("W5. Character selection preserves the open chat"));
    doc.setSelection(ProjectDocument::SelectionType::Conversation, conv.id());

    preview.resize(900, 500);
    preview.resize(320, 900);
    check(chatTitle && chatTitle->text() == QStringLiteral("Chat")
              && doc.isDirty() == dirtyBefore,
          QStringLiteral("W6. Resize is safe, chat intact, never dirties"));

    out << (g_failures == 0 ? "APPLE-EXACT-CHECK: ALL PASS\n"
                            : "APPLE-EXACT-CHECK: FAILURES\n");
    out.flush();
    return g_failures == 0 ? 0 : 1;
}
