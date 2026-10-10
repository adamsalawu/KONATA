#pragma once

#include <QWidget>

#include "core/Message.h"

class QCheckBox;
class QComboBox;
class QDateTimeEdit;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class ProjectDocument;

// Edits a real Core::Message (nested in a Conversation) through
// ProjectDocument. Author is a Character ID reference; the text field serves
// as body (Text/System) or caption (Image/File). For Image/File the file
// reference maps to the first attachment entry (reference only — no asset
// processing). Value copy + stable IDs only, never Core pointers.
class MessageEditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MessageEditorWidget(QWidget *parent = nullptr);

    void setDocument(ProjectDocument *document);
    void setMessage(const QString &conversationId, const QString &messageId);
    void clearMessage();

private slots:
    void onProjectChanged();

private:
    void reload();
    void reloadAuthorCombo();
    void reloadReactions();
    void reloadReactionAuthors();
    void apply();
    void applyFileRef(const QString &fileRef);
    void applyAnchor();
    void applyTimestamp();
    void addReaction();
    void removeSelectedReaction();

    ProjectDocument *m_document = nullptr;
    QString m_conversationId;
    Konata::Core::Message m_message;
    bool m_loading = false;
    bool m_applying = false;

    QComboBox *m_authorCombo = nullptr;
    QComboBox *m_typeCombo = nullptr;
    QPlainTextEdit *m_textEdit = nullptr;
    QLineEdit *m_fileRefEdit = nullptr;
    QWidget *m_positionRow = nullptr;
    QDoubleSpinBox *m_anchorXSpin = nullptr;
    QDoubleSpinBox *m_anchorYSpin = nullptr;
    QLabel *m_orderLabel = nullptr;
    QCheckBox *m_timestampCheck = nullptr;
    QDateTimeEdit *m_timestampEdit = nullptr;
    QListWidget *m_reactionList = nullptr;
    QLineEdit *m_reactionEmojiEdit = nullptr;
    QComboBox *m_reactionAuthorCombo = nullptr;
};
