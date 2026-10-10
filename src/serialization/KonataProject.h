#pragma once

#include <QString>

#include "core/AppDefinition.h"
#include "core/CanvasSettings.h"
#include "core/Story.h"
#include "core/UiDesign.h"

namespace Konata {
namespace Serialization {

// In-memory KONATA project: the unit saved to / loaded from a .konata file.
// Plain value type; dirty-state and file-path tracking live in ProjectManager.
struct KonataProject {
    Core::Story story;
    Core::AppDefinition appDefinition;
    Core::CanvasSettings canvas;
    Core::UiDesign uiDesign;

    bool isEmpty() const { return story.id().isEmpty(); }

    static KonataProject createNew(const QString &storyTitle = QStringLiteral("Untitled Story"),
                                   const QString &appName = QStringLiteral("Konata Chat"));
};

} // namespace Serialization
} // namespace Konata
