#include "ui/authoring/MessageEditorWidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include "core/Id.h"
#include "document/ProjectDocument.h"

using namespace Konata::Core;

namespace {
QString authorLabel(const Character &c)
{
    if (c.displayName.isEmpty())
        return MessageEditorWidget::tr("(unnamed character)");
    return QStringLiteral("%1 (@%2)").arg(c.displayName, c.username);
}
} // namespace

MessageEditorWidget::MessageEditorWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    auto *form = new QFormLayout();
    form->setSpacing(6);

    m_authorCombo = new QComboBox(this);
    form->addRow(tr("Author:"), m_authorCombo);

    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem(tr("Text"), static_cast<int>(MessageType::Text));
    m_typeCombo->addItem(tr("Image"), static_cast<int>(MessageType::Image));
    m_typeCombo->addItem(tr("File"), static_cast<int>(MessageType::File));
    m_typeCombo->addItem(tr("System"), static_cast<int>(MessageType::System));
    form->addRow(tr("Type:"), m_typeCombo);

    layout->addLayout(form);

    auto *textLabel = new QLabel(tr("Text / caption:"), this);
    layout->addWidget(textLabel);
    m_textEdit = new QPlainTextEdit(this);
    m_textEdit->setPlaceholderText(tr("Message text"));
    m_textEdit->setMaximumBlockCount(200);
    m_textEdit->setFixedHeight(72);
    layout->addWidget(m_textEdit);

    auto *fileLabel = new QLabel(tr("File reference (image/file messages):"), this);
    layout->addWidget(fileLabel);
    auto *fileRow = new QWidget(this);
    auto *fileLayout = new QHBoxLayout(fileRow);
    fileLayout->setContentsMargins(0, 0, 0, 0);
    fileLayout->setSpacing(6);
    m_fileRefEdit = new QLineEdit(fileRow);
    m_fileRefEdit->setPlaceholderText(tr("File path or URI reference"));
    auto *browseButton = new QPushButton(tr("Browse…"), fileRow);
    fileLayout->addWidget(m_fileRefEdit, 1);
    fileLayout->addWidget(browseButton);
    layout->addWidget(fileRow);

    // Normalized image anchor (Image messages only). Exact numeric entry;
    // no drag interaction in this milestone.
    m_positionRow = new QWidget(this);
    auto *positionLayout = new QHBoxLayout(m_positionRow);
    positionLayout->setContentsMargins(0, 0, 0, 0);
    positionLayout->setSpacing(6);
    auto *xLabel = new QLabel(tr("Position X:"), m_positionRow);
    m_anchorXSpin = new QDoubleSpinBox(m_positionRow);
    auto *yLabel = new QLabel(tr("Y:"), m_positionRow);
    m_anchorYSpin = new QDoubleSpinBox(m_positionRow);
    for (QDoubleSpinBox *spin : {m_anchorXSpin, m_anchorYSpin}) {
        spin->setRange(0.0, 1.0);
        spin->setDecimals(2);
        spin->setSingleStep(0.05);
        spin->setValue(0.5);
    }
    positionLayout->addWidget(xLabel);
    positionLayout->addWidget(m_anchorXSpin, 1);
    positionLayout->addWidget(yLabel);
    positionLayout->addWidget(m_anchorYSpin, 1);
    layout->addWidget(m_positionRow);

    // Order is managed by list position; shown read-only for transparency.
    m_orderLabel = new QLabel(this);
    m_orderLabel->setObjectName(QStringLiteral("InspectorHint"));
    layout->addWidget(m_orderLabel);

    // Timestamp: optional; unchecked means "no timestamp" (invalid QDateTime).
    auto *timestampRow = new QWidget(this);
    auto *timestampLayout = new QHBoxLayout(timestampRow);
    timestampLayout->setContentsMargins(0, 0, 0, 0);
    timestampLayout->setSpacing(6);
    m_timestampCheck = new QCheckBox(tr("Timestamp:"), timestampRow);
    m_timestampEdit = new QDateTimeEdit(timestampRow);
    m_timestampEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd HH:mm"));
    m_timestampEdit->setCalendarPopup(true);
    timestampLayout->addWidget(m_timestampCheck);
    timestampLayout->addWidget(m_timestampEdit, 1);
    layout->addWidget(timestampRow);

    // Reactions: emoji + author references stored on the Core message.
    layout->addWidget(new QLabel(tr("Reactions:"), this));
    m_reactionList = new QListWidget(this);
    m_reactionList->setMaximumHeight(88);
    layout->addWidget(m_reactionList);
    auto *reactionRow = new QWidget(this);
    auto *reactionLayout = new QHBoxLayout(reactionRow);
    reactionLayout->setContentsMargins(0, 0, 0, 0);
    reactionLayout->setSpacing(6);
    m_reactionEmojiEdit = new QLineEdit(reactionRow);
    m_reactionEmojiEdit->setPlaceholderText(tr("Emoji"));
    m_reactionEmojiEdit->setMaximumWidth(72);
    m_reactionAuthorCombo = new QComboBox(reactionRow);
    auto *reactionAddButton = new QPushButton(tr("Add"), reactionRow);
    auto *reactionRemoveButton = new QPushButton(tr("Remove"), reactionRow);
    reactionLayout->addWidget(m_reactionEmojiEdit);
    reactionLayout->addWidget(m_reactionAuthorCombo, 1);
    reactionLayout->addWidget(reactionAddButton);
    reactionLayout->addWidget(reactionRemoveButton);
    layout->addWidget(reactionRow);
    layout->addStretch(1);
    setEnabled(false);

    connect(m_authorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index) {
                if (m_loading || m_message.id.isEmpty())
                    return;
                m_message.authorId = m_authorCombo->itemData(index).toString();
                apply();
            });
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index) {
                if (m_loading || m_message.id.isEmpty())
                    return;
                const MessageType type =
                    static_cast<MessageType>(m_typeCombo->itemData(index).toInt());
                m_message.type = type;
                if (type == MessageType::System) {
                    m_message.authorId = QString(); // System messages have no author
                } else if (m_message.authorId.isEmpty() && m_document) {
                    const Conversation *conv =
                        m_document->story().findConversation(m_conversationId);
                    if (conv && !conv->participantIds().isEmpty())
                        m_message.authorId = conv->participantIds().first();
                }
                apply();
                reload(); // refresh author enablement + file row visibility
            });
    connect(m_textEdit, &QPlainTextEdit::textChanged, this, [this]() {
        if (m_loading || m_message.id.isEmpty())
            return;
        m_message.text = m_textEdit->toPlainText();
        apply();
    });
    connect(m_fileRefEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (m_loading || m_message.id.isEmpty())
            return;
        applyFileRef(text);
    });
    connect(browseButton, &QPushButton::clicked, this, [this]() {
        if (m_message.id.isEmpty())
            return;
        const QString path = QFileDialog::getOpenFileName(
            this, tr("Choose File Reference"), QString(), tr("All files (*)"));
        if (!path.isEmpty())
            m_fileRefEdit->setText(path); // textChanged maps it onto the attachment
    });
    connect(m_anchorXSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this]() {
                if (m_loading || m_message.id.isEmpty())
                    return;
                applyAnchor();
            });
    connect(m_anchorYSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this]() {
                if (m_loading || m_message.id.isEmpty())
                    return;
                applyAnchor();
            });
    connect(m_timestampCheck, &QCheckBox::toggled, this, [this]() {
        if (m_loading || m_message.id.isEmpty())
            return;
        m_timestampEdit->setEnabled(m_timestampCheck->isChecked());
        applyTimestamp();
    });
    connect(m_timestampEdit, &QDateTimeEdit::dateTimeChanged, this, [this]() {
        if (m_loading || m_message.id.isEmpty())
            return;
        applyTimestamp();
    });
    connect(reactionAddButton, &QPushButton::clicked, this,
            &MessageEditorWidget::addReaction);
    connect(reactionRemoveButton, &QPushButton::clicked, this,
            &MessageEditorWidget::removeSelectedReaction);
}

void MessageEditorWidget::setDocument(ProjectDocument *document)
{
    if (m_document)
        disconnect(m_document, nullptr, this, nullptr);
    m_document = document;
    if (m_document)
        connect(m_document, &ProjectDocument::projectChanged, this,
                &MessageEditorWidget::onProjectChanged);
    clearMessage();
}

void MessageEditorWidget::setMessage(const QString &conversationId, const QString &messageId)
{
    m_conversationId = conversationId;
    m_message = Message();
    m_message.id = messageId;
    reload();
}

void MessageEditorWidget::clearMessage()
{
    m_conversationId.clear();
    m_message = Message();
    setEnabled(false);
}

void MessageEditorWidget::onProjectChanged()
{
    if (m_applying || m_loading || !m_document || m_message.id.isEmpty())
        return;
    reload();
}

void MessageEditorWidget::reload()
{
    if (!m_document || m_conversationId.isEmpty() || m_message.id.isEmpty()) {
        clearMessage();
        return;
    }
    const Conversation *conv = m_document->story().findConversation(m_conversationId);
    const Message *found = conv ? conv->findMessage(m_message.id) : nullptr;
    if (!found) {
        clearMessage();
        return;
    }
    m_message = *found;

    m_loading = true;
    {
        const QSignalBlocker blockAuthor(m_authorCombo);
        const QSignalBlocker blockType(m_typeCombo);
        const QSignalBlocker blockText(m_textEdit);
        const QSignalBlocker blockFile(m_fileRefEdit);
        const QSignalBlocker blockX(m_anchorXSpin);
        const QSignalBlocker blockY(m_anchorYSpin);
        reloadAuthorCombo();
        const int typeIndex = m_typeCombo->findData(static_cast<int>(m_message.type));
        m_typeCombo->setCurrentIndex(typeIndex < 0 ? 0 : typeIndex);
        m_textEdit->setPlainText(m_message.text);
        m_fileRefEdit->setText(m_message.attachments.isEmpty()
                                   ? QString()
                                   : m_message.attachments.first().fileRef);
        const bool hasAnchor = !m_message.attachments.isEmpty();
        m_anchorXSpin->setValue(
            hasAnchor ? Attachment::clampAnchor(m_message.attachments.first().anchorX) : 0.5);
        m_anchorYSpin->setValue(
            hasAnchor ? Attachment::clampAnchor(m_message.attachments.first().anchorY) : 0.5);
    }
    const bool isSystem = m_message.type == MessageType::System;
    const bool hasFile = m_message.type == MessageType::Image
        || m_message.type == MessageType::File;
    const bool isImage = m_message.type == MessageType::Image;
    m_authorCombo->setEnabled(!isSystem);
    m_fileRefEdit->setEnabled(hasFile);
    m_positionRow->setVisible(isImage);
    // Order display: list position (1-based) plus the persisted order value.
    int position = 0;
    if (const Conversation *ordered = m_document->story().findConversation(m_conversationId)) {
        const QList<Message> &all = ordered->messages();
        for (int i = 0; i < all.size(); ++i) {
            if (all.at(i).id == m_message.id) {
                position = i + 1;
                break;
            }
        }
    }
    m_orderLabel->setText(
        tr("Position %1 of %2 · order value %3")
            .arg(position)
            .arg(m_document->story().findConversation(m_conversationId)->messageCount())
            .arg(m_message.order));
    m_timestampCheck->setChecked(m_message.createdAt.isValid());
    m_timestampEdit->setEnabled(m_message.createdAt.isValid());
    if (m_message.createdAt.isValid())
        m_timestampEdit->setDateTime(m_message.createdAt.toLocalTime());
    reloadReactionAuthors();
    reloadReactions();
    m_loading = false;
    setEnabled(true);
}

void MessageEditorWidget::reloadAuthorCombo()
{
    const QSignalBlocker blockAuthor(m_authorCombo);
    m_authorCombo->clear();
    if (!m_document)
        return;
    m_authorCombo->addItem(tr("(no author)"), QString());
    const Story &story = m_document->story();
    for (const Character &c : story.characters())
        m_authorCombo->addItem(authorLabel(c), c.id);
    if (!m_message.authorId.isEmpty() && !story.findCharacter(m_message.authorId))
        m_authorCombo->addItem(tr("(missing character %1)").arg(m_message.authorId),
                               m_message.authorId);
    const int index = m_authorCombo->findData(m_message.authorId);
    m_authorCombo->setCurrentIndex(index < 0 ? 0 : index);
}

void MessageEditorWidget::apply()
{
    if (!m_document || m_conversationId.isEmpty() || m_message.id.isEmpty())
        return;
    m_applying = true;
    if (!m_document->updateMessage(m_conversationId, m_message))
        reload();
    m_applying = false;
}

void MessageEditorWidget::applyFileRef(const QString &fileRef)
{
    // Maps the reference line onto the first attachment entry: created on
    // demand, updated in place, removed when cleared. No asset processing.
    if (!m_message.attachments.isEmpty()
        && m_message.attachments.first().fileRef == fileRef)
        return;
    if (fileRef.isEmpty()) {
        if (!m_message.attachments.isEmpty())
            m_message.attachments.removeFirst();
    } else if (m_message.attachments.isEmpty()) {
        Attachment attachment;
        attachment.id = IdGenerator::create(QStringLiteral("att"));
        attachment.type = m_message.type == MessageType::Image ? AttachmentType::Image
                                                               : AttachmentType::File;
        attachment.fileName = fileRef.section(QChar::fromLatin1('/'), -1);
        attachment.fileRef = fileRef;
        m_message.attachments.append(attachment);
    } else {
        m_message.attachments.first().fileRef = fileRef;
    }
    apply();
}

void MessageEditorWidget::applyAnchor()
{
    // Writes the spinbox values onto the first attachment's normalized
    // anchor, creating the entry on demand like applyFileRef does.
    const double x = Attachment::clampAnchor(m_anchorXSpin->value());
    const double y = Attachment::clampAnchor(m_anchorYSpin->value());
    if (!m_message.attachments.isEmpty()
        && m_message.attachments.first().anchorX == x
        && m_message.attachments.first().anchorY == y)
        return;
    if (m_message.attachments.isEmpty()) {
        Attachment attachment;
        attachment.id = IdGenerator::create(QStringLiteral("att"));
        attachment.type = AttachmentType::Image;
        attachment.anchorX = x;
        attachment.anchorY = y;
        m_message.attachments.append(attachment);
    } else {
        m_message.attachments.first().anchorX = x;
        m_message.attachments.first().anchorY = y;
    }
    apply();
}

void MessageEditorWidget::applyTimestamp()
{
    const QDateTime value = m_timestampCheck->isChecked()
        ? m_timestampEdit->dateTime().toUTC()
        : QDateTime();
    if (m_message.createdAt == value)
        return;
    m_message.createdAt = value;
    apply();
}

void MessageEditorWidget::reloadReactionAuthors()
{
    const QSignalBlocker blocker(m_reactionAuthorCombo);
    m_reactionAuthorCombo->clear();
    if (!m_document)
        return;
    for (const Character &c : m_document->story().characters())
        m_reactionAuthorCombo->addItem(authorLabel(c), c.id);
}

void MessageEditorWidget::reloadReactions()
{
    const QSignalBlocker blocker(m_reactionList);
    m_reactionList->clear();
    if (!m_document)
        return;
    const Story &story = m_document->story();
    for (const Reaction &reaction : m_message.reactions) {
        QString who = reaction.characterId;
        if (const Character *c = story.findCharacter(reaction.characterId)) {
            who = c->displayName.isEmpty() ? c->username : c->displayName;
        }
        auto *item = new QListWidgetItem(
            QStringLiteral("%1 → %2").arg(reaction.emoji, who), m_reactionList);
        item->setData(Qt::UserRole, reaction.characterId + QChar::fromLatin1('\n')
                                        + reaction.emoji);
    }
}

void MessageEditorWidget::addReaction()
{
    const QString emoji = m_reactionEmojiEdit->text().trimmed();
    if (m_message.id.isEmpty() || emoji.isEmpty())
        return;
    Reaction reaction;
    reaction.emoji = emoji;
    reaction.characterId = m_reactionAuthorCombo->currentData().toString();
    reaction.createdAt = QDateTime::currentDateTimeUtc();
    m_message.reactions.append(reaction);
    m_reactionEmojiEdit->clear();
    apply();
    reloadReactions();
}

void MessageEditorWidget::removeSelectedReaction()
{
    if (m_message.id.isEmpty())
        return;
    const QList<QListWidgetItem *> selected = m_reactionList->selectedItems();
    if (selected.isEmpty())
        return;
    const QString key = selected.first()->data(Qt::UserRole).toString();
    for (int i = 0; i < m_message.reactions.size(); ++i) {
        const Reaction &reaction = m_message.reactions.at(i);
        if (reaction.characterId + QChar::fromLatin1('\n') + reaction.emoji == key) {
            m_message.reactions.removeAt(i);
            apply();
            reloadReactions();
            return;
        }
    }
}
