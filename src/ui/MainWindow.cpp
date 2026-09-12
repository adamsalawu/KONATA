#include "ui/MainWindow.h"

#include <QApplication>
#include <QFile>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>

#include "ui/InspectorPanel.h"
#include "ui/PreviewArea.h"
#include "ui/ProjectPanel.h"
#include "ui/TimelinePanel.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("KONATA 0.1A — Master Layout"));
    setMinimumSize(1024, 640);
    setObjectName(QStringLiteral("KonataMainWindow"));

    loadStyleSheet();
    setupMenuBar();
    setupToolBar();
    setupCentralLayout();
    setupStatusBar();
}

void MainWindow::setupMenuBar()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QAction *newAction = fileMenu->addAction(tr("&New Project..."));
    newAction->setEnabled(false);
    QAction *openAction = fileMenu->addAction(tr("&Open..."));
    openAction->setEnabled(false);
    fileMenu->addSeparator();
    QAction *exitAction = fileMenu->addAction(tr("E&xit"));
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    // Placeholder toggles wired after panels exist; actions added in setupCentralLayout.
    // Keep menu present in this milestone so future view options have a home.
    viewMenu->setEnabled(true);

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    QAction *aboutAction = helpMenu->addAction(tr("&About KONATA"));
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, tr("About KONATA"),
                           tr("KONATA 0.1A — Master Layout foundation.\n"
                              "Qt 6.8.1 + CMake + MSVC 2022."));
    });
}

void MainWindow::setupToolBar()
{
    QToolBar *toolbar = addToolBar(tr("Main"));
    toolbar->setObjectName(QStringLiteral("KonataMainToolBar"));
    toolbar->setMovable(false);
    toolbar->setFloatable(false);

    QAction *newAction = toolbar->addAction(tr("New"));
    newAction->setEnabled(false);
    QAction *openAction = toolbar->addAction(tr("Open"));
    openAction->setEnabled(false);
    QAction *saveAction = toolbar->addAction(tr("Save"));
    saveAction->setEnabled(false);
}

void MainWindow::setupCentralLayout()
{
    auto *central = new QWidget(this);
    central->setObjectName(QStringLiteral("KonataCentral"));
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(6);

    m_verticalSplitter = new QSplitter(Qt::Vertical, central);
    m_verticalSplitter->setObjectName(QStringLiteral("KonataVerticalSplitter"));
    m_verticalSplitter->setChildrenCollapsible(false);

    m_horizontalSplitter = new QSplitter(Qt::Horizontal);
    m_horizontalSplitter->setObjectName(QStringLiteral("KonataHorizontalSplitter"));
    m_horizontalSplitter->setChildrenCollapsible(false);

    m_projectPanel = new ProjectPanel(m_horizontalSplitter);
    m_previewArea = new PreviewArea(m_horizontalSplitter);
    m_inspectorPanel = new InspectorPanel(m_horizontalSplitter);

    m_horizontalSplitter->addWidget(m_projectPanel);
    m_horizontalSplitter->addWidget(m_previewArea);
    m_horizontalSplitter->addWidget(m_inspectorPanel);

    // Center preview receives the majority of space.
    m_horizontalSplitter->setStretchFactor(0, 1);
    m_horizontalSplitter->setStretchFactor(1, 3);
    m_horizontalSplitter->setStretchFactor(2, 1);
    m_horizontalSplitter->setSizes({220, 720, 260});

    m_timelinePanel = new TimelinePanel(m_verticalSplitter);

    m_verticalSplitter->addWidget(m_horizontalSplitter);
    m_verticalSplitter->addWidget(m_timelinePanel);
    m_verticalSplitter->setStretchFactor(0, 4);
    m_verticalSplitter->setStretchFactor(1, 1);
    m_verticalSplitter->setSizes({580, 180});

    layout->addWidget(m_verticalSplitter);
    setCentralWidget(central);

    for (QAction *menuAction : menuBar()->actions()) {
        if (menuAction->text() == tr("&View")) {
            if (QMenu *viewMenu = menuAction->menu()) {
                QAction *projectToggle = viewMenu->addAction(tr("Project Panel"));
                projectToggle->setCheckable(true);
                projectToggle->setChecked(true);
                connect(projectToggle, &QAction::toggled, m_projectPanel, &QWidget::setVisible);

                QAction *inspectorToggle = viewMenu->addAction(tr("Inspector Panel"));
                inspectorToggle->setCheckable(true);
                inspectorToggle->setChecked(true);
                connect(inspectorToggle, &QAction::toggled, m_inspectorPanel, &QWidget::setVisible);

                QAction *timelineToggle = viewMenu->addAction(tr("Timeline Panel"));
                timelineToggle->setCheckable(true);
                timelineToggle->setChecked(true);
                connect(timelineToggle, &QAction::toggled, m_timelinePanel, &QWidget::setVisible);
            }
        }
    }
}

void MainWindow::setupStatusBar()
{
    statusBar()->setObjectName(QStringLiteral("KonataStatusBar"));
    statusBar()->showMessage(tr("Ready — KONATA 0.1A Master Layout"));
}

void MainWindow::loadStyleSheet()
{
    // Prefer embedded resource so the exe works from any working directory.
    const QStringList candidates = {
        QStringLiteral(":/styles/dark.qss"),
        QStringLiteral("resources/styles/dark.qss"),
        QStringLiteral("../resources/styles/dark.qss")
    };

    for (const QString &path : candidates) {
        QFile file(path);
        if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QString style = QString::fromUtf8(file.readAll());
            qApp->setStyleSheet(style);
            return;
        }
    }
}
