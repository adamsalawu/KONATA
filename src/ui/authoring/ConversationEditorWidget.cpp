#include "ui/authoring/ConversationEditorWidget.h"

#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include "document/ProjectDocument.h"

using namespace Konata::Core;

ConversationEditorWidget::ConversationEditorWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    auto *form = new QFormLayout();
    form->setSpacing(6);

    m_titleEdit = new QLineEdit(this);
    m_titleEdit->setPlaceholderText(tr("Conversation title"));
    form->addRow(tr("Title:"), m_titleEdit);

    m_skinLabel = new QLabel(this);
    m_skinLabel->setObjectName(QStringLiteral("InspectorHint"));
    m_skinLabel->setWordWrap(true);
    form->addRow(tr("Skin:"), m_skinLabel);

    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem(tr("Private"), static_cast<int>(ConversationType::Private));
    m_typeCombo->addItem(tr("Group"), static_cast<int>(ConversationType::Group));
    form->addRow(tr("Type:"), m_typeCombo);

    layout->addLayout(form);

    auto *participantsLabel = new QLabel(tr("Participants (from Story characters):"), this);
    layout->addWidget(participantsLabel);

    m_participantList = new QListWidget(this);
    m_participantList->setSelectionMode(QAbstractItemView::NoSelection);
    m_participantList->setMaximumHeight(110);
    layout->addWidget(m_participantList);

    auto *messagesLabel = new QLabel(tr("Messages (in conversation order):"), this);
    layout->addWidget(messagesLabel);

    // Plain authoring list — not a chat preview. No bubbles, no mock styling.
    m_messageList = new QListWidget(this);
    m_messageList->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_messageList, 1);

    auto *messageButtons = new QWidget(this);
    auto *messageButtonLayout = new QHBoxLayout(messageButtons);
    messageButtonLayout->setContentsMargins(0, 0, 0, 0);
    messageButtonLayout->setSpacing(6);
    m_addMessageButton = new QPushButton(tr("+ Message"), messageButtons);
    m_deleteMessageButton = new QPushButton(tr("Delete"), messageButtons);
    messageButtonLayout->addWidget(m_addMessageButton);
    messageButtonLayout->addWidget(m_deleteMessageButton);
    messageButtonLayout->addStretch(1);
    layout->addWidget(messageButtons);
    setEnabled(false);

    connect(m_titleEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (m_loading || m_conversation.id().isEmpty())
            return;
        m_conversation.setTitle(text);
        apply();
    });
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index) {
                if (m_loading || m_conversation.id().isEmpty())
                    return;
                m_conversation.setType(
                    static_cast<ConversationType>(m_typeCombo->itemData(index).toInt()));
                apply();
            });
    connect(m_participantList, &QListWidget::itemChanged, this,
            [this](QListWidgetItem *) {
                if (m_loading || m_applying || m_conversation.id().isEmpty())
                    return;
                QStringList ids;
                for (int i = 0; i < m_participantList->count(); ++i) {
                    const QListWidgetItem *item = m_participantList->item(i);
                    if (item->checkState() == Qt::Checked)
                        ids.append(item->data(Qt::UserRole).toString());
                }
                m_conversation.setParticipantIds(ids);
                apply();
            });
    connect(m_messageList, &QListWidget::itemSelectionChanged, this,
            &ConversationEditorWidget::onMessageSelection);
    connect(m_addMessageButton, &QPushButton::clicked, this,
            &ConversationEditorWidget::addMessage);
    connect(m_deleteMessageButton, &QPushButton::clicked, this,
            &ConversationEditorWidget::deleteSelectedMessage);
}

void ConversationEditorWidget::setDocument(ProjectDocument *document)
{
    if (m_document)
        disconnect(m_document, nullptr, this, nullptr);
    m_document = document;
    if (m_document) {
        connect(m_document, &ProjectDocument::projectChanged, this,
                &ConversationEditorWidget::onProjectChanged);
        connect(m_document, &ProjectDocument::selectionChanged, this,
                &ConversationEditorWidget::onSelectionChanged);
    }
    clearConversation();
}

void ConversationEditorWidget::setConversationId(const QString &conversationId)
{
    m_conversation = Conversation();
    m_conversation.setId(conversationId);
    reload();
}

void ConversationEditorWidget::clearConversation()
{
    m_conversation = Conversation();
    setEnabled(false);
}

void ConversationEditorWidget::onProjectChanged()
{
    if (m_applying || m_loading || !m_document || m_conversation.id().isEmpty())
        return;
    reload();
}

void ConversationEditorWidget::onSelectionChanged()
{
    if (!m_document || m_conversation.id().isEmpty() || !isEnabled())
        return;
    syncMessageSelection();
}

void ConversationEditorWidget::onMessageSelection()
{
    if (m_loading || !m_document || m_conversation.id().isEmpty())
        return;
    const QList<QListWidgetItem *> selected = m_messageList->selectedItems();
    if (selected.isEmpty()) {
        // Only clear when the vanished selection was one of our messages;
        // the parent Conversation selection itself must survive.
        const ProjectDocument::Selection selection = m_document->selection();
        if (selection.type == ProjectDocument::SelectionType::Message
            && selection.parentId == m_conversation.id())
            m_document->setSelection(ProjectDocument::SelectionType::Conversation,
                                     m_conversation.id());
        return;
    }
    m_document->setSelection(ProjectDocument::SelectionType::Message,
                             selected.first()->data(Qt::UserRole).toString(),
                             m_conversation.id());
}

void ConversationEditorWidget::reload()
{
    if (!m_document || m_conversation.id().isEmpty()) {
        clearConversation();
        return;
    }
    const Conversation *found = m_document->story().findConversation(m_conversation.id());
    if (!found) {
        clearConversation();
        return;
    }
    m_conversation = *found;

    m_loading = true;
    {
        const QSignalBlocker blockTitle(m_titleEdit);
        const QSignalBlocker blockType(m_typeCombo);
        m_titleEdit->setText(m_conversation.title());
        const int index = m_typeCombo->findData(static_cast<int>(m_conversation.type()));
        m_typeCombo->setCurrentIndex(index < 0 ? 0 : index);
    }
    reloadParticipantList();
    reloadMessageList();
    syncMessageSelection();
    // Read-only proof of which skin renders this conversation.
    m_skinLabel->setText(m_document->project().appDefinition.appName);
    m_loading = false;
    setEnabled(true);
}

void ConversationEditorWidget::reloadParticipantList()
{
    const QSignalBlocker blockList(m_participantList);
    m_participantList->clear();
    if (!m_document)
        return;

    const Story &story = m_document->story();
    for (const Character &c : story.characters()) {
        const QString label = c.displayName.isEmpty()
            ? tr("(unnamed character)")
            : QStringLiteral("%1 (@%2)").arg(c.displayName, c.username);
        auto *item = new QListWidgetItem(label, m_participantList);
        item->setData(Qt::UserRole, c.id);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(m_conversation.hasParticipant(c.id) ? Qt::Checked : Qt::Unchecked);
    }
    // Referenced IDs with no matching character (e.g. from an older file) are
    // shown as-is so the data is never silently dropped by the editor.
    for (const QString &id : m_conversation.participantIds()) {
        if (!story.findCharacter(id)) {
            auto *item = new QListWidgetItem(tr("(missing character %1)").arg(id),
                                             m_participantList);
            item->setData(Qt::UserRole, id);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Checked);
        }
    }
}

void ConversationEditorWidget::reloadMessageList()
{
    const QSignalBlocker blockList(m_messageList);
    m_messageList->clear();
    for (const Message &m : m_conversation.messages()) {
        auto *item = new QListWidgetItem(messageRowLabel(m), m_messageList);
        item->setData(Qt::UserRole, m.id);
    }
    m_deleteMessageButton->setEnabled(false);
}

void ConversationEditorWidget::syncMessageSelection()
{
    const QSignalBlocker blockList(m_messageList);
    const ProjectDocument::Selection selection = m_document->selection();
    m_messageList->clearSelection();
    m_deleteMessageButton->setEnabled(false);
    if (selection.type != ProjectDocument::SelectionType::Message
        || selection.parentId != m_conversation.id())
        return;
    for (int i = 0; i < m_messageList->count(); ++i) {
        QListWidgetItem *item = m_messageList->item(i);
        if (item->data(Qt::UserRole).toString() == selection.id) {
            item->setSelected(true);
            m_messageList->setCurrentRow(i);
            m_deleteMessageButton->setEnabled(true);
            return;
        }
    }
}

void ConversationEditorWidget::addMessage()
{
    if (!m_document || m_conversation.id().isEmpty())
        return;
    const Message created = m_document->createMessage(m_conversation.id());
    if (created.id.isEmpty())
        return;
    m_document->setSelection(ProjectDocument::SelectionType::Message, created.id,
                             m_conversation.id());
}

void ConversationEditorWidget::deleteSelectedMessage()
{
    if (!m_document || m_conversation.id().isEmpty())
        return;
    const QList<QListWidgetItem *> selected = m_messageList->selectedItems();
    if (selected.isEmpty())
        return;
    m_document->removeMessage(m_conversation.id(),
                              selected.first()->data(Qt::UserRole).toString());
}

void ConversationEditorWidget::apply()
{
    if (!m_document || m_conversation.id().isEmpty())
        return;
    m_applying = true;
    if (!m_document->updateConversation(m_conversation))
        reload();
    m_applying = false;
}

QString ConversationEditorWidget::messageRowLabel(const Message &message) const
{
    QString who = tr("?");
    if (message.type == MessageType::System) {
        who = tr("System");
    } else if (m_document) {
        const Character *author = m_document->story().findCharacter(message.authorId);
        if (author && !author->displayName.isEmpty())
            who = author->displayName;
        else if (!message.authorId.isEmpty())
            who = tr("?%1").arg(message.authorId.right(6));
    }
    QString preview = message.text.split(QChar::fromLatin1('\n')).first().left(40);
    if (preview.isEmpty())
        preview = tr("(empty message)");
    return tr("[%1] %2").arg(who, preview);
}
