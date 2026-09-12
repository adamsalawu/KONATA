#include "ui/ProjectPanel.h"

#include <QLabel>
#include <QTreeWidget>
#include <QVBoxLayout>

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

    auto *story = new QTreeWidgetItem(m_tree, QStringList(tr("Story")));
    new QTreeWidgetItem(story, QStringList(tr("Characters")));
    new QTreeWidgetItem(story, QStringList(tr("Conversations")));
    new QTreeWidgetItem(story, QStringList(tr("Scenes")));

    m_tree->expandAll();
    m_tree->setCurrentItem(story);

    layout->addWidget(m_tree, 1);
}
