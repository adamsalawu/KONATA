#pragma once

#include <QWidget>

class QStackedWidget;
class ProjectDocument;
class CharacterEditorWidget;
class ConversationEditorWidget;
class MessageEditorWidget;
class SceneEditorWidget;
class SceneEventEditorWidget;

// Selection-aware editor host. Switches between the empty state and the
// Character/Conversation/Message/Scene/SceneEvent editors based on
// ProjectDocument selection. Holds no Core data itself.
class InspectorPanel : public QWidget
{
    Q_OBJECT

public:
    explicit InspectorPanel(QWidget *parent = nullptr);

    void setDocument(ProjectDocument *document);
    // Puts keyboard focus into the current editor's primary editable field
    // (used by chat double-click and the Edit context action). No-op when the
    // current page has no editable field.
    void focusEditableField();

private slots:
    void showEditorForSelection();

private:
    ProjectDocument *m_document = nullptr;
    QStackedWidget *m_stack = nullptr;
    QWidget *m_emptyPage = nullptr;
    CharacterEditorWidget *m_characterEditor = nullptr;
    ConversationEditorWidget *m_conversationEditor = nullptr;
    MessageEditorWidget *m_messageEditor = nullptr;
    SceneEditorWidget *m_sceneEditor = nullptr;
    SceneEventEditorWidget *m_eventEditor = nullptr;
};
