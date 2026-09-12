#include "ui/TimelinePanel.h"

#include <QLabel>
#include <QVBoxLayout>

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

    auto *body = new QLabel(
        tr("Timeline region — functionality arrives in a later milestone."), this);
    body->setObjectName(QStringLiteral("TimelinePlaceholder"));
    body->setAlignment(Qt::AlignCenter);

    layout->addWidget(body, 1);
}
