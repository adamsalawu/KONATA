#include "ui/PreviewArea.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSpinBox>
#include <QVBoxLayout>

#include "core/CanvasSettings.h"
#include "document/ProjectDocument.h"
#include "rendering/CanvasLayout.h"
#include "rendering/KonataRenderer.h"
#include "rendering/skins/AppleGeometry.h"
#include "rendering/skins/AppleIcons.h"
#include "rendering/skins/AppleTypography.h"
#include "ui/RoundedBubble.h"

namespace {
constexpr int kAnchorScale = 100;

bool isLocalImageRef(const QString &ref)
{
    if (ref.isEmpty() || ref.contains(QStringLiteral("://")))
        return false;
    const QString lower = ref.toLower();
    return !lower.startsWith(QStringLiteral("http:"))
        && !lower.startsWith(QStringLiteral("https:"))
        && !lower.startsWith(QStringLiteral("ftp:"))
        && !lower.startsWith(QStringLiteral("data:"));
}

bool loadScaledPixmap(const QString &ref, const QSize &bounds, QPixmap &out)
{
    if (!isLocalImageRef(ref))
        return false;
    const QPixmap source(ref);
    if (source.isNull())
        return false;
    out = source.scaled(bounds, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    return !out.isNull();
}

QPixmap circularPixmap(const QPixmap &source, const QSize &size)
{
    QPixmap scaled = source.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    QPixmap out(size);
    out.fill(Qt::transparent);
    QPainter p(&out);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setClipRegion(QRegion(QRect(QPoint(0,0), size), QRegion::Ellipse));
    p.drawPixmap(0,0, scaled);
    return out;
}

void setCircularAvatar(QLabel* label, const QString& ref, const QSize& size, const QString& fallbackInitial)
{
    QPixmap pm;
    if (!ref.isEmpty() && loadScaledPixmap(ref, size, pm)) {
        label->setPixmap(circularPixmap(pm, size));
    } else {
        // Fallback: initial in circle
        QPixmap out(size);
        out.fill(Qt::transparent);
        QPainter p(&out);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setBrush(QColor(QStringLiteral("#3A3A3C")));
        p.setPen(Qt::NoPen);
        p.drawEllipse(QRect(QPoint(0,0), size));
        if (!fallbackInitial.isEmpty()) {
            p.setPen(QColor(QStringLiteral("#FFFFFF")));
            QFont f = AppleTypography::font(14, QFont::DemiBold);
            p.setFont(f);
            p.drawText(QRect(QPoint(0,0), size), Qt::AlignCenter, fallbackInitial);
        }
        label->setPixmap(out);
    }
}

} // namespace

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

    // Canvas bar: aspect / custom size (presentation, not Core)
    m_canvasBar = new QWidget(this);
    m_canvasBar->setObjectName(QStringLiteral("CanvasBar"));
    auto *canvasLayout = new QHBoxLayout(m_canvasBar);
    canvasLayout->setContentsMargins(12, 6, 12, 6);
    canvasLayout->setSpacing(8);
    auto *aspectLabel = new QLabel(tr("Canvas:"), m_canvasBar);
    aspectLabel->setObjectName(QStringLiteral("CanvasLabel"));
    m_aspectCombo = new QComboBox(m_canvasBar);
    m_aspectCombo->setObjectName(QStringLiteral("CanvasAspectCombo"));
    m_aspectCombo->addItems(QStringList{QStringLiteral("1:1"), QStringLiteral("4:5"), QStringLiteral("9:16"), QStringLiteral("16:9"), QStringLiteral("Custom")});
    m_widthSpin = new QSpinBox(m_canvasBar);
    m_widthSpin->setObjectName(QStringLiteral("CanvasWidthSpin"));
    m_widthSpin->setRange(1, 8192);
    m_widthSpin->setSuffix(QStringLiteral(" px"));
    m_heightSpin = new QSpinBox(m_canvasBar);
    m_heightSpin->setObjectName(QStringLiteral("CanvasHeightSpin"));
    m_heightSpin->setRange(1, 8192);
    m_heightSpin->setSuffix(QStringLiteral(" px"));
    auto *xLabel = new QLabel(QStringLiteral("×"), m_canvasBar);
    xLabel->setObjectName(QStringLiteral("CanvasXLabel"));
    canvasLayout->addWidget(aspectLabel);
    canvasLayout->addWidget(m_aspectCombo);
    canvasLayout->addWidget(m_widthSpin);
    canvasLayout->addWidget(xLabel);
    canvasLayout->addWidget(m_heightSpin);
    canvasLayout->addStretch(1);
    layout->addWidget(m_canvasBar);
    connect(m_aspectCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PreviewArea::onAspectChanged);
    connect(m_widthSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &PreviewArea::onCustomSizeChanged);
    connect(m_heightSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &PreviewArea::onCustomSizeChanged);

    // Title bar: centered header for private (single avatar) vs group (overlapping stack).
    auto *titleBar = new QWidget(this);
    titleBar->setObjectName(QStringLiteral("RenderTitleBar"));
    auto *titleRow = new QHBoxLayout(titleBar);
    titleRow->setContentsMargins(12, 8, 12, 6);
    titleRow->setSpacing(8);
    m_backLabel = new QLabel(titleBar);
    m_backLabel->setObjectName(QStringLiteral("RenderBack"));
    m_backLabel->setAlignment(Qt::AlignCenter);
    m_backLabel->setFixedSize(32, 32);
    // Left headerAvatar kept for test compatibility but hidden in new centered layout;
    // the visible avatar is m_centerAvatar inside the centered column.
    m_headerAvatar = new QLabel(titleBar);
    m_headerAvatar->setObjectName(QStringLiteral("RenderHeaderAvatar"));
    m_headerAvatar->setAlignment(Qt::AlignCenter);
    m_headerAvatar->setVisible(false);
    auto *titleColumn = new QWidget(titleBar);
    m_titleColumn = titleColumn;
    auto *titleLayout = new QVBoxLayout(titleColumn);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(2);
    titleLayout->setAlignment(Qt::AlignHCenter);
    m_centerAvatar = new QLabel(titleColumn);
    m_centerAvatar->setObjectName(QStringLiteral("RenderCenterAvatar"));
    m_centerAvatar->setAlignment(Qt::AlignCenter);
    m_groupStack = new QWidget(titleColumn);
    m_groupStack->setObjectName(QStringLiteral("RenderGroupAvatars"));
    auto *groupLayout = new QHBoxLayout(m_groupStack);
    groupLayout->setContentsMargins(0,0,0,0);
    groupLayout->setSpacing(-AppleGeometry::groupHeaderOverlap);
    groupLayout->setAlignment(Qt::AlignHCenter);
    m_titleLabel = new QLabel(titleColumn);
    m_titleLabel->setObjectName(QStringLiteral("RenderConvTitle"));
    m_titleLabel->setAlignment(Qt::AlignHCenter);
    m_titleChevron = new QLabel(QStringLiteral("›"), titleColumn);
    m_titleChevron->setObjectName(QStringLiteral("RenderTitleChevron"));
    m_titleChevron->setAlignment(Qt::AlignVCenter);
    m_titleChevron->setStyleSheet(QStringLiteral("color:#0A84FF; font-size:16px;"));
    auto *titleHBox = new QWidget(titleColumn);
    auto *titleHBoxLayout = new QHBoxLayout(titleHBox);
    titleHBoxLayout->setContentsMargins(0,0,0,0);
    titleHBoxLayout->setSpacing(2);
    titleHBoxLayout->setAlignment(Qt::AlignHCenter);
    titleHBoxLayout->addWidget(m_titleLabel, 0, Qt::AlignVCenter);
    titleHBoxLayout->addWidget(m_titleChevron, 0, Qt::AlignVCenter);
    m_subLabel = new QLabel(titleColumn);
    m_subLabel->setObjectName(QStringLiteral("RenderParticipants"));
    m_subLabel->setWordWrap(true);
    m_subLabel->setAlignment(Qt::AlignHCenter);
    m_screenBadge = new QLabel(titleColumn);
    m_screenBadge->setObjectName(QStringLiteral("RenderScreenBadge"));
    m_screenBadge->setWordWrap(true);
    m_screenBadge->setAlignment(Qt::AlignHCenter);
    titleLayout->addWidget(m_centerAvatar, 0, Qt::AlignHCenter);
    titleLayout->addWidget(m_groupStack, 0, Qt::AlignHCenter);
    titleLayout->addWidget(titleHBox);
    titleLayout->addWidget(m_subLabel);
    titleLayout->addWidget(m_screenBadge);
    titleRow->addWidget(m_backLabel);
    titleRow->addStretch(1);
    titleRow->addWidget(titleColumn, 0, Qt::AlignHCenter);
    titleRow->addStretch(1);
    m_infoLabel = new QLabel(titleBar);
    m_infoLabel->setObjectName(QStringLiteral("RenderInfoAction"));
    m_infoLabel->setAlignment(Qt::AlignCenter);
    m_infoLabel->setFixedSize(32, 32);
    titleRow->addWidget(m_infoLabel);
    layout->addWidget(titleBar);
    m_titleBar = titleBar;

    m_scroll = new QScrollArea(this);
    m_scroll->setObjectName(QStringLiteral("RenderScroll"));
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_rowsContainer = new QWidget(m_scroll);
    m_rowsContainer->setObjectName(QStringLiteral("RenderRows"));
    m_rowsLayout = new QVBoxLayout(m_rowsContainer);
    m_rowsLayout->setContentsMargins(12, 8, 12, 12);
    m_rowsLayout->setSpacing(8);
    m_rowsLayout->setAlignment(Qt::AlignTop);
    m_emptyLabel = new QLabel(m_rowsContainer);
    m_emptyLabel->setObjectName(QStringLiteral("RenderEmpty"));
    m_emptyLabel->setWordWrap(true);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_scroll->setWidget(m_rowsContainer);
    layout->addWidget(m_scroll, 1);

    // Composer: iMessage style. Plus circle, field, mic/camera.
    m_composer = new QWidget(this);
    m_composer->setObjectName(QStringLiteral("RenderComposer"));
    auto *composerRow = new QHBoxLayout(m_composer);
    composerRow->setContentsMargins(12, 6, 12, 10);
    composerRow->setSpacing(8);
    auto *attachButton = new QPushButton(m_composer);
    attachButton->setObjectName(QStringLiteral("RenderComposerAttach"));
    attachButton->setFixedSize(32, 32);
    attachButton->setEnabled(false);
    auto *cameraButton = new QPushButton(m_composer);
    cameraButton->setObjectName(QStringLiteral("RenderComposerCamera"));
    cameraButton->setFixedSize(32, 32);
    cameraButton->setEnabled(false);
    m_composerField = new QLineEdit(m_composer);
    m_composerField->setObjectName(QStringLiteral("RenderComposerField"));
    m_composerField->setEnabled(false);
    auto *sendButton = new QPushButton(m_composer);
    sendButton->setObjectName(QStringLiteral("RenderComposerSend"));
    sendButton->setFixedSize(32, 32);
    sendButton->setEnabled(false);
    composerRow->addWidget(attachButton);
    composerRow->addWidget(cameraButton);
    composerRow->addWidget(m_composerField, 1);
    composerRow->addWidget(sendButton);
    layout->addWidget(m_composer);
    m_attachButton = attachButton;
    m_cameraButton = cameraButton;
    m_sendButton = sendButton;

    m_addBar = new QWidget(this);
    m_addBar->setObjectName(QStringLiteral("ChatAddBar"));
    auto *addRow = new QHBoxLayout(m_addBar);
    addRow->setContentsMargins(12, 6, 12, 8);
    addRow->setSpacing(8);
    m_addMessageButton = new QPushButton(tr("+ Add Message"), m_addBar);
    m_addMessageButton->setObjectName(QStringLiteral("ChatAddButton"));
    m_addTypeCombo = new QComboBox(m_addBar);
    m_addTypeCombo->setObjectName(QStringLiteral("ChatAddType"));
    m_addTypeCombo->addItem(tr("Text"), static_cast<int>(Konata::Core::MessageType::Text));
    m_addTypeCombo->addItem(tr("Image"), static_cast<int>(Konata::Core::MessageType::Image));
    m_addTypeCombo->addItem(tr("File"), static_cast<int>(Konata::Core::MessageType::File));
    m_addTypeCombo->addItem(tr("System"), static_cast<int>(Konata::Core::MessageType::System));
    addRow->addWidget(m_addMessageButton);
    addRow->addWidget(m_addTypeCombo);
    addRow->addStretch(1);
    layout->addWidget(m_addBar);
    connect(m_addMessageButton, &QPushButton::clicked, this, &PreviewArea::onAddMessage);

    m_structCard = new QWidget(this);
    m_structCard->setObjectName(QStringLiteral("PreviewCenter"));
    auto *structLayout = new QVBoxLayout(m_structCard);
    structLayout->setAlignment(Qt::AlignCenter);
    m_structLabel = new QLabel(m_structCard);
    m_structLabel->setObjectName(QStringLiteral("PreviewSubtitle"));
    m_structLabel->setAlignment(Qt::AlignCenter);
    m_structLabel->setWordWrap(true);
    structLayout->addWidget(m_structLabel);
    layout->addWidget(m_structCard, 1);
}

void PreviewArea::setDocument(ProjectDocument *document)
{
    if (m_document)
        disconnect(m_document, nullptr, this, nullptr);
    m_document = document;
    if (m_document) {
        connect(m_document, &ProjectDocument::projectChanged, this, &PreviewArea::refresh);
        connect(m_document, &ProjectDocument::selectionChanged, this, &PreviewArea::refresh);
        connect(m_document, &ProjectDocument::canvasChanged, this, &PreviewArea::onCanvasChangedFromDocument);
        connect(m_document, &ProjectDocument::projectChanged, this, &PreviewArea::updateCanvasControls);
    }
    updateCanvasControls();
    refresh();
}

void PreviewArea::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    applyBubbleWidths();
}

bool PreviewArea::eventFilter(QObject *watched, QEvent *event)
{
    const QEvent::Type type = event->type();
    if (type != QEvent::MouseButtonPress && type != QEvent::MouseButtonDblClick
        && type != QEvent::ContextMenu)
        return QWidget::eventFilter(watched, event);
    if (!m_document || m_openConversationId.isEmpty())
        return QWidget::eventFilter(watched, event);
    QWidget *widget = qobject_cast<QWidget *>(watched);
    QWidget *frame = nullptr;
    while (widget && widget != m_rowsContainer) {
        const QString name = widget->objectName();
        if (name == QStringLiteral("RenderRow") || name == QStringLiteral("RenderRowSystem")) {
            frame = widget;
            break;
        }
        widget = widget->parentWidget();
    }
    if (!frame)
        return QWidget::eventFilter(watched, event);
    const QString messageId = frame->property("messageId").toString();
    if (messageId.isEmpty())
        return QWidget::eventFilter(watched, event);
    if (type == QEvent::ContextMenu) {
        auto *contextEvent = static_cast<QContextMenuEvent *>(event);
        showRowContextMenu(m_openConversationId, messageId, contextEvent->globalPos());
        return true;
    }
    const bool alreadySelected =
        m_document->selection().type == ProjectDocument::SelectionType::Message
        && m_document->selection().id == messageId
        && m_document->selection().parentId == m_openConversationId;
    if (!alreadySelected)
        m_document->setSelection(ProjectDocument::SelectionType::Message, messageId,
                                 m_openConversationId);
    if (type == QEvent::MouseButtonDblClick)
        emit messageEditRequested();
    return true;
}

QList<QAction *> PreviewArea::rowActions(const QString &conversationId,
                                         const QString &messageId)
{
    QList<QAction *> actions;
    if (!m_document || conversationId.isEmpty() || messageId.isEmpty())
        return actions;
    const Konata::Core::Conversation *conv =
        m_document->story().findConversation(conversationId);
    if (!conv || !conv->findMessage(messageId))
        return actions;
    int index = -1;
    const QList<Konata::Core::Message> &messages = conv->messages();
    for (int i = 0; i < messages.size(); ++i) {
        if (messages.at(i).id == messageId) {
            index = i;
            break;
        }
    }
    auto *editAction = new QAction(tr("Edit"), this);
    connect(editAction, &QAction::triggered, this, [this, conversationId, messageId]() {
        m_document->setSelection(ProjectDocument::SelectionType::Message, messageId,
                                 conversationId);
        emit messageEditRequested();
    });
    auto *duplicateAction = new QAction(tr("Duplicate"), this);
    connect(duplicateAction, &QAction::triggered, this, [this, conversationId, messageId]() {
        const Konata::Core::Message created =
            m_document->duplicateMessage(conversationId, messageId);
        if (!created.id.isEmpty())
            m_document->setSelection(ProjectDocument::SelectionType::Message, created.id,
                                     conversationId);
    });
    auto *deleteAction = new QAction(tr("Delete"), this);
    connect(deleteAction, &QAction::triggered, this, [this, conversationId, messageId]() {
        m_document->removeMessage(conversationId, messageId);
    });
    auto *moveUpAction = new QAction(tr("Move Up"), this);
    moveUpAction->setEnabled(index > 0);
    connect(moveUpAction, &QAction::triggered, this, [this, conversationId, messageId]() {
        m_document->moveMessage(conversationId, messageId, -1);
    });
    auto *moveDownAction = new QAction(tr("Move Down"), this);
    moveDownAction->setEnabled(index >= 0 && index + 1 < messages.size());
    connect(moveDownAction, &QAction::triggered, this, [this, conversationId, messageId]() {
        m_document->moveMessage(conversationId, messageId, 1);
    });
    auto *copyAction = new QAction(tr("Copy Text"), this);
    connect(copyAction, &QAction::triggered, this, [this, conversationId, messageId]() {
        const Konata::Core::Conversation *live =
            m_document->story().findConversation(conversationId);
        const Konata::Core::Message *message =
            live ? live->findMessage(messageId) : nullptr;
        if (message)
            QApplication::clipboard()->setText(message->text);
    });
    actions << editAction << duplicateAction << deleteAction << moveUpAction << moveDownAction
            << copyAction;
    return actions;
}

void PreviewArea::onAddMessage()
{
    if (!m_document || m_openConversationId.isEmpty())
        return;
    const auto type = static_cast<Konata::Core::MessageType>(
        m_addTypeCombo->currentData().toInt());
    const Konata::Core::Message created = m_document->createMessage(m_openConversationId,
                                                                    QString(), type);
    if (!created.id.isEmpty())
        m_document->setSelection(ProjectDocument::SelectionType::Message, created.id,
                                 m_openConversationId);
}

void PreviewArea::refresh()
{
    if (!m_document) {
        showNeutral(tr("No project."));
        return;
    }
    const ProjectDocument::Selection selection = m_document->selection();
    QString conversationId;
    switch (selection.type) {
    case ProjectDocument::SelectionType::Conversation:
        conversationId = selection.id;
        break;
    case ProjectDocument::SelectionType::Message:
        conversationId = selection.parentId;
        break;
    case ProjectDocument::SelectionType::Scene:
        showSceneCard(selection.id);
        return;
    case ProjectDocument::SelectionType::Character:
    case ProjectDocument::SelectionType::SceneEvent:
    case ProjectDocument::SelectionType::None:
        break;
    }
    if (conversationId.isEmpty())
        conversationId = m_openConversationId;
    if (!conversationId.isEmpty()
        && m_document->story().findConversation(conversationId)) {
        showConversation(conversationId);
        return;
    }
    m_openConversationId.clear();
    showNeutral(tr("Select a conversation in the Project panel to start editing."));
}

void PreviewArea::showConversation(const QString &conversationId)
{
    const Konata::Core::Story &story = m_document->story();
    m_currentView = KonataRenderer::renderAppView(
        story, m_document->project().appDefinition, conversationId);
    m_hasView = true;
    const RenderedConversation &rendered = m_currentView.conversation;
    if (!rendered.valid) {
        m_hasView = false;
        m_openConversationId.clear();
        showNeutral(tr("Conversation not found."));
        return;
    }
    m_openConversationId = conversationId;
    updateScreenBadge(m_currentView);
    // Background follows palette: light for neutral, but keep appDefinition colors
    const QString bg = m_document->project().appDefinition.chat.backgroundColor;
    m_scroll->setStyleSheet(QStringLiteral("background:%1;").arg(bg));
    m_rowsContainer->setStyleSheet(QStringLiteral("background:%1;").arg(bg));

    const Konata::Core::DesignTokens &tokens = m_currentView.tokens;
    // Header for private vs group
    if (rendered.isGroup) {
        // Group: overlapping avatars center, title is participant count or title
        m_titleLabel->setFont(AppleTypography::font(AppleTypography::titleSize(), QFont::DemiBold));
        QString title = rendered.title;
        if (title == QStringLiteral("(untitled conversation)") || title.isEmpty()) {
            title = tr("%1 People").arg(rendered.participantNames.size());
        }
        m_titleLabel->setText(title);
        m_titleChevron->setVisible(true); // ">" for group as separate chevron
        m_subLabel->clear();
        m_subLabel->setVisible(false);
        m_centerAvatar->setVisible(false);
        m_headerAvatar->setVisible(false);
        m_groupStack->setVisible(true);
        // Clear previous avatars
        QLayoutItem *child;
        while ((child = m_groupStack->layout()->takeAt(0)) != nullptr) {
            if (child->widget()) delete child->widget();
            delete child;
        }
        // Create overlapping group avatars
        for (int i=0; i<qMin(4, rendered.groupAvatarRefs.size()); ++i) {
            QLabel* av = new QLabel(m_groupStack);
            av->setFixedSize(AppleGeometry::groupHeaderAvatarSize, AppleGeometry::groupHeaderAvatarSize);
            av->setAlignment(Qt::AlignCenter);
            QString initial = rendered.participantNames.value(i).left(1).toUpper();
            if (initial.isEmpty()) initial = QStringLiteral("?");
            setCircularAvatar(av, rendered.groupAvatarRefs.at(i), av->size(), initial);
            // Overlap via negative margin already in layout spacing
            m_groupStack->layout()->addWidget(av);
        }
        m_infoLabel->setVisible(false);
    } else {
        m_groupStack->setVisible(false);
        m_centerAvatar->setVisible(true);
        m_centerAvatar->setFixedSize(AppleGeometry::headerAvatarSize, AppleGeometry::headerAvatarSize);
        m_centerAvatar->setAlignment(Qt::AlignCenter);
        if (!rendered.participantNames.isEmpty()) {
            QString avRef = rendered.groupAvatarRefs.value(0);
            QString initial = rendered.participantNames.first().left(1).toUpper();
            setCircularAvatar(m_centerAvatar, avRef, m_centerAvatar->size(), initial);
        } else {
            QPixmap out(m_centerAvatar->size());
            out.fill(Qt::transparent);
            QPainter p(&out);
            p.setRenderHint(QPainter::Antialiasing, true);
            p.setBrush(QColor(QStringLiteral("#3A3A3C")));
            p.setPen(Qt::NoPen);
            p.drawEllipse(QRect(QPoint(0,0), m_centerAvatar->size()));
            m_centerAvatar->setPixmap(out);
        }
        m_headerAvatar->setVisible(false);
        m_titleLabel->setFont(AppleTypography::font(tokens.fontSizeTitle, QFont::DemiBold));
        // Private: show conversation title as is (reference shows contact name, but tests expect title)
        m_titleLabel->setText(rendered.title);
        m_titleChevron->setVisible(true);
        m_subLabel->clear();
        m_subLabel->setVisible(false);
    }
    m_titleBar->setVisible(true);
    m_scroll->setVisible(true);
    m_addBar->setVisible(true);
    m_structCard->setVisible(false);
    updateHeaderExtras(m_currentView, conversationId);
    updateComposer(m_currentView);
    m_rowsLayout->setSpacing(tokens.spacingMedium);
    clearRows();
    if (rendered.empty) {
        m_emptyLabel->setText(
            tr("This conversation is empty — press + Add Message below to begin."));
        m_emptyLabel->setVisible(true);
    } else {
        m_emptyLabel->setVisible(false);
        for (int i = 0; i < rendered.rows.size(); ++i) {
            const RenderedMessage &row = rendered.rows.at(i);
            if (row.startsNewDay)
                m_rowsLayout->addWidget(buildDaySeparator(row.dayLabel));
            m_rowsLayout->addWidget(buildRow(row));
            if (i + 1 < rendered.rows.size()) {
                const bool continuesGroup =
                    row.group != Konata::Core::MessageGroup::Single
                    && row.group != Konata::Core::MessageGroup::Last;
                m_rowsLayout->addSpacing(continuesGroup
                                             ? AppleGeometry::messageSpacingSameAuthor
                                             : AppleGeometry::messageSpacingGroups);
            }
        }
    }
    syncRowSelection();
    applyBubbleWidths();
}

void PreviewArea::showSceneCard(const QString &sceneId)
{
    const Konata::Core::Scene *scene = m_document->story().findScene(sceneId);
    clearRows();
    updateScreenBadge(KonataRenderer::renderAppView(
        m_document->story(), m_document->project().appDefinition, QString()));
    m_hasView = false;
    m_scroll->setVisible(false);
    m_addBar->setVisible(false);
    m_structCard->setVisible(true);
    m_titleLabel->setText(scene && !scene->name().isEmpty() ? scene->name()
                                                            : tr("Scene"));
    m_subLabel->setText(tr("Scene — playback arrives in a later milestone."));
    const int count = scene ? scene->eventCount() : 0;
    const QString description =
        scene && !scene->description().isEmpty() ? scene->description() : QString();
    m_structLabel->setText(
        description.isEmpty()
            ? tr("%n structured event(s). No playback in 0.2A.", "", count)
            : tr("%1\n\n%2 structured event(s). No playback in 0.2A.")
                  .arg(description)
                  .arg(count));
}

void PreviewArea::showNeutral(const QString &hint)
{
    const Konata::Core::Story &story = m_document->story();
    int messageCount = 0;
    for (const Konata::Core::Conversation &c : story.conversations())
        messageCount += c.messageCount();
    clearRows();
    updateScreenBadge(KonataRenderer::renderAppView(
        m_document->story(), m_document->project().appDefinition, QString()));
    m_hasView = false;
    m_scroll->setVisible(false);
    m_addBar->setVisible(false);
    m_structCard->setVisible(true);
    m_titleLabel->setText(tr("KONATA Preview"));
    m_subLabel->setText(tr("Renderer: conversation view (neutral, 0.2A)."));
    m_structLabel->setText(tr("Project: %1\n%2 characters · %3 conversations · %4 messages\n%5")
                               .arg(story.title().isEmpty() ? tr("Untitled") : story.title())
                               .arg(story.characterCount())
                               .arg(story.conversationCount())
                               .arg(messageCount)
                               .arg(hint));
}

void PreviewArea::updateScreenBadge(const RenderedAppView &view)
{
    QString screen;
    switch (view.screen.type) {
    case Konata::Core::ScreenType::ConversationList:
        screen = tr("Conversation list");
        break;
    case Konata::Core::ScreenType::Chat:
        screen = tr("Chat");
        break;
    case Konata::Core::ScreenType::Profile:
        screen = tr("Profile");
        break;
    case Konata::Core::ScreenType::Custom:
        screen = view.screen.screenId.isEmpty() ? tr("Custom") : view.screen.screenId;
        break;
    }
    QStringList components;
    for (Konata::Core::ComponentType component : view.screen.components)
        components.append(componentLabel(component));
    m_screenBadge->setText(
        tr("%1 · %2 · %3").arg(screen, view.appName, components.join(QStringLiteral(", "))));
}

QString PreviewArea::componentLabel(Konata::Core::ComponentType component)
{
    using Konata::Core::ComponentType;
    switch (component) {
    case ComponentType::Avatar:
        return tr("Avatar");
    case ComponentType::Header:
        return tr("Header");
    case ComponentType::ConversationRow:
        return tr("Conversation row");
    case ComponentType::MessageBubble:
        return tr("Message bubble");
    case ComponentType::ImageMessage:
        return tr("Image message");
    case ComponentType::Composer:
        return tr("Composer");
    case ComponentType::Timestamp:
        return tr("Timestamp");
    case ComponentType::TypingIndicator:
        return tr("Typing indicator");
    case ComponentType::NavigationItem:
        return tr("Navigation item");
    case ComponentType::ProfileHeader:
        return tr("Profile header");
    case ComponentType::Custom:
        return tr("Custom");
    }
    return tr("Custom");
}

void PreviewArea::updateCanvasControls()
{
    if (!m_document || m_updatingCanvasControls)
        return;
    m_updatingCanvasControls = true;
    const auto canvas = m_document->canvas();
    const QString label = Konata::Core::CanvasSettings::presetLabel(canvas.preset);
    const int idx = m_aspectCombo->findText(label);
    if (idx >= 0) {
        QSignalBlocker b(m_aspectCombo);
        m_aspectCombo->setCurrentIndex(idx);
    }
    {
        QSignalBlocker b1(m_widthSpin);
        QSignalBlocker b2(m_heightSpin);
        m_widthSpin->setValue(canvas.customWidth);
        m_heightSpin->setValue(canvas.customHeight);
    }
    const bool isCustom = canvas.preset == Konata::Core::CanvasSettings::Preset::Custom;
    m_widthSpin->setEnabled(isCustom);
    m_heightSpin->setEnabled(isCustom);
    m_updatingCanvasControls = false;
    updateCanvasFrame();
}

void PreviewArea::updateCanvasFrame()
{
    if (!m_document)
        return;
    const QSize logical = m_document->canvas().logicalSize();
    const CanvasLayout layout = CanvasLayout::fromSize(logical);
    const int contentW = layout.contentRect.width();
    // Canvas-aware: header, message, composer all use content width, centered.
    // No stretching of individual elements, just layout within canvas.
    const QList<QWidget*> contentWidgets = {m_titleBar, m_scroll, m_composer, m_addBar, m_structCard};
    for (QWidget *w : contentWidgets) {
        if (!w) continue;
        w->setMaximumWidth(contentW);
        w->setMinimumWidth(qMin(contentW, 320));
        if (auto *parentLayout = qobject_cast<QVBoxLayout*>(this->layout())) {
            parentLayout->setAlignment(w, Qt::AlignHCenter);
        }
    }
    // Background follows canvas
    const QString bg = m_document->project().appDefinition.chat.backgroundColor;
    const QString bgColor = bg.isEmpty() ? QStringLiteral("#000000") : bg;
    this->setStyleSheet(QStringLiteral("background:%1;").arg(bgColor));
    m_scroll->setStyleSheet(QStringLiteral("background:%1;").arg(bgColor));
    m_rowsContainer->setStyleSheet(QStringLiteral("background:%1;").arg(bgColor));
    // Tooltip for verification
    this->setToolTip(tr("Canvas %1×%2 → content %3").arg(logical.width()).arg(logical.height()).arg(contentW));
    update();
}

void PreviewArea::onAspectChanged()
{
    if (!m_document || m_updatingCanvasControls)
        return;
    const QString label = m_aspectCombo->currentText();
    bool ok = false;
    const auto preset = Konata::Core::CanvasSettings::presetFromLabel(label, &ok);
    if (!ok) return;
    auto canvas = m_document->canvas();
    canvas.preset = preset;
    // When switching to a preset, keep custom dims as preset size for next Custom
    if (preset != Konata::Core::CanvasSettings::Preset::Custom) {
        const QSize presetSize = Konata::Core::CanvasSettings::presetSize(preset);
        canvas.customWidth = presetSize.width();
        canvas.customHeight = presetSize.height();
    }
    if (!m_document->setCanvas(canvas)) {
        updateCanvasControls();
    }
}

void PreviewArea::onCustomSizeChanged()
{
    if (!m_document || m_updatingCanvasControls)
        return;
    auto canvas = m_document->canvas();
    canvas.customWidth = m_widthSpin->value();
    canvas.customHeight = m_heightSpin->value();
    // Auto-switch to Custom if user edits dimensions while on preset
    if (canvas.preset != Konata::Core::CanvasSettings::Preset::Custom) {
        canvas.preset = Konata::Core::CanvasSettings::Preset::Custom;
        QSignalBlocker b(m_aspectCombo);
        const int idx = m_aspectCombo->findText(QStringLiteral("Custom"));
        if (idx >= 0) m_aspectCombo->setCurrentIndex(idx);
    }
    if (!canvas.isValid()) {
        // Clamp to valid range
        canvas.customWidth = qBound(1, canvas.customWidth, 8192);
        canvas.customHeight = qBound(1, canvas.customHeight, 8192);
    }
    m_document->setCanvas(canvas);
}

void PreviewArea::onCanvasChangedFromDocument()
{
    updateCanvasControls();
    updateCanvasFrame();
}

QWidget *PreviewArea::buildRow(const RenderedMessage &row)
{
    const bool isSystem = row.type == Konata::Core::MessageType::System;
    auto *frame = new QWidget(m_rowsContainer);
    frame->setObjectName(isSystem ? QStringLiteral("RenderRowSystem")
                                  : QStringLiteral("RenderRow"));
    frame->setProperty("messageId", row.messageId);
    frame->setCursor(Qt::PointingHandCursor);
    auto *frameLayout = new QHBoxLayout(frame);
    const int margin = m_hasView ? m_currentView.tokens.spacingMedium : 8;
    frameLayout->setContentsMargins(margin, margin / 2, margin, margin / 2);
    frameLayout->setSpacing(8);
    if (isSystem) {
        auto *centered = new QLabel(row.textEmpty ? tr("(empty message)") : row.text, frame);
        centered->setObjectName(QStringLiteral("RenderText"));
        centered->setWordWrap(true);
        centered->setAlignment(Qt::AlignCenter);
        if (m_hasView)
            centered->setFont(AppleTypography::font(m_currentView.tokens.fontSizeSmall));
        frameLayout->addWidget(centered, 1);
        installRowFilter(frame);
        return frame;
    }
    // For group chats, incoming messages show sender name above bubble and avatar left
    QWidget* contentCol = new QWidget(frame);
    QVBoxLayout* colLayout = new QVBoxLayout(contentCol);
    colLayout->setContentsMargins(0,0,0,0);
    colLayout->setSpacing(2);
    if (row.showSenderName) {
        QLabel* sender = new QLabel(row.senderName, contentCol);
        sender->setObjectName(QStringLiteral("RenderSender"));
        sender->setStyleSheet(QStringLiteral("color:#8E8E93; font-size:12px;"));
        sender->setFont(AppleTypography::font(AppleTypography::senderNameSize()));
        colLayout->addWidget(sender);
    }
    QWidget* bubbleRow = new QWidget(contentCol);
    QHBoxLayout* bubbleRowLayout = new QHBoxLayout(bubbleRow);
    bubbleRowLayout->setContentsMargins(0,0,0,0);
    bubbleRowLayout->setSpacing(8);
    if (row.outgoing) {
        bubbleRowLayout->addStretch(1);
    } else if (row.showAvatar) {
        // Avatar left for incoming group
        QLabel* av = new QLabel(bubbleRow);
        av->setObjectName(QStringLiteral("RenderAvatar"));
        av->setFixedSize(AppleGeometry::chatAvatarSize, AppleGeometry::chatAvatarSize);
        av->setAlignment(Qt::AlignCenter);
        QString initial = row.authorName.left(1).toUpper();
        if (initial.isEmpty()) initial = QStringLiteral("?");
        setCircularAvatar(av, row.avatarRef, av->size(), initial);
        bubbleRowLayout->addWidget(av, 0, Qt::AlignBottom);
    } else {
        // Private incoming: no avatar, but keep spacing if needed? No.
    }
    bubbleRowLayout->addWidget(buildBubble(row));
    if (!row.outgoing) bubbleRowLayout->addStretch(1);
    colLayout->addWidget(bubbleRow);
    frameLayout->addWidget(contentCol, 1);
    installRowFilter(frame);
    return frame;
}

void PreviewArea::installRowFilter(QWidget *frame)
{
    frame->installEventFilter(this);
    const QList<QWidget *> children = frame->findChildren<QWidget *>();
    for (QWidget *child : children)
        child->installEventFilter(this);
}

QWidget *PreviewArea::buildBubble(const RenderedMessage &row)
{
    auto *bubble = new RoundedBubble(m_rowsContainer);
    bubble->setObjectName(row.outgoing ? QStringLiteral("RenderBubbleOutgoing")
                                       : QStringLiteral("RenderBubble"));
    const char *groupName = "Single";
    switch (row.group) {
    case Konata::Core::MessageGroup::First:
        groupName = "First";
        break;
    case Konata::Core::MessageGroup::Middle:
        groupName = "Middle";
        break;
    case Konata::Core::MessageGroup::Last:
        groupName = "Last";
        break;
    case Konata::Core::MessageGroup::Single:
        break;
    }
    bubble->setProperty("groupPosition", QString::fromLatin1(groupName));
    bubble->setFillColor(QColor(row.bubbleColor.isEmpty() ? QStringLiteral("#2C2C2E")
                                                          : row.bubbleColor));
    bubble->setCorners(row.corners.topLeft, row.corners.topRight, row.corners.bottomLeft,
                       row.corners.bottomRight);
    bubble->setTail(row.hasTail, row.outgoing);
    auto *bubbleLayout = new QVBoxLayout(bubble);
    const int padding = m_hasView ? m_currentView.tokens.spacingMedium : 8;
    bubbleLayout->setContentsMargins(padding + 2, padding - 2, padding + 2, padding - 2);
    bubbleLayout->setSpacing(2);
    const bool showAuthor =
        !m_hasView || m_currentView.conversation.showAuthorNames;
    auto *author = new QLabel(row.authorName, bubble);
    author->setObjectName(QStringLiteral("RenderAuthor"));
    author->setVisible(showAuthor && !row.outgoing);
    if (m_hasView)
        author->setFont(AppleTypography::font(m_currentView.tokens.fontSizeSmall, QFont::DemiBold));
    bubbleLayout->addWidget(author);
    auto *text = new QLabel(row.textEmpty ? tr("(empty message)") : row.text, bubble);
    text->setObjectName(QStringLiteral("RenderText"));
    text->setWordWrap(true);
    text->setAttribute(Qt::WA_TranslucentBackground, true);
    text->setStyleSheet(QStringLiteral("background: transparent;"));
    text->setTextInteractionFlags(Qt::TextSelectableByMouse);
    if (m_hasView) {
        text->setFont(AppleTypography::font(m_currentView.tokens.fontSizeBody));
        QPalette palette = text->palette();
        palette.setColor(QPalette::WindowText,
                         QColor(row.bubbleTextColor.isEmpty() ? QStringLiteral("#FFFFFF")
                                                              : row.bubbleTextColor));
        text->setPalette(palette);
    }
    bubbleLayout->addWidget(text);
    if (!row.timeText.isEmpty()) {
        auto *time = new QLabel(row.timeText, bubble);
        time->setObjectName(QStringLiteral("RenderTimestamp"));
        time->setAlignment(Qt::AlignRight);
        if (m_hasView)
            time->setFont(AppleTypography::font(m_currentView.tokens.fontSizeSmall));
        bubbleLayout->addWidget(time);
    }
    if (row.type == Konata::Core::MessageType::Image)
        bubbleLayout->addWidget(buildImageBox(row, bubble));
    else if (row.isFallbackType)
        bubbleLayout->addWidget(buildFallbackNote(row, bubble));
    return bubble;
}

QWidget *PreviewArea::buildAvatar(const RenderedMessage &row)
{
    // Deprecated: avatar now built directly in buildRow for group; keep for compatibility
    auto *avatar = new QLabel();
    avatar->setObjectName(QStringLiteral("RenderAvatar"));
    const int avatarSize = m_hasView ? qMax(1, m_currentView.tokens.avatarSize) : 36;
    avatar->setFixedSize(avatarSize, avatarSize);
    avatar->setAlignment(Qt::AlignCenter);
    QPixmap pixmap;
    if (row.hasAvatar && loadScaledPixmap(row.avatarRef, avatar->size(), pixmap)) {
        avatar->setPixmap(circularPixmap(pixmap, avatar->size()));
    } else {
        QString initial = QStringLiteral("?");
        if (!row.authorName.isEmpty() && row.authorName != QStringLiteral("(unknown author)"))
            initial = row.authorName.left(1).toUpper();
        avatar->setText(initial);
    }
    return avatar;
}

QWidget *PreviewArea::buildImageBox(const RenderedMessage &row, QWidget *parent)
{
    auto *box = new QWidget(parent);
    box->setObjectName(QStringLiteral("RenderImageBox"));
    box->setFixedHeight(132);
    auto *vertical = new QVBoxLayout(box);
    vertical->setContentsMargins(4, 4, 4, 4);
    vertical->setSpacing(0);
    const int x = qRound(qBound(0.0, row.imageAnchorX, 1.0) * kAnchorScale);
    const int y = qRound(qBound(0.0, row.imageAnchorY, 1.0) * kAnchorScale);
    vertical->addStretch(qMax(0, y));
    auto *thumbRow = new QWidget(box);
    thumbRow->setObjectName(QStringLiteral("RenderImageRow"));
    auto *horizontal = new QHBoxLayout(thumbRow);
    horizontal->setContentsMargins(0, 0, 0, 0);
    horizontal->setSpacing(0);
    horizontal->addStretch(qMax(0, x));
    auto *thumb = new QLabel(thumbRow);
    thumb->setObjectName(QStringLiteral("RenderImageThumb"));
    thumb->setFixedSize(120, 72);
    thumb->setAlignment(Qt::AlignCenter);
    thumb->setWordWrap(true);
    QPixmap pixmap;
    if (!row.imageRef.isEmpty() && loadScaledPixmap(row.imageRef, thumb->size(), pixmap)) {
        thumb->setPixmap(pixmap);
    } else {
        thumb->setText(row.attachmentNote.isEmpty() ? tr("Image unavailable")
                                                    : row.attachmentNote);
    }
    horizontal->addWidget(thumb);
    horizontal->addStretch(qMax(0, kAnchorScale - x));
    vertical->addWidget(thumbRow);
    vertical->addStretch(qMax(0, kAnchorScale - y));
    return box;
}

QWidget *PreviewArea::buildFallbackNote(const RenderedMessage &row, QWidget *parent)
{
    auto *note = new QLabel(row.attachmentNote.isEmpty()
                                ? tr("File attachment — preview arrives later.")
                                : tr("File attachment: %1 — preview arrives later.")
                                      .arg(row.attachmentNote),
                            parent);
    note->setObjectName(QStringLiteral("RenderFallback"));
    note->setWordWrap(true);
    return note;
}

QWidget *PreviewArea::buildDaySeparator(const QString &label)
{
    auto *pill = new QLabel(label.isEmpty() ? tr("Undated") : label, m_rowsContainer);
    pill->setObjectName(QStringLiteral("RenderDaySeparator"));
    pill->setAlignment(Qt::AlignCenter);
    if (m_hasView)
        pill->setFont(AppleTypography::font(m_currentView.tokens.fontSizeSmall));
    pill->setStyleSheet(QStringLiteral("color:#8E8E93; font-size:12px;"));
    auto *holder = new QWidget(m_rowsContainer);
    holder->setObjectName(QStringLiteral("RenderDaySeparatorRow"));
    auto *holderLayout = new QHBoxLayout(holder);
    holderLayout->setContentsMargins(0, 4, 0, 4);
    holderLayout->addStretch(1);
    holderLayout->addWidget(pill);
    holderLayout->addStretch(1);
    return holder;
}

QWidget *PreviewArea::findRowFrame(const QString &messageId) const
{
    if (messageId.isEmpty())
        return nullptr;
    const QList<QWidget *> frames = m_rowsContainer->findChildren<QWidget *>();
    for (QWidget *frame : frames) {
        const QString name = frame->objectName();
        if ((name == QStringLiteral("RenderRow") || name == QStringLiteral("RenderRowSystem"))
            && frame->property("messageId").toString() == messageId)
            return frame;
    }
    return nullptr;
}

void PreviewArea::syncRowSelection()
{
    const QString accent = m_document
        ? m_document->project().appDefinition.theme.accentColor
        : QStringLiteral("#0A84FF");
    const ProjectDocument::Selection selection =
        m_document ? m_document->selection() : ProjectDocument::Selection();
    const bool matchMessage =
        selection.type == ProjectDocument::SelectionType::Message
        && selection.parentId == m_openConversationId;
    const QList<QWidget *> frames = m_rowsContainer->findChildren<QWidget *>();
    QWidget *selectedFrame = nullptr;
    for (QWidget *frame : frames) {
        const QString name = frame->objectName();
        if (name != QStringLiteral("RenderRow") && name != QStringLiteral("RenderRowSystem"))
            continue;
        const bool selected = matchMessage && frame->property("messageId") == selection.id;
        frame->setStyleSheet(selected
                                 ? QStringLiteral("border:1px solid %1; border-radius:6px;").arg(accent)
                                 : QString());
        if (selected)
            selectedFrame = frame;
    }
    scrollToSelection(selectedFrame);
}

void PreviewArea::scrollToSelection(QWidget *selectedFrame)
{
    if (selectedFrame)
        m_scroll->ensureWidgetVisible(selectedFrame, 50, 50);
}

void PreviewArea::showRowContextMenu(const QString &conversationId, const QString &messageId,
                                     const QPoint &globalPos)
{
    QMenu menu(this);
    const QList<QAction *> actions = rowActions(conversationId, messageId);
    for (QAction *action : actions)
        menu.addAction(action);
    menu.exec(globalPos);
    qDeleteAll(actions);
}

void PreviewArea::updateHeaderExtras(const RenderedAppView &view, const QString &conversationId)
{
    Q_UNUSED(view);
    const bool showHeader =
        view.screen.components.contains(Konata::Core::ComponentType::Header);
    // Back always shows for private, but group maybe also? Keep visible.
    m_backLabel->setVisible(showHeader);
    // Header avatar handled in showConversation main logic now (group stack vs single)
    // Info icon: show for private only
    const bool isGroup = view.conversation.isGroup;
    m_infoLabel->setVisible(showHeader && !isGroup);
    if (!showHeader)
        return;
    const QColor accent(view.appName.isEmpty() ? QStringLiteral("#0A84FF")
                                               : m_document->project().appDefinition.theme.accentColor);
    // Back chevron
    m_backLabel->setPixmap(AppleIcons::pixmap(AppleIcon::Back, 20, accent, Qt::transparent));
    // Video for private, info already hidden for group
    if (!isGroup) {
        m_infoLabel->setPixmap(AppleIcons::pixmap(AppleIcon::Video, 26, accent, Qt::transparent));
    } else {
        m_infoLabel->clear();
    }
}

void PreviewArea::updateComposer(const RenderedAppView &view)
{
    const bool showComposer =
        view.screen.components.contains(Konata::Core::ComponentType::Composer);
    m_composer->setVisible(showComposer);
    if (!showComposer)
        return;
    m_composerField->setPlaceholderText(
        view.composerPlaceholder.isEmpty() ? tr("iMessage") : view.composerPlaceholder);
    const QColor gray(QStringLiteral("#8E8E93"));
    const QColor accent(m_document->project().appDefinition.theme.accentColor);
    const QColor white(QStringLiteral("#FFFFFF"));
    // Reference: plus circle left, iMessage field, mic circle right. No camera in this composition.
    m_attachButton->setIcon(AppleIcons::icon(AppleIcon::Add, gray, Qt::transparent));
    m_attachButton->setIconSize(QSize(22, 22));
    m_cameraButton->setVisible(false);
    m_sendButton->setIcon(AppleIcons::icon(AppleIcon::Mic, gray, Qt::transparent));
    m_sendButton->setIconSize(QSize(22, 22));
    // Field styling: rounded, background follows app palette (dark vs light handled via tokens)
    const QString fieldBg = view.tokens.avatarSize ? QStringLiteral("#1C1C1E") : QStringLiteral("#FFFFFF");
    Q_UNUSED(accent);
    Q_UNUSED(white);
    m_composerField->setStyleSheet(QStringLiteral("background:%1; border:1px solid #3E3E42; border-radius:16px; padding:6px 12px;").arg(fieldBg));
}

void PreviewArea::applyBubbleWidths()
{
    if (!m_hasView || !m_scroll->isVisible() || !m_document)
        return;
    // Canvas-aware: bubble max is 70% of the canvas's message area, not viewport
    const CanvasLayout layout = CanvasLayout::fromSize(m_document->canvas().logicalSize());
    const int maxWidth = layout.bubbleMaxWidth;
    const QList<QWidget *> bubbles = m_rowsContainer->findChildren<QWidget *>();
    for (QWidget *bubble : bubbles) {
        const QString name = bubble->objectName();
        if (name == QStringLiteral("RenderBubble")
            || name == QStringLiteral("RenderBubbleOutgoing"))
            bubble->setMaximumWidth(maxWidth);
    }
}

void PreviewArea::clearRows()
{
    QLayoutItem *child = nullptr;
    while ((child = m_rowsLayout->takeAt(0)) != nullptr) {
        if (child->widget() && child->widget() != m_emptyLabel)
            delete child->widget();
        delete child;
    }
    m_rowsLayout->addWidget(m_emptyLabel);
    m_emptyLabel->setVisible(false);
}
