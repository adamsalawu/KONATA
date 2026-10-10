#include "ui/authoring/SceneEditorWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include "document/ProjectDocument.h"
#include "ui/authoring/SceneEventEditorWidget.h"

using namespace Konata::Core;

SceneEditorWidget::SceneEditorWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    auto *nameLabel = new QLabel(tr("Name:"), this);
    layout->addWidget(nameLabel);
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(tr("Scene name"));
    layout->addWidget(m_nameEdit);

    auto *descriptionLabel = new QLabel(tr("Description:"), this);
    layout->addWidget(descriptionLabel);
    m_descriptionEdit = new QPlainTextEdit(this);
    m_descriptionEdit->setPlaceholderText(tr("What happens in this scene?"));
    m_descriptionEdit->setMaximumBlockCount(200);
    m_descriptionEdit->setMaximumHeight(80);
    layout->addWidget(m_descriptionEdit);

    m_eventCountLabel = new QLabel(this);
    m_eventCountLabel->setObjectName(QStringLiteral("InspectorHint"));
    layout->addWidget(m_eventCountLabel);

    // Structured event list in scene order — authoring rows, not a timeline.
    m_eventList = new QListWidget(this);
    m_eventList->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_eventList, 1);

    auto *eventButtons = new QWidget(this);
    auto *eventButtonLayout = new QHBoxLayout(eventButtons);
    eventButtonLayout->setContentsMargins(0, 0, 0, 0);
    eventButtonLayout->setSpacing(6);
    m_addEventButton = new QPushButton(tr("+ Event"), eventButtons);
    auto *addMenu = new QMenu(m_addEventButton);
    addMenu->addAction(SceneEventEditorWidget::typeLabel(SceneEventType::SendMessage),
                       this, [this]() { addEventOfType(SceneEventType::SendMessage); });
    addMenu->addAction(SceneEventEditorWidget::typeLabel(SceneEventType::Wait), this,
                       [this]() { addEventOfType(SceneEventType::Wait); });
    addMenu->addAction(SceneEventEditorWidget::typeLabel(SceneEventType::StartTyping), this,
                       [this]() { addEventOfType(SceneEventType::StartTyping); });
    addMenu->addAction(SceneEventEditorWidget::typeLabel(SceneEventType::StopTyping), this,
                       [this]() { addEventOfType(SceneEventType::StopTyping); });
    addMenu->addAction(SceneEventEditorWidget::typeLabel(SceneEventType::Notification), this,
                       [this]() { addEventOfType(SceneEventType::Notification); });
    m_addEventButton->setMenu(addMenu);
    m_deleteEventButton = new QPushButton(tr("Delete"), eventButtons);
    eventButtonLayout->addWidget(m_addEventButton);
    eventButtonLayout->addWidget(m_deleteEventButton);
    eventButtonLayout->addStretch(1);
    layout->addWidget(eventButtons);
    setEnabled(false);

    connect(m_nameEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (m_loading || m_scene.id().isEmpty())
            return;
        m_scene.setName(text);
        apply();
    });
    connect(m_descriptionEdit, &QPlainTextEdit::textChanged, this, [this]() {
        if (m_loading || m_scene.id().isEmpty())
            return;
        m_scene.setDescription(m_descriptionEdit->toPlainText());
        apply();
    });
    connect(m_eventList, &QListWidget::itemSelectionChanged, this,
            &SceneEditorWidget::onEventSelection);
    connect(m_deleteEventButton, &QPushButton::clicked, this,
            &SceneEditorWidget::deleteSelectedEvent);
}

void SceneEditorWidget::setDocument(ProjectDocument *document)
{
    if (m_document)
        disconnect(m_document, nullptr, this, nullptr);
    m_document = document;
    if (m_document) {
        connect(m_document, &ProjectDocument::projectChanged, this,
                &SceneEditorWidget::onProjectChanged);
        connect(m_document, &ProjectDocument::selectionChanged, this,
                &SceneEditorWidget::onSelectionChanged);
    }
    clearScene();
}

void SceneEditorWidget::setSceneId(const QString &sceneId)
{
    m_scene = Scene();
    m_scene.setId(sceneId);
    reload();
}

void SceneEditorWidget::clearScene()
{
    m_scene = Scene();
    setEnabled(false);
}

void SceneEditorWidget::onProjectChanged()
{
    if (m_applying || m_loading || !m_document || m_scene.id().isEmpty())
        return;
    reload();
}

void SceneEditorWidget::onSelectionChanged()
{
    if (!m_document || m_scene.id().isEmpty() || !isEnabled())
        return;
    syncEventSelection();
}

void SceneEditorWidget::onEventSelection()
{
    if (m_loading || !m_document || m_scene.id().isEmpty())
        return;
    const QList<QListWidgetItem *> selected = m_eventList->selectedItems();
    if (selected.isEmpty()) {
        const ProjectDocument::Selection selection = m_document->selection();
        if (selection.type == ProjectDocument::SelectionType::SceneEvent
            && selection.parentId == m_scene.id())
            m_document->setSelection(ProjectDocument::SelectionType::Scene, m_scene.id());
        return;
    }
    m_document->setSelection(ProjectDocument::SelectionType::SceneEvent,
                             selected.first()->data(Qt::UserRole).toString(), m_scene.id());
}

void SceneEditorWidget::reload()
{
    if (!m_document || m_scene.id().isEmpty()) {
        clearScene();
        return;
    }
    const Scene *found = m_document->story().findScene(m_scene.id());
    if (!found) {
        clearScene();
        return;
    }
    m_scene = *found;

    m_loading = true;
    {
        const QSignalBlocker blockName(m_nameEdit);
        const QSignalBlocker blockDescription(m_descriptionEdit);
        m_nameEdit->setText(m_scene.name());
        m_descriptionEdit->setPlainText(m_scene.description());
    }
    m_eventCountLabel->setText(tr("%n event(s) in scene order.", "", m_scene.eventCount()));
    reloadEventList();
    syncEventSelection();
    m_loading = false;
    setEnabled(true);
}

void SceneEditorWidget::reloadEventList()
{
    const QSignalBlocker blockList(m_eventList);
    m_eventList->clear();
    int index = 1;
    for (const SceneEvent &e : m_scene.events()) {
        auto *item = new QListWidgetItem(
            tr("%1. %2").arg(index++).arg(SceneEventEditorWidget::typeLabel(e.type)),
            m_eventList);
        item->setData(Qt::UserRole, e.id);
    }
    m_deleteEventButton->setEnabled(false);
}

void SceneEditorWidget::syncEventSelection()
{
    const QSignalBlocker blockList(m_eventList);
    const ProjectDocument::Selection selection = m_document->selection();
    m_eventList->clearSelection();
    m_deleteEventButton->setEnabled(false);
    if (selection.type != ProjectDocument::SelectionType::SceneEvent
        || selection.parentId != m_scene.id())
        return;
    for (int i = 0; i < m_eventList->count(); ++i) {
        QListWidgetItem *item = m_eventList->item(i);
        if (item->data(Qt::UserRole).toString() == selection.id) {
            item->setSelected(true);
            m_eventList->setCurrentRow(i);
            m_deleteEventButton->setEnabled(true);
            return;
        }
    }
}

void SceneEditorWidget::addEventOfType(SceneEventType type)
{
    if (!m_document || m_scene.id().isEmpty())
        return;
    const SceneEvent created = m_document->createEvent(m_scene.id(), type);
    if (created.id.isEmpty())
        return;
    m_document->setSelection(ProjectDocument::SelectionType::SceneEvent, created.id,
                             m_scene.id());
}

void SceneEditorWidget::deleteSelectedEvent()
{
    if (!m_document || m_scene.id().isEmpty())
        return;
    const QList<QListWidgetItem *> selected = m_eventList->selectedItems();
    if (selected.isEmpty())
        return;
    m_document->removeEvent(m_scene.id(),
                            selected.first()->data(Qt::UserRole).toString());
}

void SceneEditorWidget::apply()
{
    if (!m_document || m_scene.id().isEmpty())
        return;
    m_applying = true;
    if (!m_document->updateScene(m_scene))
        reload();
    m_applying = false;
}
