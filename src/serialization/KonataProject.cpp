#include "serialization/KonataProject.h"

namespace Konata {
namespace Serialization {

KonataProject KonataProject::createNew(const QString &storyTitle, const QString &appName)
{
    KonataProject project;
    project.story = Core::Story::create(storyTitle);
    project.appDefinition = Core::AppDefinition::defaults(appName);
    return project;
}

} // namespace Serialization
} // namespace Konata
