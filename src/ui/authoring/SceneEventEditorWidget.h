#pragma once

#include <QWidget>

#include "core/SceneEvent.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class ProjectDocument;

// Edits a real Core::SceneEvent (nested in a Scene) through ProjectDocument.
// Per-type rows expose the existing Core references: conversation/message
// pickers (SendMessage), validated duration (Wait), character picker
// (Start/StopTyping), text (Notification). Value copy + stable IDs only.
class SceneEventEditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SceneEventEditorWidget(QWidget *parent = nullptr);

    void setDocument(ProjectDocument *document);
    void setSceneEvent(const QString &sceneId, const QString &eventId);
    void clearEvent();

    static QString typeLabel(Konata::Core::SceneEventType type);

private slots:
    void onProjectChanged();

private:
    void reload();
    void reloadConversationCombos();
    void reloadMessageCombo();
    void reloadCharacterCombo();
    void apply();

    ProjectDocument *m_document = nullptr;
    QString m_sceneId;
    Konata::Core::SceneEvent m_event;
    bool m_loading = false;
    bool m_applying = false;

    QComboBox *m_typeCombo = nullptr;
    QStackedWidget *m_payloadStack = nullptr;
    // SendMessage page
    QComboBox *m_sendConversationCombo = nullptr;
    QComboBox *m_sendMessageCombo = nullptr;
    // Wait page
    QSpinBox *m_waitSpin = nullptr;
    // Typing page (shared by Start/Stop)
    QComboBox *m_typingCharacterCombo = nullptr;
    QComboBox *m_typingConversationCombo = nullptr;
    // Notification page
    QLineEdit *m_notificationEdit = nullptr;
};
