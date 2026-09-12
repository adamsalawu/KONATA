#pragma once

#include <QWidget>

class QTreeWidget;

class ProjectPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ProjectPanel(QWidget *parent = nullptr);

private:
    QTreeWidget *m_tree = nullptr;
};
