#include "ui/InspectorPanel.h"

#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "document/ProjectDocument.h"
#include "ui/authoring/CharacterEditorWidget.h"
#include "ui/authoring/ConversationEditorWidget.h"
#include "ui/authoring/MessageEditorWidget.h"
#include "ui/authoring/SceneEditorWidget.h"
#include "ui/authoring/SceneEventEditorWidget.h"

InspectorPanel::InspectorPanel(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("KonataInspectorPanel"));
    setMinimumWidth(200);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *header = new QLabel(tr("INSPECTOR"), this);
    header->setObjectName(QStringLiteral("PanelHeader"));
    layout->addWidget(header);

    m_stack = new QStackedWidget(this);
    layout->addWidget(m_stack, 1);

    m_emptyPage = new QWidget(m_stack);
    m_emptyPage->setObjectName(QStringLiteral("InspectorBody"));
    auto *emptyLayout = new QVBoxLayout(m_emptyPage);
    emptyLayout->setAlignment(Qt::AlignCenter);

    auto *state = new QLabel(tr("No object selected"), m_emptyPage);
    state->setObjectName(QStringLiteral("InspectorEmpty"));
    state->setAlignment(Qt::AlignCenter);

    auto *hint = new QLabel(tr("Select a project item to inspect and edit it."), m_emptyPage);
    hint->setObjectName(QStringLiteral("InspectorHint"));
    hint->setAlignment(Qt::AlignCenter);
    hint->setWordWrap(true);

    emptyLayout->addWidget(state);
    emptyLayout->addWidget(hint);

    m_characterEditor = new CharacterEditorWidget(m_stack);
    m_conversationEditor = new ConversationEditorWidget(m_stack);
    m_messageEditor = new MessageEditorWidget(m_stack);
    m_sceneEditor = new SceneEditorWidget(m_stack);
    m_eventEditor = new SceneEventEditorWidget(m_stack);

    m_stack->addWidget(m_emptyPage);
    m_stack->addWidget(m_characterEditor);
    m_stack->addWidget(m_conversationEditor);
    m_stack->addWidget(m_messageEditor);
    m_stack->addWidget(m_sceneEditor);
    m_stack->addWidget(m_eventEditor);
    m_stack->setCurrentWidget(m_emptyPage);
}

void InspectorPanel::setDocument(ProjectDocument *document)
{
    if (m_document)
        disconnect(m_document, nullptr, this, nullptr);
    m_document = document;
    m_characterEditor->setDocument(document);
    m_conversationEditor->setDocument(document);
    m_messageEditor->setDocument(document);
    m_sceneEditor->setDocument(document);
    m_eventEditor->setDocument(document);
    if (m_document)
        connect(m_document, &ProjectDocument::selectionChanged, this,
                &InspectorPanel::showEditorForSelection);
    showEditorForSelection();
}

void InspectorPanel::showEditorForSelection()
{
    if (!m_document) {
        m_stack->setCurrentWidget(m_emptyPage);
        return;
    }
    const ProjectDocument::Selection selection = m_document->selection();
    switch (selection.type) {
    case ProjectDocument::SelectionType::Character:
        m_characterEditor->setCharacterId(selection.id);
        m_stack->setCurrentWidget(m_characterEditor);
        break;
    case ProjectDocument::SelectionType::Conversation:
        m_conversationEditor->setConversationId(selection.id);
        m_stack->setCurrentWidget(m_conversationEditor);
        break;
    case ProjectDocument::SelectionType::Message:
        m_messageEditor->setMessage(selection.parentId, selection.id);
        m_stack->setCurrentWidget(m_messageEditor);
        break;
    case ProjectDocument::SelectionType::Scene:
        m_sceneEditor->setSceneId(selection.id);
        m_stack->setCurrentWidget(m_sceneEditor);
        break;
    case ProjectDocument::SelectionType::SceneEvent:
        m_eventEditor->setSceneEvent(selection.parentId, selection.id);
        m_stack->setCurrentWidget(m_eventEditor);
        break;
    case ProjectDocument::SelectionType::None:
        m_stack->setCurrentWidget(m_emptyPage);
        break;
    }
}

void InspectorPanel::focusEditableField()
{
    QWidget *page = m_stack->currentWidget();
    if (!page || page == m_emptyPage)
        return;
    // Prefer the message text field, otherwise the first enabled line edit.
    const QList<QPlainTextEdit *> plainEdits = page->findChildren<QPlainTextEdit *>();
    for (QPlainTextEdit *edit : plainEdits) {
        if (edit->isEnabled()) {
            edit->setFocus(Qt::OtherFocusReason);
            return;
        }
    }
    const QList<QLineEdit *> lineEdits = page->findChildren<QLineEdit *>();
    for (QLineEdit *edit : lineEdits) {
        if (edit->isEnabled() && edit->isVisible()) {
            edit->setFocus(Qt::OtherFocusReason);
            return;
        }
    }
}
