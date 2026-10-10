#include "ui/ProjectPanel.h"

#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "document/ProjectDocument.h"

namespace {
constexpr int kKindRole = Qt::UserRole;
constexpr int kIdRole = Qt::UserRole + 1;
} // namespace

ProjectPanel::ProjectPanel(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("KonataProjectPanel"));
    setMinimumWidth(180);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *header = new QLabel(tr("PROJECT"), this);
    header->setObjectName(QStringLiteral("PanelHeader"));
    layout->addWidget(header);

    m_tree = new QTreeWidget(this);
    m_tree->setObjectName(QStringLiteral("ProjectTree"));
    m_tree->setHeaderHidden(true);
    m_tree->setEditTriggers(QTreeWidget::NoEditTriggers);
    m_tree->setSelectionMode(QTreeWidget::SingleSelection);
    m_tree->setUniformRowHeights(true);
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(m_tree, 1);

    connect(m_tree, &QTreeWidget::itemSelectionChanged, this,
            &ProjectPanel::onItemSelectionChanged);
    connect(m_tree, &QWidget::customContextMenuRequested, this,
            &ProjectPanel::showContextMenu);
}

void ProjectPanel::setDocument(ProjectDocument *document)
{
    if (m_document) {
        disconnect(m_document, nullptr, this, nullptr);
    }
    m_document = document;
    if (m_document) {
        connect(m_document, &ProjectDocument::projectChanged, this, &ProjectPanel::refresh);
        connect(m_document, &ProjectDocument::selectionChanged, this,
                &ProjectPanel::syncSelection);
    }
    refresh();
}

void ProjectPanel::refresh()
{
    const QSignalBlocker blocker(m_tree);
    m_tree->clear();
    if (!m_document) {
        return;
    }

    const Konata::Core::Story &story = m_document->story();
    const QString rootLabel =
        story.title().isEmpty() ? tr("Project") : story.title();
    auto *root = new QTreeWidgetItem(m_tree, QStringList(rootLabel));
    root->setData(0, kKindRole, static_cast<int>(NodeKind::Root));
    root->setExpanded(true);

    auto *charGroup = new QTreeWidgetItem(root, QStringList(tr("Characters")));
    charGroup->setData(0, kKindRole, static_cast<int>(NodeKind::CharacterGroup));
    charGroup->setExpanded(true);
    for (const Konata::Core::Character &c : story.characters()) {
        const QString label =
            c.displayName.isEmpty() ? tr("(unnamed character)") : c.displayName;
        auto *item = new QTreeWidgetItem(charGroup, QStringList(label));
        item->setData(0, kKindRole, static_cast<int>(NodeKind::CharacterItem));
        item->setData(0, kIdRole, c.id);
    }

    auto *convGroup = new QTreeWidgetItem(root, QStringList(tr("Conversations")));
    convGroup->setData(0, kKindRole, static_cast<int>(NodeKind::ConversationGroup));
    convGroup->setExpanded(true);
    for (const Konata::Core::Conversation &c : story.conversations()) {
        const QString label =
            c.title().isEmpty() ? tr("(untitled conversation)") : c.title();
        auto *item = new QTreeWidgetItem(convGroup, QStringList(label));
        item->setData(0, kKindRole, static_cast<int>(NodeKind::ConversationItem));
        item->setData(0, kIdRole, c.id());
    }

    auto *sceneGroup = new QTreeWidgetItem(root, QStringList(tr("Scenes")));
    sceneGroup->setData(0, kKindRole, static_cast<int>(NodeKind::SceneGroup));
    sceneGroup->setExpanded(true);
    for (const Konata::Core::Scene &s : story.scenes()) {
        const QString label = s.name().isEmpty() ? tr("(unnamed scene)") : s.name();
        auto *item = new QTreeWidgetItem(sceneGroup, QStringList(label));
        item->setData(0, kKindRole, static_cast<int>(NodeKind::SceneItem));
        item->setData(0, kIdRole, s.id());
    }

    syncSelection();
}

void ProjectPanel::onItemSelectionChanged()
{
    if (!m_document)
        return;
    const QList<QTreeWidgetItem *> selected = m_tree->selectedItems();
    if (selected.isEmpty()) {
        m_document->clearSelection();
        return;
    }
    const QTreeWidgetItem *item = selected.first();
    switch (kindOf(item)) {
    case NodeKind::CharacterItem:
        m_document->setSelection(ProjectDocument::SelectionType::Character, idOf(item));
        break;
    case NodeKind::ConversationItem:
        m_document->setSelection(ProjectDocument::SelectionType::Conversation, idOf(item));
        break;
    case NodeKind::SceneItem:
        m_document->setSelection(ProjectDocument::SelectionType::Scene, idOf(item));
        break;
    default:
        m_document->clearSelection();
        break;
    }
}

void ProjectPanel::syncSelection()
{
    if (!m_document)
        return;
    const QSignalBlocker blocker(m_tree);
    const ProjectDocument::Selection selection = m_document->selection();
    NodeKind kind = NodeKind::Root;
    switch (selection.type) {
    case ProjectDocument::SelectionType::Character:
        kind = NodeKind::CharacterItem;
        break;
    case ProjectDocument::SelectionType::Conversation:
        kind = NodeKind::ConversationItem;
        break;
    case ProjectDocument::SelectionType::Scene:
        kind = NodeKind::SceneItem;
        break;
    case ProjectDocument::SelectionType::None:
        m_tree->clearSelection();
        return;
    }
    if (QTreeWidgetItem *item = findItem(kind, selection.id))
        m_tree->setCurrentItem(item);
    else
        m_tree->clearSelection(); // selected object is gone
}

void ProjectPanel::showContextMenu(const QPoint &pos)
{
    if (!m_document)
        return;
    QTreeWidgetItem *item = m_tree->itemAt(pos);
    const NodeKind kind = item ? kindOf(item) : NodeKind::Root;

    QMenu menu(this);
    QAction *newCharacterAction = menu.addAction(tr("New Character"));
    QAction *newConversationAction = menu.addAction(tr("New Conversation"));
    QAction *newSceneAction = menu.addAction(tr("New Scene"));

    QAction *deleteAction = nullptr;
    if (kind == NodeKind::CharacterItem || kind == NodeKind::ConversationItem
        || kind == NodeKind::SceneItem) {
        menu.addSeparator();
        deleteAction = menu.addAction(tr("Delete \"%1\"").arg(item->text(0)));
    }

    QAction *chosen = menu.exec(m_tree->viewport()->mapToGlobal(pos));
    if (!chosen)
        return;
    if (chosen == newCharacterAction)
        createCharacter();
    else if (chosen == newConversationAction)
        createConversation();
    else if (chosen == newSceneAction)
        createScene();
    else if (chosen == deleteAction)
        deleteSelectedItem();
}

ProjectPanel::NodeKind ProjectPanel::kindOf(const QTreeWidgetItem *item)
{
    return static_cast<NodeKind>(item->data(0, kKindRole).toInt());
}

QString ProjectPanel::idOf(const QTreeWidgetItem *item)
{
    return item->data(0, kIdRole).toString();
}

QTreeWidgetItem *ProjectPanel::findItem(NodeKind kind, const QString &id) const
{
    for (QTreeWidgetItemIterator it(m_tree); *it; ++it) {
        if (kindOf(*it) == kind && idOf(*it) == id)
            return *it;
    }
    return nullptr;
}

void ProjectPanel::createCharacter()
{
    const Konata::Core::Character created = m_document->addCharacter();
    m_document->setSelection(ProjectDocument::SelectionType::Character, created.id);
}

void ProjectPanel::createConversation()
{
    const Konata::Core::Conversation created = m_document->addConversation();
    m_document->setSelection(ProjectDocument::SelectionType::Conversation, created.id());
}

void ProjectPanel::createScene()
{
    const Konata::Core::Scene created = m_document->addScene();
    m_document->setSelection(ProjectDocument::SelectionType::Scene, created.id());
}

void ProjectPanel::deleteSelectedItem()
{
    const ProjectDocument::Selection selection = m_document->selection();
    if (selection.type == ProjectDocument::SelectionType::None || selection.id.isEmpty())
        return;

    QString label;
    switch (selection.type) {
    case ProjectDocument::SelectionType::Character: {
        const Konata::Core::Character *c = m_document->story().findCharacter(selection.id);
        label = c && !c->displayName.isEmpty() ? c->displayName : tr("this character");
        break;
    }
    case ProjectDocument::SelectionType::Conversation: {
        const Konata::Core::Conversation *c =
            m_document->story().findConversation(selection.id);
        label = c && !c->title().isEmpty() ? c->title() : tr("this conversation");
        break;
    }
    case ProjectDocument::SelectionType::Scene: {
        const Konata::Core::Scene *s = m_document->story().findScene(selection.id);
        label = s && !s->name().isEmpty() ? s->name() : tr("this scene");
        break;
    }
    case ProjectDocument::SelectionType::None:
        return;
    }

    if (QMessageBox::question(this, tr("Delete"), tr("Delete %1?").arg(label))
        != QMessageBox::Yes)
        return;

    switch (selection.type) {
    case ProjectDocument::SelectionType::Character:
        m_document->removeCharacter(selection.id);
        break;
    case ProjectDocument::SelectionType::Conversation:
        m_document->removeConversation(selection.id);
        break;
    case ProjectDocument::SelectionType::Scene:
        m_document->removeScene(selection.id);
        break;
    case ProjectDocument::SelectionType::None:
        break;
    }
}
