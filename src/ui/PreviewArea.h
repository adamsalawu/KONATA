#pragma once

#include <QWidget>

#include "core/AppUi.h"
#include "rendering/RenderModel.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QSpinBox;
class QAction;
class QScrollArea;
class QVBoxLayout;
class ProjectDocument;

// Central chat editor. Shows the selected conversation as one continuous,
// directly editable message stream; the ProjectPanel is the authoring
// navigation. Rows carry stable Core message IDs, support click select,
// double-click edit, context actions (Edit/Duplicate/Delete/Move/Copy) and
// an Add Message bar — every mutation flows through ProjectDocument into
// real Core data. Presentation (header, bubbles, composer) still resolves
// through KonataRenderer + skin; scenes keep a structural card. Never
// mutates project data during rendering.
class PreviewArea : public QWidget
{
    Q_OBJECT

public:
    explicit PreviewArea(QWidget *parent = nullptr);

    void setDocument(ProjectDocument *document);

    // Context actions for one message (used by the row menu and by tests /
    // automation). Triggering an action performs the real document mutation.
    QList<QAction *> rowActions(const QString &conversationId, const QString &messageId);

signals:
    // Emitted on double-click and the Edit action: host should focus the
    // Inspector's editable field for the selected message.
    void messageEditRequested();

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void refresh();
    void onAddMessage();
    void onAspectChanged();
    void onCustomSizeChanged();
    void onCanvasChangedFromDocument();

private:
    void showConversation(const QString &conversationId);
    void showSceneCard(const QString &sceneId);
    void showNeutral(const QString &hint);
    QWidget *buildRow(const RenderedMessage &row);
    QWidget *buildBubble(const RenderedMessage &row);
    QWidget *buildAvatar(const RenderedMessage &row);
    QWidget *buildImageBox(const RenderedMessage &row, QWidget *parent);
    QWidget *buildFallbackNote(const RenderedMessage &row, QWidget *parent);
    QWidget *buildDaySeparator(const QString &label);
    void updateHeaderExtras(const RenderedAppView &view, const QString &conversationId);
    void updateComposer(const RenderedAppView &view);
    void applyBubbleWidths();
    void clearRows();
    void updateScreenBadge(const RenderedAppView &view);
    static QString componentLabel(Konata::Core::ComponentType component);
    // Canvas framing (output aspect, not Core data)
    void updateCanvasControls();
    void updateCanvasFrame();
    // Row lookup + selection visuals for the currently open conversation.
    QWidget *findRowFrame(const QString &messageId) const;
    void installRowFilter(QWidget *frame);
    void syncRowSelection();
    void scrollToSelection(QWidget *selectedFrame);
    void showRowContextMenu(const QString &conversationId, const QString &messageId,
                            const QPoint &globalPos);

    ProjectDocument *m_document = nullptr;
    RenderedAppView m_currentView;
    bool m_hasView = false;
    // Session view memory: the chat stays open while authoring characters or
    // other objects. View state only — never persisted, never a selection.
    QString m_openConversationId;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_subLabel = nullptr;
    QLabel *m_screenBadge = nullptr;
    QLabel *m_backLabel = nullptr;
    QLabel *m_headerAvatar = nullptr;
    QLabel *m_centerAvatar = nullptr;
    QLabel *m_titleChevron = nullptr;
    QLabel *m_infoLabel = nullptr; // info/video icon (vector pixmap, non-interactive)
    QWidget *m_titleBar = nullptr;
    QWidget *m_titleColumn = nullptr;
    QWidget *m_groupStack = nullptr;
    QScrollArea *m_scroll = nullptr;
    QWidget *m_rowsContainer = nullptr;
    QVBoxLayout *m_rowsLayout = nullptr;
    QLabel *m_emptyLabel = nullptr;
    QWidget *m_composer = nullptr;
    QLineEdit *m_composerField = nullptr;
    QPushButton *m_attachButton = nullptr;
    QPushButton *m_cameraButton = nullptr;
    QPushButton *m_sendButton = nullptr;
    QWidget *m_addBar = nullptr;
    QPushButton *m_addMessageButton = nullptr;
    QComboBox *m_addTypeCombo = nullptr;
    // Canvas (output aspect, presentation only)
    QWidget *m_canvasBar = nullptr;
    QComboBox *m_aspectCombo = nullptr;
    QSpinBox *m_widthSpin = nullptr;
    QSpinBox *m_heightSpin = nullptr;
    QWidget *m_canvasFrame = nullptr;
    bool m_updatingCanvasControls = false;
    QWidget *m_structCard = nullptr;
    QLabel *m_structLabel = nullptr;
};
