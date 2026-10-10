#pragma once

#include <QMainWindow>

#include "document/ProjectDocument.h"

class QSplitter;
class QStackedWidget;
class ProjectPanel;
class PreviewArea;
class InspectorPanel;
class TimelinePanel;
class DeveloperMode;

// Application shell. Owns the ProjectDocument (ProjectManager -> project ->
// Core data) and connects it to the panels; owns no Core data itself.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onNewProject();
    void onOpenProject();
    void onSaveProject();
    void onSaveProjectAs();
    void onExportPng();
    void onToggleDeveloperMode();
    void updateChrome();

private:
    void setupMenuBar();
    void setupToolBar();
    void setupCentralLayout();
    void setupStatusBar();
    void loadStyleSheet();
    void wireDocument();
    // True when it is safe to discard the current project (clean, saved,
    // or user chose Discard). False when the operation must be cancelled.
    bool maybeDiscard();

    ProjectDocument m_document;
    ProjectPanel *m_projectPanel = nullptr;
    PreviewArea *m_previewArea = nullptr;
    InspectorPanel *m_inspectorPanel = nullptr;
    TimelinePanel *m_timelinePanel = nullptr;
    DeveloperMode *m_developerMode = nullptr;
    QStackedWidget *m_centerStack = nullptr;
    bool m_inDeveloperMode = false;

    QSplitter *m_horizontalSplitter = nullptr;
    QSplitter *m_verticalSplitter = nullptr;
};
