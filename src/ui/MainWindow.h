#pragma once

#include <QMainWindow>

class QSplitter;
class ProjectPanel;
class PreviewArea;
class InspectorPanel;
class TimelinePanel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private:
    void setupMenuBar();
    void setupToolBar();
    void setupCentralLayout();
    void setupStatusBar();
    void loadStyleSheet();

    ProjectPanel *m_projectPanel = nullptr;
    PreviewArea *m_previewArea = nullptr;
    InspectorPanel *m_inspectorPanel = nullptr;
    TimelinePanel *m_timelinePanel = nullptr;

    QSplitter *m_horizontalSplitter = nullptr;
    QSplitter *m_verticalSplitter = nullptr;
};
