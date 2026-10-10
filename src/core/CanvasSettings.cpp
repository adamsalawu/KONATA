#include "core/CanvasSettings.h"

namespace Konata {
namespace Core {

QSize CanvasSettings::logicalSize() const
{
    if (preset == Preset::Custom)
        return QSize(customWidth, customHeight);
    return presetSize(preset);
}

QSize CanvasSettings::presetSize(Preset preset)
{
    switch (preset) {
    case Preset::Square_1x1:
        return QSize(1080, 1080);
    case Preset::Portrait_4x5:
        return QSize(1080, 1350);
    case Preset::Portrait_9x16:
        return QSize(1080, 1920);
    case Preset::Landscape_16x9:
        return QSize(1920, 1080);
    case Preset::Custom:
        return QSize(1080, 1920);
    }
    return QSize(1080, 1920);
}

QString CanvasSettings::presetLabel(Preset preset)
{
    switch (preset) {
    case Preset::Square_1x1:
        return QStringLiteral("1:1");
    case Preset::Portrait_4x5:
        return QStringLiteral("4:5");
    case Preset::Portrait_9x16:
        return QStringLiteral("9:16");
    case Preset::Landscape_16x9:
        return QStringLiteral("16:9");
    case Preset::Custom:
        return QStringLiteral("Custom");
    }
    return QStringLiteral("9:16");
}

CanvasSettings::Preset CanvasSettings::presetFromLabel(const QString &label, bool *ok)
{
    if (ok) *ok = true;
    if (label == QStringLiteral("1:1")) return Preset::Square_1x1;
    if (label == QStringLiteral("4:5")) return Preset::Portrait_4x5;
    if (label == QStringLiteral("9:16")) return Preset::Portrait_9x16;
    if (label == QStringLiteral("16:9")) return Preset::Landscape_16x9;
    if (label == QStringLiteral("Custom")) return Preset::Custom;
    if (ok) *ok = false;
    return Preset::Portrait_9x16;
}

bool CanvasSettings::isValid() const
{
    if (preset != Preset::Custom)
        return true;
    return isValidCustom(customWidth, customHeight);
}

bool CanvasSettings::isValidCustom(int w, int h)
{
    if (w < kMinDimension || h < kMinDimension) return false;
    if (w > kMaxDimension || h > kMaxDimension) return false;
    const qint64 area = static_cast<qint64>(w) * h;
    if (area > kMaxPixels) return false;
    return true;
}

} // namespace Core
} // namespace Konata
