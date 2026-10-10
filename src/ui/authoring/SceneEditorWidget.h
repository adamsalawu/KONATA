#pragma once

#include <QWidget>

#include "core/Scene.h"

class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class ProjectDocument;

// Edits a real Core::Scene's basic info through ProjectDocument and hosts the
// structured event list. Events are selectable authoring rows (not a visual
// timeline); the selected event opens in the SceneEvent editor.
class SceneEditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SceneEditorWidget(QWidget *parent = nullptr);

    void setDocument(ProjectDocument *document);
    void setSceneId(const QString &sceneId);
    void clearScene();

private slots:
    void onProjectChanged();
    void onSelectionChanged();
    void onEventSelection();

private:
    void reload();
    void reloadEventList();
    void syncEventSelection();
    void addEventOfType(Konata::Core::SceneEventType type);
    void deleteSelectedEvent();
    void apply();

    ProjectDocument *m_document = nullptr;
    Konata::Core::Scene m_scene;
    bool m_loading = false;
    bool m_applying = false;

    QLineEdit *m_nameEdit = nullptr;
    QPlainTextEdit *m_descriptionEdit = nullptr;
    QLabel *m_eventCountLabel = nullptr;
    QListWidget *m_eventList = nullptr;
    QPushButton *m_addEventButton = nullptr;
    QPushButton *m_deleteEventButton = nullptr;
};
