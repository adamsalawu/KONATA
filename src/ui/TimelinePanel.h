#pragma once

#include <QWidget>

class QLabel;
class ProjectDocument;

class TimelinePanel : public QWidget
{
    Q_OBJECT

public:
    explicit TimelinePanel(QWidget *parent = nullptr);

    void setDocument(ProjectDocument *document);

private slots:
    void refresh();

private:
    ProjectDocument *m_document = nullptr;
    QLabel *m_contextLabel = nullptr;
};
