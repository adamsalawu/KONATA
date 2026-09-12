#include <QApplication>

#include "ui/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("KONATA"));
    QApplication::setApplicationVersion(QStringLiteral("0.1A"));
    QApplication::setOrganizationName(QStringLiteral("RisingStar"));

    MainWindow window;
    window.resize(1280, 800);
    window.show();

    return app.exec();
}
