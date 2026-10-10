#pragma once

#include <cmath>

#include <QString>
#include <QtGlobal>

namespace Konata {
namespace Core {

enum class AttachmentType {
    Image,
    File,
    Audio,
    Video,
    Other
};

// Data-only attachment reference. No uploading or rendering in Core.
struct Attachment {
    QString id;
    AttachmentType type = AttachmentType::File;
    QString fileName;   // e.g. "photo.png"
    QString fileRef;    // path, URI, or content key (resolved later)
    QString mimeType;   // e.g. "image/png"
    qint64 sizeBytes = -1; // -1 = unknown
    // Normalized image anchor (0.0 = left/top, 1.0 = right/bottom).
    // Resolution-independent; the renderer converts it to display
    // coordinates. Defaults to center, matching pre-0.2A.1 rendering.
    double anchorX = 0.5;
    double anchorY = 0.5;

    // Single validation point: non-finite values fall back to center,
    // everything else is clamped to [0, 1].
    static double clampAnchor(double value);
};

inline double Attachment::clampAnchor(double value)
{
    if (!std::isfinite(value))
        return 0.5;
    return qBound(0.0, value, 1.0);
}

} // namespace Core
} // namespace Konata
