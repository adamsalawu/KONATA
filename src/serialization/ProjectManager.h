#pragma once

#include <QString>

#include "serialization/KonataProject.h"

namespace Konata {
namespace Serialization {

struct OperationResult {
    bool ok = false;
    QString error;

    static OperationResult success() { return OperationResult{true, QString()}; }
    static OperationResult failure(const QString &error)
    {
        return OperationResult{false, error};
    }
};

// High-level .konata file operations. Tracks the current project, its file
// path, and whether it has unsaved changes. Deliberately UI-free; the future
// authoring UI will drive this class.
class ProjectManager
{
public:
    ProjectManager() = default;

    OperationResult newProject(const QString &storyTitle = QStringLiteral("Untitled Story"),
                               const QString &appName = QStringLiteral("Konata Chat"));
    OperationResult open(const QString &filePath);
    OperationResult save();
    OperationResult saveAs(const QString &filePath);

    const KonataProject &project() const { return m_project; }
    void setProject(const KonataProject &project);
    // Authoring support (0.1D): mutate the owned project in place, then call
    // markDirty() so path/dirty state and (via ProjectDocument) UI stay in sync.
    KonataProject &mutableProject() { return m_project; }
    void markDirty() { m_dirty = true; }

    const QString &filePath() const { return m_filePath; }
    bool hasFile() const { return !m_filePath.isEmpty(); }
    bool isDirty() const { return m_dirty; }
    const QString &lastError() const { return m_lastError; }

private:
    KonataProject m_project;
    QString m_filePath;
    bool m_dirty = false;
    QString m_lastError;
};

} // namespace Serialization
} // namespace Konata
