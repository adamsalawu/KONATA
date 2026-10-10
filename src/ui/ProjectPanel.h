#pragma once

#include <QWidget>

class QTreeWidget;
class QTreeWidgetItem;
class ProjectDocument;

// Shows the Story's real Characters/Conversations/Scenes. Items carry stable
// Core IDs (Qt::UserRole); display names are presentation only. Creating,
// deleting, and selecting objects flows through ProjectDocument.
class ProjectPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ProjectPanel(QWidget *parent = nullptr);

    void setDocument(ProjectDocument *document);

public slots:
    void refresh();

private slots:
    void onItemSelectionChanged();
    void syncSelection();
    void showContextMenu(const QPoint &pos);

private:
    enum class NodeKind {
        Root,
        CharacterGroup,
        ConversationGroup,
        SceneGroup,
        CharacterItem,
        ConversationItem,
        SceneItem
    };

    static NodeKind kindOf(const QTreeWidgetItem *item);
    static QString idOf(const QTreeWidgetItem *item);
    QTreeWidgetItem *findItem(NodeKind kind, const QString &id) const;
    void createCharacter();
    void createConversation();
    void createScene();
    void deleteSelectedItem();

    ProjectDocument *m_document = nullptr;
    QTreeWidget *m_tree = nullptr;
};
