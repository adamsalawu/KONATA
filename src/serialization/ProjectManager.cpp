#include "serialization/ProjectManager.h"

#include "serialization/ProjectSerializer.h"

namespace Konata {
namespace Serialization {

OperationResult ProjectManager::newProject(const QString &storyTitle, const QString &appName)
{
    m_project = KonataProject::createNew(storyTitle, appName);
    m_filePath.clear();
    m_dirty = false;
    m_lastError.clear();
    return OperationResult::success();
}

OperationResult ProjectManager::open(const QString &filePath)
{
    const LoadResult loaded = ProjectSerializer::loadFromFile(filePath);
    if (!loaded.ok) {
        m_lastError = loaded.error;
        return OperationResult::failure(loaded.error);
    }
    m_project = loaded.project;
    m_filePath = filePath;
    m_dirty = false;
    m_lastError.clear();
    return OperationResult::success();
}

OperationResult ProjectManager::save()
{
    if (m_filePath.isEmpty()) {
        m_lastError = QStringLiteral("No file path: use Save As first.");
        return OperationResult::failure(m_lastError);
    }
    const SaveResult saved = ProjectSerializer::saveToFile(m_filePath, m_project);
    if (!saved.ok) {
        m_lastError = saved.error;
        return OperationResult::failure(saved.error);
    }
    m_dirty = false;
    m_lastError.clear();
    return OperationResult::success();
}

OperationResult ProjectManager::saveAs(const QString &filePath)
{
    if (filePath.isEmpty()) {
        m_lastError = QStringLiteral("No file path given.");
        return OperationResult::failure(m_lastError);
    }
    const SaveResult saved = ProjectSerializer::saveToFile(filePath, m_project);
    if (!saved.ok) {
        m_lastError = saved.error;
        return OperationResult::failure(saved.error);
    }
    m_filePath = filePath;
    m_dirty = false;
    m_lastError.clear();
    return OperationResult::success();
}

void ProjectManager::setProject(const KonataProject &project)
{
    m_project = project;
    m_dirty = true;
}

} // namespace Serialization
} // namespace Konata
