#include "ui/PreviewArea.h"

#include <QLabel>
#include <QVBoxLayout>

PreviewArea::PreviewArea(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("KonataPreviewArea"));
    setMinimumSize(300, 200);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *header = new QLabel(tr("PREVIEW"), this);
    header->setObjectName(QStringLiteral("PanelHeader"));
    layout->addWidget(header);

    auto *center = new QWidget(this);
    center->setObjectName(QStringLiteral("PreviewCenter"));
    auto *centerLayout = new QVBoxLayout(center);
    centerLayout->setAlignment(Qt::AlignCenter);

    auto *title = new QLabel(tr("KONATA Preview"), center);
    title->setObjectName(QStringLiteral("PreviewTitle"));
    title->setAlignment(Qt::AlignCenter);

    auto *subtitle = new QLabel(
        tr("Preview workspace — renderer arrives in a later milestone."), center);
    subtitle->setObjectName(QStringLiteral("PreviewSubtitle"));
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setWordWrap(true);

    centerLayout->addWidget(title);
    centerLayout->addWidget(subtitle);

    layout->addWidget(center, 1);
}
