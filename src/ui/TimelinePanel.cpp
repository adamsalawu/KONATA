#include "ui/TimelinePanel.h"

#include <QLabel>
#include <QVBoxLayout>

#include "document/ProjectDocument.h"

TimelinePanel::TimelinePanel(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("KonataTimelinePanel"));
    setMinimumHeight(120);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *header = new QLabel(tr("TIMELINE"), this);
    header->setObjectName(QStringLiteral("PanelHeader"));
    layout->addWidget(header);

    // Read-only context readout (no playback in this milestone): which
    // conversation/message the author is currently working on.
    m_contextLabel = new QLabel(this);
    m_contextLabel->setObjectName(QStringLiteral("TimelineContext"));
    m_contextLabel->setAlignment(Qt::AlignCenter);
    m_contextLabel->setWordWrap(true);
    layout->addWidget(m_contextLabel);

    auto *body = new QLabel(
        tr("Timeline region — functionality arrives in a later milestone."), this);
    body->setObjectName(QStringLiteral("TimelinePlaceholder"));
    body->setAlignment(Qt::AlignCenter);

    layout->addWidget(body, 1);
}

void TimelinePanel::setDocument(ProjectDocument *document)
{
    if (m_document)
        disconnect(m_document, nullptr, this, nullptr);
    m_document = document;
    if (m_document) {
        connect(m_document, &ProjectDocument::projectChanged, this,
                &TimelinePanel::refresh);
        connect(m_document, &ProjectDocument::selectionChanged, this,
                &TimelinePanel::refresh);
    }
    refresh();
}

void TimelinePanel::refresh()
{
    if (!m_document) {
        m_contextLabel->clear();
        return;
    }
    const Konata::Core::Story &story = m_document->story();
    const ProjectDocument::Selection selection = m_document->selection();
    QString conversationId;
    QString messageId;
    switch (selection.type) {
    case ProjectDocument::SelectionType::Conversation:
        conversationId = selection.id;
        break;
    case ProjectDocument::SelectionType::Message:
        conversationId = selection.parentId;
        messageId = selection.id;
        break;
    default:
        break;
    }
    if (conversationId.isEmpty()) {
        m_contextLabel->setText(tr("No conversation selected."));
        return;
    }
    const Konata::Core::Conversation *conv = story.findConversation(conversationId);
    if (!conv) {
        m_contextLabel->clear();
        return;
    }
    QString context = tr("Chat: %1 · %2 messages")
                          .arg(conv->title().isEmpty() ? tr("(untitled)") : conv->title())
                          .arg(conv->messageCount());
    if (!messageId.isEmpty()) {
        if (const Konata::Core::Message *message = conv->findMessage(messageId)) {
            QString preview = message->text.split(QChar::fromLatin1('\n')).first().left(40);
            if (preview.isEmpty())
                preview = tr("(empty message)");
            context += tr(" · selected: %1").arg(preview);
        }
    }
    m_contextLabel->setText(context);
}
