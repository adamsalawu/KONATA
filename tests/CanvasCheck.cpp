// KonataCanvasCheck — canvas aspect + custom size verification (no framework).
// Covers presets, custom validation, ratio preservation, serialization,
// switching, and that canvas changes never mutate Core story data.
// Exit 0 = all pass.
#include <QByteArray>
#include <QSize>
#include <QTextStream>

#include "core/CanvasSettings.h"
#include "core/Id.h"
#include "document/ProjectDocument.h"
#include "serialization/ProjectSerializer.h"

using namespace Konata::Core;

namespace {
int g_failures = 0;
QTextStream *g_out = nullptr;
void check(bool c, const QString &l) {
    if (c) *g_out << "[PASS] " << l << "\n"; else { *g_out << "[FAIL] " << l << "\n"; ++g_failures; }
}
} // namespace

int main()
{
    QTextStream out(stdout);
    g_out = &out;
    IdGenerator::resetForTests();

    // Preset sizes
    check(CanvasSettings::presetSize(CanvasSettings::Preset::Square_1x1) == QSize(1080,1080), QStringLiteral("1:1 preset 1080x1080"));
    check(CanvasSettings::presetSize(CanvasSettings::Preset::Portrait_4x5) == QSize(1080,1350), QStringLiteral("4:5 preset 1080x1350"));
    check(CanvasSettings::presetSize(CanvasSettings::Preset::Portrait_9x16) == QSize(1080,1920), QStringLiteral("9:16 preset 1080x1920"));
    check(CanvasSettings::presetSize(CanvasSettings::Preset::Landscape_16x9) == QSize(1920,1080), QStringLiteral("16:9 preset 1920x1080"));
    // LogicalSize for presets
    CanvasSettings s;
    s.preset = CanvasSettings::Preset::Square_1x1;
    check(s.logicalSize() == QSize(1080,1080), QStringLiteral("logicalSize for 1:1"));
    s.preset = CanvasSettings::Preset::Custom;
    s.customWidth = 800; s.customHeight = 600;
    check(s.logicalSize() == QSize(800,600), QStringLiteral("custom logicalSize"));

    // Validation
    check(CanvasSettings::isValidCustom(1080,1920), QStringLiteral("valid custom 1080x1920"));
    check(!CanvasSettings::isValidCustom(0,1080), QStringLiteral("reject zero width"));
    check(!CanvasSettings::isValidCustom(1080,0), QStringLiteral("reject zero height"));
    check(!CanvasSettings::isValidCustom(-10,1080), QStringLiteral("reject negative"));
    check(!CanvasSettings::isValidCustom(10000,10000), QStringLiteral("reject absurd large"));
    check(!CanvasSettings::isValidCustom(9000,9000), QStringLiteral("reject area > maxPixels"));

    CanvasSettings invalid;
    invalid.preset = CanvasSettings::Preset::Custom;
    invalid.customWidth = 0; invalid.customHeight = 1080;
    check(!invalid.isValid(), QStringLiteral("CanvasSettings invalid when custom zero"));

    // Ratio preservation: logicalSize preserves preset ratio, not stretched
    CanvasSettings a; a.preset = CanvasSettings::Preset::Portrait_9x16;
    const QSize sz9x16 = a.logicalSize();
    check(qAbs(double(sz9x16.width())/sz9x16.height() - 9.0/16.0) < 0.01, QStringLiteral("9:16 ratio preserved"));
    CanvasSettings b; b.preset = CanvasSettings::Preset::Square_1x1;
    check(b.logicalSize().width() == b.logicalSize().height(), QStringLiteral("1:1 square"));

    // Switching presets
    ProjectDocument doc;
    doc.newProject(QStringLiteral("Canvas Test"));
    const QString convId = doc.addConversation(QStringLiteral("Chat")).id();
    const int beforeCount = doc.story().characterCount();
    CanvasSettings c1; c1.preset = CanvasSettings::Preset::Square_1x1;
    check(doc.setCanvas(c1) && doc.canvas().preset == CanvasSettings::Preset::Square_1x1, QStringLiteral("switch to 1:1"));
    CanvasSettings c2; c2.preset = CanvasSettings::Preset::Landscape_16x9;
    check(doc.setCanvas(c2) && doc.canvas().preset == CanvasSettings::Preset::Landscape_16x9, QStringLiteral("switch to 16:9"));
    CanvasSettings c3; c3.preset = CanvasSettings::Preset::Custom; c3.customWidth=800; c3.customHeight=600;
    check(doc.setCanvas(c3) && doc.canvas().logicalSize()==QSize(800,600), QStringLiteral("switch to custom 800x600"));
    // Invalid custom rejected, no mutation, no dirty change to story
    CanvasSettings bad; bad.preset = CanvasSettings::Preset::Custom; bad.customWidth=0; bad.customHeight=0;
    const auto beforeCanvas = doc.canvas();
    check(!doc.setCanvas(bad) && doc.canvas().preset==beforeCanvas.preset, QStringLiteral("reject invalid custom, no change"));
    check(doc.story().characterCount()==beforeCount, QStringLiteral("canvas change does not mutate Core"));

    // Serialization: canvas persists, backward compatible
    Konata::Serialization::KonataProject proj;
    proj.story = doc.story();
    proj.appDefinition = doc.project().appDefinition;
    proj.canvas = c3;
    const QByteArray bytes = Konata::Serialization::ProjectSerializer::serialize(proj);
    check(bytes.contains("\"canvas\"") && bytes.contains("\"customWidth\""), QStringLiteral("canvas serialized with explicit fields"));
    const auto loaded = Konata::Serialization::ProjectSerializer::deserialize(bytes);
    check(loaded.ok && loaded.project.canvas.preset==CanvasSettings::Preset::Custom && loaded.project.canvas.logicalSize()==QSize(800,600), QStringLiteral("canvas round-trips field-for-field"));

    // Old file without canvas loads with default 9:16
    const QByteArray legacy = R"({"format":"konata-project","formatVersion":1,"story":{"id":"story_1","title":"Old"},"appDefinition":{"id":"app_1","appName":"Old"}})";
    const auto legacyLoaded = Konata::Serialization::ProjectSerializer::deserialize(legacy);
    check(legacyLoaded.ok && legacyLoaded.project.canvas.preset==CanvasSettings::Preset::Portrait_9x16, QStringLiteral("old file without canvas loads default 9:16"));

    // Switching canvas does not affect story/appDefinition
    const QString storyIdBefore = doc.story().id();
    CanvasSettings c4; c4.preset = CanvasSettings::Preset::Portrait_4x5;
    doc.setCanvas(c4);
    check(doc.story().id()==storyIdBefore, QStringLiteral("canvas switch leaves Story intact"));

    out << (g_failures==0 ? "CANVAS-CHECK: ALL PASS\n" : "CANVAS-CHECK: FAILURES\n");
    out.flush();
    return g_failures==0 ? 0 : 1;
}
