#pragma once

#include <QByteArray>
#include <QString>

#include "serialization/KonataProject.h"

namespace Konata {
namespace Serialization {

struct SaveResult {
    bool ok = false;
    QString error;
};

struct LoadResult {
    bool ok = false;
    QString error;
    KonataProject project;
};

// Converts KonataProject <-> .konata JSON files.
// All functions are thread-compatible; none touch the UI.
class ProjectSerializer
{
public:
    static constexpr int kFormatVersion = 1;
    static constexpr const char *kFormatName = "konata-project";
    static constexpr const char *kFileExtension = ".konata";

    static SaveResult saveToFile(const QString &filePath, const KonataProject &project);
    static LoadResult loadFromFile(const QString &filePath);

    // In-memory form (UTF-8 JSON). Useful for tests and future features.
    static QByteArray serialize(const KonataProject &project);
    static LoadResult deserialize(const QByteArray &data);

private:
    ProjectSerializer() = delete;
};

} // namespace Serialization
} // namespace Konata
