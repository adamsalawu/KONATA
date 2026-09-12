#include "ui/InspectorPanel.h"

#include <QLabel>
#include <QVBoxLayout>

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

    auto *body = new QWidget(this);
    body->setObjectName(QStringLiteral("InspectorBody"));
    auto *bodyLayout = new QVBoxLayout(body);
    bodyLayout->setAlignment(Qt::AlignCenter);

    auto *state = new QLabel(tr("No object selected"), body);
    state->setObjectName(QStringLiteral("InspectorEmpty"));
    state->setAlignment(Qt::AlignCenter);

    auto *hint = new QLabel(
        tr("Select a project item to inspect it.\nEditors arrive in a later milestone."),
        body);
    hint->setObjectName(QStringLiteral("InspectorHint"));
    hint->setAlignment(Qt::AlignCenter);
    hint->setWordWrap(true);

    bodyLayout->addWidget(state);
    bodyLayout->addWidget(hint);

    layout->addWidget(body, 1);
}
