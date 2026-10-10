#include "ui/authoring/SceneEventEditorWidget.h"

#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "document/ProjectDocument.h"

using namespace Konata::Core;

SceneEventEditorWidget::SceneEventEditorWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    auto *form = new QFormLayout();
    form->setSpacing(6);

    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem(typeLabel(SceneEventType::SendMessage),
                         static_cast<int>(SceneEventType::SendMessage));
    m_typeCombo->addItem(typeLabel(SceneEventType::Wait),
                         static_cast<int>(SceneEventType::Wait));
    m_typeCombo->addItem(typeLabel(SceneEventType::StartTyping),
                         static_cast<int>(SceneEventType::StartTyping));
    m_typeCombo->addItem(typeLabel(SceneEventType::StopTyping),
                         static_cast<int>(SceneEventType::StopTyping));
    m_typeCombo->addItem(typeLabel(SceneEventType::Notification),
                         static_cast<int>(SceneEventType::Notification));
    form->addRow(tr("Event type:"), m_typeCombo);
    layout->addLayout(form);

    m_payloadStack = new QStackedWidget(this);
    layout->addWidget(m_payloadStack);

    // SendMessage page
    auto *sendPage = new QWidget(m_payloadStack);
    auto *sendForm = new QFormLayout(sendPage);
    m_sendConversationCombo = new QComboBox(sendPage);
    m_sendMessageCombo = new QComboBox(sendPage);
    sendForm->addRow(tr("Conversation:"), m_sendConversationCombo);
    sendForm->addRow(tr("Message:"), m_sendMessageCombo);

    // Wait page
    auto *waitPage = new QWidget(m_payloadStack);
    auto *waitForm = new QFormLayout(waitPage);
    m_waitSpin = new QSpinBox(waitPage);
    m_waitSpin->setRange(0, 3600000);
    m_waitSpin->setSingleStep(250);
    m_waitSpin->setSuffix(tr(" ms"));
    waitForm->addRow(tr("Duration:"), m_waitSpin);

    // Typing page (shared by Start/StopTyping)
    auto *typingPage = new QWidget(m_payloadStack);
    auto *typingForm = new QFormLayout(typingPage);
    m_typingCharacterCombo = new QComboBox(typingPage);
    m_typingConversationCombo = new QComboBox(typingPage);
    typingForm->addRow(tr("Character:"), m_typingCharacterCombo);
    typingForm->addRow(tr("Conversation:"), m_typingConversationCombo);

    // Notification page
    auto *notificationPage = new QWidget(m_payloadStack);
    auto *notificationForm = new QFormLayout(notificationPage);
    m_notificationEdit = new QLineEdit(notificationPage);
    m_notificationEdit->setPlaceholderText(tr("Notification text"));
    notificationForm->addRow(tr("Text:"), m_notificationEdit);

    m_payloadStack->addWidget(sendPage);        // 0
    m_payloadStack->addWidget(waitPage);        // 1
    m_payloadStack->addWidget(typingPage);      // 2
    m_payloadStack->addWidget(notificationPage);// 3
    layout->addStretch(1);
    setEnabled(false);

    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index) {
                if (m_loading || m_event.id.isEmpty())
                    return;
                const SceneEventType type =
                    static_cast<SceneEventType>(m_typeCombo->itemData(index).toInt());
                if (type == m_event.type)
                    return;
                // Rebuild the payload for the new type via the Core factories,
                // preserving identity and shared references where meaningful.
                SceneEvent rebuilt;
                switch (type) {
                case SceneEventType::SendMessage:
                    rebuilt = SceneEvent::sendMessage(m_event.conversationId, m_event.messageId);
                    rebuilt.characterId = m_event.characterId;
                    break;
                case SceneEventType::Wait:
                    rebuilt = SceneEvent::wait(1000);
                    rebuilt.conversationId = m_event.conversationId;
                    break;
                case SceneEventType::StartTyping:
                    rebuilt = SceneEvent::typing(m_event.characterId, m_event.conversationId, true);
                    break;
                case SceneEventType::StopTyping:
                    rebuilt =
                        SceneEvent::typing(m_event.characterId, m_event.conversationId, false);
                    break;
                case SceneEventType::Notification:
                    rebuilt = SceneEvent::notification(m_event.text);
                    rebuilt.conversationId = m_event.conversationId;
                    break;
                }
                rebuilt.id = m_event.id;
                m_event = rebuilt;
                apply();
                reload();
            });
    connect(m_sendConversationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index) {
                if (m_loading || m_applying || m_event.id.isEmpty())
                    return;
                m_event.conversationId = m_sendConversationCombo->itemData(index).toString();
                m_event.messageId = QString(); // message list depends on conversation
                apply();
                reloadMessageCombo();
            });
    connect(m_sendMessageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index) {
                if (m_loading || m_applying || m_event.id.isEmpty())
                    return;
                m_event.messageId = m_sendMessageCombo->itemData(index).toString();
                apply();
            });
    connect(m_waitSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
        if (m_loading || m_event.id.isEmpty())
            return;
        m_event.waitMs = value;
        apply();
    });
    connect(m_typingCharacterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index) {
                if (m_loading || m_applying || m_event.id.isEmpty())
                    return;
                m_event.characterId = m_typingCharacterCombo->itemData(index).toString();
                apply();
            });
    connect(m_typingConversationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index) {
                if (m_loading || m_applying || m_event.id.isEmpty())
                    return;
                m_event.conversationId =
                    m_typingConversationCombo->itemData(index).toString();
                apply();
            });
    connect(m_notificationEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (m_loading || m_event.id.isEmpty())
            return;
        m_event.text = text;
        apply();
    });
}

void SceneEventEditorWidget::setDocument(ProjectDocument *document)
{
    if (m_document)
        disconnect(m_document, nullptr, this, nullptr);
    m_document = document;
    if (m_document)
        connect(m_document, &ProjectDocument::projectChanged, this,
                &SceneEventEditorWidget::onProjectChanged);
    clearEvent();
}

void SceneEventEditorWidget::setSceneEvent(const QString &sceneId, const QString &eventId)
{
    m_sceneId = sceneId;
    m_event = SceneEvent();
    m_event.id = eventId;
    reload();
}

void SceneEventEditorWidget::clearEvent()
{
    m_sceneId.clear();
    m_event = SceneEvent();
    setEnabled(false);
}

QString SceneEventEditorWidget::typeLabel(SceneEventType type)
{
    switch (type) {
    case SceneEventType::SendMessage:
        return tr("Send message");
    case SceneEventType::Wait:
        return tr("Wait");
    case SceneEventType::StartTyping:
        return tr("Start typing");
    case SceneEventType::StopTyping:
        return tr("Stop typing");
    case SceneEventType::Notification:
        return tr("Notification");
    }
    return tr("Unknown");
}

void SceneEventEditorWidget::onProjectChanged()
{
    if (m_applying || m_loading || !m_document || m_event.id.isEmpty())
        return;
    reload();
}

void SceneEventEditorWidget::reload()
{
    if (!m_document || m_sceneId.isEmpty() || m_event.id.isEmpty()) {
        clearEvent();
        return;
    }
    const Scene *scene = m_document->story().findScene(m_sceneId);
    const SceneEvent *found = scene ? scene->findEvent(m_event.id) : nullptr;
    if (!found) {
        clearEvent();
        return;
    }
    m_event = *found;

    m_loading = true;
    {
        const QSignalBlocker blockType(m_typeCombo);
        const int typeIndex = m_typeCombo->findData(static_cast<int>(m_event.type));
        m_typeCombo->setCurrentIndex(typeIndex < 0 ? 0 : typeIndex);
    }
    reloadConversationCombos();
    reloadMessageCombo();
    reloadCharacterCombo();
    {
        const QSignalBlocker blockWait(m_waitSpin);
        const QSignalBlocker blockNotification(m_notificationEdit);
        m_waitSpin->setValue(m_event.waitMs);
        m_notificationEdit->setText(m_event.text);
    }
    switch (m_event.type) {
    case SceneEventType::SendMessage:
        m_payloadStack->setCurrentIndex(0);
        break;
    case SceneEventType::Wait:
        m_payloadStack->setCurrentIndex(1);
        break;
    case SceneEventType::StartTyping:
    case SceneEventType::StopTyping:
        m_payloadStack->setCurrentIndex(2);
        break;
    case SceneEventType::Notification:
        m_payloadStack->setCurrentIndex(3);
        break;
    }
    m_loading = false;
    setEnabled(true);
}

void SceneEventEditorWidget::reloadConversationCombos()
{
    const QSignalBlocker blockSend(m_sendConversationCombo);
    const QSignalBlocker blockTyping(m_typingConversationCombo);
    m_sendConversationCombo->clear();
    m_typingConversationCombo->clear();
    if (!m_document)
        return;
    m_sendConversationCombo->addItem(tr("(none)"), QString());
    m_typingConversationCombo->addItem(tr("(none)"), QString());
    const Story &story = m_document->story();
    for (const Conversation &c : story.conversations()) {
        const QString label =
            c.title().isEmpty() ? tr("(untitled conversation)") : c.title();
        m_sendConversationCombo->addItem(label, c.id());
        m_typingConversationCombo->addItem(label, c.id());
    }
    for (QComboBox *combo : {m_sendConversationCombo, m_typingConversationCombo}) {
        if (!m_event.conversationId.isEmpty()
            && combo->findData(m_event.conversationId) < 0)
            combo->addItem(tr("(missing conversation %1)").arg(m_event.conversationId),
                           m_event.conversationId);
        const int index = combo->findData(m_event.conversationId);
        combo->setCurrentIndex(index < 0 ? 0 : index);
    }
}

void SceneEventEditorWidget::reloadMessageCombo()
{
    const QSignalBlocker blockMessage(m_sendMessageCombo);
    m_sendMessageCombo->clear();
    m_sendMessageCombo->addItem(tr("(none)"), QString());
    if (!m_document)
        return;
    const Conversation *conv =
        m_document->story().findConversation(m_event.conversationId);
    if (conv) {
        for (const Message &m : conv->messages()) {
            const Character *author = m_document->story().findCharacter(m.authorId);
            const QString who = author && !author->displayName.isEmpty()
                ? author->displayName
                : tr("?");
            const QString preview =
                m.text.size() > 32 ? m.text.left(32) + QStringLiteral("…") : m.text;
            m_sendMessageCombo->addItem(tr("[%1] %2").arg(who, preview), m.id);
        }
    }
    if (!m_event.messageId.isEmpty() && m_sendMessageCombo->findData(m_event.messageId) < 0)
        m_sendMessageCombo->addItem(tr("(missing message %1)").arg(m_event.messageId),
                                    m_event.messageId);
    const int index = m_sendMessageCombo->findData(m_event.messageId);
    m_sendMessageCombo->setCurrentIndex(index < 0 ? 0 : index);
}

void SceneEventEditorWidget::reloadCharacterCombo()
{
    const QSignalBlocker blockCharacter(m_typingCharacterCombo);
    m_typingCharacterCombo->clear();
    if (!m_document)
        return;
    m_typingCharacterCombo->addItem(tr("(none)"), QString());
    const Story &story = m_document->story();
    for (const Character &c : story.characters()) {
        const QString label = c.displayName.isEmpty()
            ? tr("(unnamed character)")
            : QStringLiteral("%1 (@%2)").arg(c.displayName, c.username);
        m_typingCharacterCombo->addItem(label, c.id);
    }
    if (!m_event.characterId.isEmpty()
        && m_typingCharacterCombo->findData(m_event.characterId) < 0)
        m_typingCharacterCombo->addItem(
            tr("(missing character %1)").arg(m_event.characterId), m_event.characterId);
    const int index = m_typingCharacterCombo->findData(m_event.characterId);
    m_typingCharacterCombo->setCurrentIndex(index < 0 ? 0 : index);
}

void SceneEventEditorWidget::apply()
{
    if (!m_document || m_sceneId.isEmpty() || m_event.id.isEmpty())
        return;
    m_applying = true;
    if (!m_document->updateEvent(m_sceneId, m_event))
        reload();
    m_applying = false;
}
