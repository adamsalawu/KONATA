#include "ui/authoring/CharacterEditorWidget.h"

#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include "document/ProjectDocument.h"

using namespace Konata::Core;

CharacterEditorWidget::CharacterEditorWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    auto *form = new QFormLayout();
    form->setSpacing(6);

    m_displayNameEdit = new QLineEdit(this);
    m_displayNameEdit->setPlaceholderText(tr("Display name"));
    form->addRow(tr("Name:"), m_displayNameEdit);

    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText(tr("username (without @)"));
    form->addRow(tr("Username:"), m_usernameEdit);

    auto *avatarRow = new QWidget(this);
    auto *avatarLayout = new QHBoxLayout(avatarRow);
    avatarLayout->setContentsMargins(0, 0, 0, 0);
    avatarLayout->setSpacing(6);
    m_avatarEdit = new QLineEdit(avatarRow);
    m_avatarEdit->setPlaceholderText(tr("Avatar reference (path or URI)"));
    auto *browseButton = new QPushButton(tr("Browse…"), avatarRow);
    avatarLayout->addWidget(m_avatarEdit, 1);
    avatarLayout->addWidget(browseButton);
    form->addRow(tr("Avatar:"), avatarRow);

    m_statusEdit = new QLineEdit(this);
    m_statusEdit->setPlaceholderText(tr("Status message"));
    form->addRow(tr("Status:"), m_statusEdit);

    m_presenceCombo = new QComboBox(this);
    m_presenceCombo->addItem(tr("Offline"), static_cast<int>(Presence::Offline));
    m_presenceCombo->addItem(tr("Online"), static_cast<int>(Presence::Online));
    m_presenceCombo->addItem(tr("Away"), static_cast<int>(Presence::Away));
    m_presenceCombo->addItem(tr("Busy"), static_cast<int>(Presence::Busy));
    form->addRow(tr("Presence:"), m_presenceCombo);

    layout->addLayout(form);
    layout->addStretch(1);
    setEnabled(false);

    connect(m_displayNameEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (m_loading || m_character.id.isEmpty())
            return;
        m_character.displayName = text;
        apply();
    });
    connect(m_usernameEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (m_loading || m_character.id.isEmpty())
            return;
        m_character.username = text;
        apply();
    });
    connect(m_avatarEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (m_loading || m_character.id.isEmpty())
            return;
        m_character.avatarRef = text;
        apply();
    });
    connect(m_statusEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (m_loading || m_character.id.isEmpty())
            return;
        m_character.status = text;
        apply();
    });
    connect(m_presenceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index) {
                if (m_loading || m_character.id.isEmpty())
                    return;
                m_character.presence =
                    static_cast<Presence>(m_presenceCombo->itemData(index).toInt());
                apply();
            });
    connect(browseButton, &QPushButton::clicked, this, [this]() {
        if (m_character.id.isEmpty())
            return;
        const QString path = QFileDialog::getOpenFileName(
            this, tr("Choose Avatar Reference"), QString(),
            tr("Images (*.png *.jpg *.jpeg *.webp *.svg);;All files (*)"));
        if (!path.isEmpty())
            m_avatarEdit->setText(path); // textChanged applies through the document
    });
}

void CharacterEditorWidget::setDocument(ProjectDocument *document)
{
    if (m_document)
        disconnect(m_document, nullptr, this, nullptr);
    m_document = document;
    if (m_document)
        connect(m_document, &ProjectDocument::projectChanged, this,
                &CharacterEditorWidget::onProjectChanged);
    clearCharacter();
}

void CharacterEditorWidget::setCharacterId(const QString &characterId)
{
    m_character = Character();
    m_character.id = characterId;
    reload();
}

void CharacterEditorWidget::clearCharacter()
{
    m_character = Character();
    setEnabled(false);
}

void CharacterEditorWidget::onProjectChanged()
{
    if (m_applying || m_loading || !m_document || m_character.id.isEmpty())
        return;
    reload(); // external change (open/delete/elsewhere): refresh, keep no stale data
}

void CharacterEditorWidget::reload()
{
    if (!m_document || m_character.id.isEmpty()) {
        clearCharacter();
        return;
    }
    const Character *found = m_document->story().findCharacter(m_character.id);
    if (!found) {
        clearCharacter(); // deleted elsewhere
        return;
    }
    m_character = *found;

    const QSignalBlocker blockThis(this);
    m_loading = true;
    m_displayNameEdit->setText(m_character.displayName);
    m_usernameEdit->setText(m_character.username);
    m_avatarEdit->setText(m_character.avatarRef);
    m_statusEdit->setText(m_character.status);
    const int index = m_presenceCombo->findData(static_cast<int>(m_character.presence));
    m_presenceCombo->setCurrentIndex(index < 0 ? 0 : index);
    m_loading = false;
    setEnabled(true);
}

void CharacterEditorWidget::apply()
{
    if (!m_document || m_character.id.isEmpty())
        return;
    m_applying = true;
    if (!m_document->updateCharacter(m_character))
        reload(); // object vanished mid-edit: resync
    m_applying = false;
}
