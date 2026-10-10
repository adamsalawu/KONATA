#pragma once

#include <QWidget>

#include "core/Character.h"

class QComboBox;
class QLineEdit;
class ProjectDocument;

// Edits a real Core::Character through ProjectDocument. Holds a value copy
// plus the stable ID only — never a Core pointer. Changes apply live through
// ProjectDocument::updateCharacter (which marks the project dirty).
class CharacterEditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CharacterEditorWidget(QWidget *parent = nullptr);

    void setDocument(ProjectDocument *document);
    void setCharacterId(const QString &characterId);
    void clearCharacter();

private slots:
    void onProjectChanged();

private:
    void reload();
    void apply();

    ProjectDocument *m_document = nullptr;
    Konata::Core::Character m_character;
    bool m_loading = false;
    bool m_applying = false;

    QLineEdit *m_displayNameEdit = nullptr;
    QLineEdit *m_usernameEdit = nullptr;
    QLineEdit *m_avatarEdit = nullptr;
    QLineEdit *m_statusEdit = nullptr;
    QComboBox *m_presenceCombo = nullptr;
};
