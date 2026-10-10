#pragma once

#include <QSize>
#include <QString>

namespace Konata {
namespace Core {

// Output canvas definition (presentation, not Core story data).
// Controls the logical export/preview size. Preset sizes are 1080-based
// 1080 family; Custom allows explicit dimensions. All values are logical
// pixels; preview scales uniformly, export renders at exact size.
struct CanvasSettings {
    enum class Preset {
        Square_1x1,
        Portrait_4x5,
        Portrait_9x16,
        Landscape_16x9,
        Custom
    };

    Preset preset = Preset::Portrait_9x16;
    int customWidth = 1080;
    int customHeight = 1920;

    // Logical output size for current preset (or custom dimensions).
    QSize logicalSize() const;
    static QSize presetSize(Preset preset);
    static QString presetLabel(Preset preset);
    static Preset presetFromLabel(const QString &label, bool *ok = nullptr);

    // Validation: Custom 1..8192, presets are always valid.
    bool isValid() const;
    static bool isValidCustom(int w, int h);
    static constexpr int kMinDimension = 1;
    static constexpr int kMaxDimension = 8192;
    static constexpr int kMaxPixels = 8192 * 8192; // guard absurd area
};

} // namespace Core
} // namespace Konata
