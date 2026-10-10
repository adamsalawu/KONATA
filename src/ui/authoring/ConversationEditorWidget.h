#pragma once

#include <QWidget>

#include "core/Conversation.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class ProjectDocument;

// Edits a real Core::Conversation through ProjectDocument. Participants are
// Character ID references picked from the Story's actual characters —
// Character objects are never duplicated. Value copy + stable ID only.
class ConversationEditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ConversationEditorWidget(QWidget *parent = nullptr);

    void setDocument(ProjectDocument *document);
    void setConversationId(const QString &conversationId);
    void clearConversation();

private slots:
    void onProjectChanged();
    void onSelectionChanged();
    void onMessageSelection();

private:
    void reload();
    void reloadParticipantList();
    void reloadMessageList();
    void syncMessageSelection();
    void addMessage();
    void deleteSelectedMessage();
    void apply();
    QString messageRowLabel(const Konata::Core::Message &message) const;

    ProjectDocument *m_document = nullptr;
    Konata::Core::Conversation m_conversation;
    bool m_loading = false;
    bool m_applying = false;

    QLineEdit *m_titleEdit = nullptr;
    QComboBox *m_typeCombo = nullptr;
    QLabel *m_skinLabel = nullptr;
    QListWidget *m_participantList = nullptr;
    QListWidget *m_messageList = nullptr;
    QPushButton *m_addMessageButton = nullptr;
    QPushButton *m_deleteMessageButton = nullptr;
};
