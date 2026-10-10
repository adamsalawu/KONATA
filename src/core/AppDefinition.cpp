#include "core/AppDefinition.h"

#include "core/Id.h"

namespace Konata {
namespace Core {

AppDefinition AppDefinition::defaults(const QString &appName)
{
    AppDefinition app;
    app.id = IdGenerator::create(QStringLiteral("app"));
    app.appName = appName;
    app.packageId = QStringLiteral("tools.konata.chat");
    // Appearance structs keep their in-header defaults.
    return app;
}

} // namespace Core
} // namespace Konata
