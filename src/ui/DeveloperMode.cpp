#include "ui/DeveloperMode.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGraphicsScene>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QScrollArea>
#include <QShowEvent>
#include <QSpinBox>
#include <QSplitter>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include "ui/DeveloperCanvas.h"
#include "ui/UiElementGraphicsItem.h"

using namespace Konata::Core;

DeveloperMode::DeveloperMode(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet(
        "DeveloperMode { background: #2C2C2E; color: #FFFFFF; }"
        "QLabel { color: #CCCCCC; }"
        "QPushButton { background: #3A3A3C; color: #FFFFFF; border: 1px solid #555; border-radius: 4px; padding: 4px 8px; }"
        "QPushButton:hover { background: #48484A; }"
        "QPushButton:pressed { background: #0A84FF; }"
        "QToolButton { background: #3A3A3C; color: #FFFFFF; border: 1px solid #555; border-radius: 4px; padding: 4px 8px; }"
        "QToolButton:checked { background: #0A84FF; color: #FFFFFF; }"
        "QToolButton:hover { background: #48484A; }"
        "QComboBox { background: #3A3A3C; color: #FFFFFF; border: 1px solid #555; border-radius: 4px; padding: 4px 8px; }"
        "QComboBox QAbstractItemView { background: #3A3A3C; color: #FFFFFF; selection-background-color: #0A84FF; }"
        "QSpinBox, QDoubleSpinBox { background: #3A3A3C; color: #FFFFFF; border: 1px solid #555; border-radius: 4px; padding: 2px 4px; }"
        "QLineEdit { background: #3A3A3C; color: #FFFFFF; border: 1px solid #555; border-radius: 4px; padding: 2px 4px; }"
        "QCheckBox { color: #FFFFFF; }"
        "QListWidget { background: #1C1C1E; color: #FFFFFF; border: 1px solid #555; }"
        "QListWidget::item:selected { background: #0A84FF; }"
        "QScrollArea { background: #2C2C2E; border: none; }"
        "QWidget { background: #2C2C2E; }"
    );

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    buildToolBar();
    buildElementPalette();
    buildLayerPanel();
    buildInspector();

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);

    // Left panel: element palette + layer list
    auto *leftPanel = new QWidget;
    auto *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(4, 4, 4, 4);

    leftLayout->addWidget(new QLabel(QStringLiteral("ELEMENTS")));
    leftLayout->addLayout(m_paletteGrid);
    leftLayout->addSpacing(8);
    leftLayout->addWidget(new QLabel(QStringLiteral("LAYERS")));
    leftLayout->addWidget(m_layerList, 1);

    splitter->addWidget(leftPanel);

    // Center: canvas
    m_canvas = new DeveloperCanvas;
    splitter->addWidget(m_canvas);

    // Right panel: inspector
    auto *inspectorScroll = new QScrollArea;
    inspectorScroll->setWidgetResizable(true);
    inspectorScroll->setWidget(m_inspectorPage);
    inspectorScroll->setMinimumWidth(260);
    inspectorScroll->setMaximumWidth(320);
    splitter->addWidget(inspectorScroll);

    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 0);

    root->addWidget(splitter, 1);

    // --- Toolbar connections ---
    connect(m_canvasCombo, &QComboBox::currentTextChanged, this, &DeveloperMode::onCanvasPresetChanged);
    connect(m_customWidth, &QSpinBox::valueChanged, this, &DeveloperMode::onCanvasPresetChanged);
    connect(m_customHeight, &QSpinBox::valueChanged, this, &DeveloperMode::onCanvasPresetChanged);
    connect(m_gridBtn, &QToolButton::clicked, this, &DeveloperMode::onToggleGrid);
    connect(m_snapBtn, &QToolButton::clicked, this, &DeveloperMode::onToggleSnap);

    auto *zoomInBtn = findChild<QToolButton *>(QStringLiteral("zoomInBtn"));
    auto *zoomOutBtn = findChild<QToolButton *>(QStringLiteral("zoomOutBtn"));
    auto *zoomFitBtn = findChild<QToolButton *>(QStringLiteral("zoomFitBtn"));
    auto *zoom100Btn = findChild<QToolButton *>(QStringLiteral("zoom100Btn"));
    auto *exitBtn = findChild<QPushButton *>(QStringLiteral("exitBtn"));

    if (zoomInBtn) connect(zoomInBtn, &QToolButton::clicked, this, &DeveloperMode::onZoomChanged);
    if (zoomOutBtn) connect(zoomOutBtn, &QToolButton::clicked, this, &DeveloperMode::onZoomChanged);
    if (zoomFitBtn) connect(zoomFitBtn, &QToolButton::clicked, this, &DeveloperMode::onZoomChanged);
    if (zoom100Btn) connect(zoom100Btn, &QToolButton::clicked, this, &DeveloperMode::onZoomChanged);
    if (exitBtn) connect(exitBtn, &QPushButton::clicked, this, &DeveloperMode::exitDeveloperMode);

    // --- Canvas connections ---
    connect(m_canvas, &DeveloperCanvas::elementSelected, this, &DeveloperMode::onSelectedElementChanged);
    connect(m_canvas, &DeveloperCanvas::selectionCleared, this, &DeveloperMode::onSelectionCleared);
    connect(m_canvas, &DeveloperCanvas::elementMoved, this, &DeveloperMode::onElementMoved);
    connect(m_canvas, &DeveloperCanvas::elementResized, this, &DeveloperMode::onElementResized);
    connect(m_canvas, &DeveloperCanvas::designChanged, this, &DeveloperMode::designChanged);

    // --- Layer list connections ---
    connect(m_layerList, &QListWidget::currentRowChanged, this, &DeveloperMode::onLayerSelected);
    m_layerList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_layerList, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        QListWidgetItem *item = m_layerList->itemAt(pos);
        if (!item) return;
        QMenu menu;
        menu.addAction(QStringLiteral("Bring Forward"), this, &DeveloperMode::onBringForward);
        menu.addAction(QStringLiteral("Send Backward"), this, &DeveloperMode::onSendBackward);
        menu.addSeparator();
        menu.addAction(QStringLiteral("Bring to Front"), this, &DeveloperMode::onBringToFront);
        menu.addAction(QStringLiteral("Send to Back"), this, &DeveloperMode::onSendToBack);
        menu.addSeparator();
        menu.addAction(QStringLiteral("Duplicate"), this, &DeveloperMode::onDuplicateSelected);
        menu.addAction(QStringLiteral("Delete"), this, &DeveloperMode::onDeleteSelected);
        menu.exec(m_layerList->mapToGlobal(pos));
    });

    // --- Inspector connections ---
    connect(m_nameEdit, &QLineEdit::editingFinished, this, &DeveloperMode::onInspectorChanged);
    connect(m_xSpin, &QDoubleSpinBox::valueChanged, this, &DeveloperMode::onInspectorChanged);
    connect(m_ySpin, &QDoubleSpinBox::valueChanged, this, &DeveloperMode::onInspectorChanged);
    connect(m_wSpin, &QDoubleSpinBox::valueChanged, this, &DeveloperMode::onInspectorChanged);
    connect(m_hSpin, &QDoubleSpinBox::valueChanged, this, &DeveloperMode::onInspectorChanged);
    connect(m_zSpin, &QSpinBox::valueChanged, this, &DeveloperMode::onInspectorChanged);
    connect(m_opacitySpin, &QDoubleSpinBox::valueChanged, this, &DeveloperMode::onInspectorChanged);
    connect(m_visibleCheck, &QCheckBox::checkStateChanged, this, &DeveloperMode::onInspectorChanged);
    connect(m_lockedCheck, &QCheckBox::checkStateChanged, this, &DeveloperMode::onInspectorChanged);
    connect(m_fillEdit, &QLineEdit::editingFinished, this, &DeveloperMode::onInspectorChanged);
    connect(m_strokeEdit, &QLineEdit::editingFinished, this, &DeveloperMode::onInspectorChanged);
    connect(m_strokeWidthSpin, &QDoubleSpinBox::valueChanged, this, &DeveloperMode::onInspectorChanged);
    connect(m_cornerSpin, &QDoubleSpinBox::valueChanged, this, &DeveloperMode::onInspectorChanged);
    connect(m_textEdit, &QLineEdit::editingFinished, this, &DeveloperMode::onInspectorChanged);
    connect(m_fontEdit, &QLineEdit::editingFinished, this, &DeveloperMode::onInspectorChanged);
    connect(m_fontSizeSpin, &QSpinBox::valueChanged, this, &DeveloperMode::onInspectorChanged);
    connect(m_fontWeightSpin, &QSpinBox::valueChanged, this, &DeveloperMode::onInspectorChanged);
    connect(m_textColorEdit, &QLineEdit::editingFinished, this, &DeveloperMode::onInspectorChanged);
    connect(m_imageRefEdit, &QLineEdit::editingFinished, this, &DeveloperMode::onInspectorChanged);
    connect(m_bubbleColorEdit, &QLineEdit::editingFinished, this, &DeveloperMode::onInspectorChanged);
    connect(m_bubbleCornerSpin, &QDoubleSpinBox::valueChanged, this, &DeveloperMode::onInspectorChanged);
    connect(m_tailCheck, &QCheckBox::checkStateChanged, this, &DeveloperMode::onInspectorChanged);
    connect(m_outgoingCheck, &QCheckBox::checkStateChanged, this, &DeveloperMode::onInspectorChanged);
    connect(m_lineColorEdit, &QLineEdit::editingFinished, this, &DeveloperMode::onInspectorChanged);
    connect(m_lineWidthSpin, &QDoubleSpinBox::valueChanged, this, &DeveloperMode::onInspectorChanged);

    updateZoomLabel();
}

void DeveloperMode::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    QTimer::singleShot(0, this, [this]() {
        m_canvas->zoomFit();
        m_canvas->setFocus();
    });
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void DeveloperMode::setDesign(const UiDesign &design)
{
    m_canvas->setDesign(design);
    refreshLayerList();
}

UiDesign DeveloperMode::currentDesign() const
{
    return m_canvas->currentDesign();
}

void DeveloperMode::setCanvasSettings(const CanvasSettings &canvas)
{
    m_canvasSettings = canvas;

    const QString label = CanvasSettings::presetLabel(canvas.preset);
    const int idx = m_canvasCombo->findText(label);
    if (idx >= 0) {
        QSignalBlocker b(m_canvasCombo);
        m_canvasCombo->setCurrentIndex(idx);
    }

    {
        QSignalBlocker bw(m_customWidth);
        QSignalBlocker bh(m_customHeight);
        m_customWidth->setValue(canvas.customWidth);
        m_customHeight->setValue(canvas.customHeight);
    }

    const bool isCustom = (canvas.preset == CanvasSettings::Preset::Custom);
    m_customWidth->setEnabled(isCustom);
    m_customHeight->setEnabled(isCustom);

    m_canvas->setCanvasSize(canvas.logicalSize());
}

// ---------------------------------------------------------------------------
// Build UI sections
// ---------------------------------------------------------------------------

void DeveloperMode::buildToolBar()
{
    auto *toolbar = new QHBoxLayout;
    toolbar->setContentsMargins(8, 4, 8, 4);
    toolbar->setSpacing(6);

    auto *title = new QLabel(QStringLiteral("Developer Mode"));
    title->setStyleSheet(QStringLiteral("font-weight:bold; font-size:14px;"));
    toolbar->addWidget(title);

    toolbar->addSpacing(12);
    toolbar->addWidget(new QLabel(QStringLiteral("Canvas:")));

    m_canvasCombo = new QComboBox;
    m_canvasCombo->addItems({QStringLiteral("1:1"), QStringLiteral("4:5"),
                             QStringLiteral("9:16"), QStringLiteral("16:9"),
                             QStringLiteral("Custom")});
    m_canvasCombo->setCurrentText(QStringLiteral("9:16"));
    toolbar->addWidget(m_canvasCombo);

    m_customWidth = new QSpinBox;
    m_customWidth->setRange(1, 8192);
    m_customWidth->setValue(1080);
    m_customWidth->setEnabled(false);
    toolbar->addWidget(m_customWidth);

    toolbar->addWidget(new QLabel(QStringLiteral("\u00D7")));

    m_customHeight = new QSpinBox;
    m_customHeight->setRange(1, 8192);
    m_customHeight->setValue(1920);
    m_customHeight->setEnabled(false);
    toolbar->addWidget(m_customHeight);

    toolbar->addSpacing(12);

    m_gridBtn = new QToolButton;
    m_gridBtn->setText(QStringLiteral("Grid"));
    m_gridBtn->setCheckable(true);
    toolbar->addWidget(m_gridBtn);

    m_snapBtn = new QToolButton;
    m_snapBtn->setText(QStringLiteral("Snap"));
    m_snapBtn->setCheckable(true);
    m_snapBtn->setChecked(true);
    toolbar->addWidget(m_snapBtn);

    toolbar->addSpacing(12);

    m_zoomLabel = new QLabel(QStringLiteral("100%"));
    m_zoomLabel->setMinimumWidth(48);
    toolbar->addWidget(m_zoomLabel);

    auto addZoomBtn = [&](const QString &name, const QString &text) {
        auto *btn = new QToolButton;
        btn->setObjectName(name);
        btn->setText(text);
        toolbar->addWidget(btn);
        return btn;
    };

    addZoomBtn(QStringLiteral("zoomInBtn"), QStringLiteral("+"));
    addZoomBtn(QStringLiteral("zoomOutBtn"), QStringLiteral("-"));
    addZoomBtn(QStringLiteral("zoomFitBtn"), QStringLiteral("Fit"));
    addZoomBtn(QStringLiteral("zoom100Btn"), QStringLiteral("100%"));

    toolbar->addStretch();

    auto *exitBtn = new QPushButton(QStringLiteral("Exit Developer Mode"));
    exitBtn->setObjectName(QStringLiteral("exitBtn"));
    exitBtn->setStyleSheet(QStringLiteral("color:#FF453A; font-weight:bold;"));
    toolbar->addWidget(exitBtn);

    auto *topWidget = new QWidget;
    topWidget->setLayout(toolbar);
    if (auto *parentLayout = qobject_cast<QVBoxLayout *>(this->layout()))
        parentLayout->addWidget(topWidget);
}

void DeveloperMode::buildElementPalette()
{
    m_paletteGrid = new QGridLayout;
    m_paletteGrid->setSpacing(4);

    const QList<QPair<QString, UiElementType>> types = {
        {QStringLiteral("Rect"), UiElementType::Rectangle},
        {QStringLiteral("Rounded"), UiElementType::RoundedRect},
        {QStringLiteral("Circle"), UiElementType::Circle},
        {QStringLiteral("Line"), UiElementType::Line},
        {QStringLiteral("Image"), UiElementType::Image},
        {QStringLiteral("Text"), UiElementType::Text},
        {QStringLiteral("Bubble"), UiElementType::MessageBubble},
        {QStringLiteral("Avatar"), UiElementType::Avatar},
        {QStringLiteral("Name"), UiElementType::SenderName},
        {QStringLiteral("Time"), UiElementType::Timestamp},
        {QStringLiteral("Composer"), UiElementType::Composer},
        {QStringLiteral("Input"), UiElementType::InputField},
        {QStringLiteral("Button"), UiElementType::Button},
        {QStringLiteral("Icon"), UiElementType::Icon},
        {QStringLiteral("Divider"), UiElementType::Divider},
    };

    const int cols = 3;
    for (int i = 0; i < types.size(); ++i) {
        auto *btn = new QPushButton(types[i].first);
        btn->setToolTip(types[i].first);
        btn->setMinimumHeight(32);

        auto type = types[i].second;
        connect(btn, &QPushButton::clicked, this, [this, type]() {
            m_pendingAddType = type;
            onAddElement();
        });

        m_paletteGrid->addWidget(btn, i / cols, i % cols);
    }
}

void DeveloperMode::buildLayerPanel()
{
    m_layerList = new QListWidget;
    m_layerList->setAlternatingRowColors(true);
}

void DeveloperMode::buildInspector()
{
    m_inspectorPage = new QWidget;
    auto *form = new QFormLayout(m_inspectorPage);
    form->setContentsMargins(8, 8, 8, 8);
    form->setSpacing(4);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    auto mkDouble = [&](double min, double max, double step, double dec) {
        auto *s = new QDoubleSpinBox;
        s->setRange(min, max);
        s->setSingleStep(step);
        s->setDecimals(static_cast<int>(dec));
        return s;
    };

    m_nameEdit = new QLineEdit;
    form->addRow(QStringLiteral("Name"), m_nameEdit);

    m_xSpin = mkDouble(-10000, 10000, 1, 1);
    form->addRow(QStringLiteral("X"), m_xSpin);

    m_ySpin = mkDouble(-10000, 10000, 1, 1);
    form->addRow(QStringLiteral("Y"), m_ySpin);

    m_wSpin = mkDouble(1, 10000, 1, 1);
    form->addRow(QStringLiteral("Width"), m_wSpin);

    m_hSpin = mkDouble(1, 10000, 1, 1);
    form->addRow(QStringLiteral("Height"), m_hSpin);

    m_zSpin = new QSpinBox;
    m_zSpin->setRange(-10000, 10000);
    form->addRow(QStringLiteral("Z"), m_zSpin);

    m_opacitySpin = mkDouble(0, 1, 0.05, 2);
    form->addRow(QStringLiteral("Opacity"), m_opacitySpin);

    m_visibleCheck = new QCheckBox;
    m_visibleCheck->setChecked(true);
    form->addRow(QStringLiteral("Visible"), m_visibleCheck);

    m_lockedCheck = new QCheckBox;
    form->addRow(QStringLiteral("Locked"), m_lockedCheck);

    m_fillEdit = new QLineEdit;
    m_fillEdit->setPlaceholderText(QStringLiteral("#RRGGBB"));
    form->addRow(QStringLiteral("Fill"), m_fillEdit);

    m_strokeEdit = new QLineEdit;
    m_strokeEdit->setPlaceholderText(QStringLiteral("#RRGGBB"));
    form->addRow(QStringLiteral("Stroke"), m_strokeEdit);

    m_strokeWidthSpin = mkDouble(0, 100, 0.5, 1);
    form->addRow(QStringLiteral("Stroke W"), m_strokeWidthSpin);

    m_cornerSpin = mkDouble(0, 500, 1, 1);
    form->addRow(QStringLiteral("Corner"), m_cornerSpin);

    m_textEdit = new QLineEdit;
    form->addRow(QStringLiteral("Text"), m_textEdit);

    m_fontEdit = new QLineEdit;
    form->addRow(QStringLiteral("Font"), m_fontEdit);

    m_fontSizeSpin = new QSpinBox;
    m_fontSizeSpin->setRange(1, 200);
    form->addRow(QStringLiteral("Font Size"), m_fontSizeSpin);

    m_fontWeightSpin = new QSpinBox;
    m_fontWeightSpin->setRange(100, 900);
    m_fontWeightSpin->setSingleStep(100);
    form->addRow(QStringLiteral("Font Wt"), m_fontWeightSpin);

    m_textColorEdit = new QLineEdit;
    m_textColorEdit->setPlaceholderText(QStringLiteral("#RRGGBB"));
    form->addRow(QStringLiteral("Text Clr"), m_textColorEdit);

    m_imageRefEdit = new QLineEdit;
    form->addRow(QStringLiteral("Image Ref"), m_imageRefEdit);

    m_bubbleColorEdit = new QLineEdit;
    m_bubbleColorEdit->setPlaceholderText(QStringLiteral("#RRGGBB"));
    form->addRow(QStringLiteral("Bubble Clr"), m_bubbleColorEdit);

    m_bubbleCornerSpin = mkDouble(0, 200, 1, 1);
    form->addRow(QStringLiteral("Bubble R"), m_bubbleCornerSpin);

    m_tailCheck = new QCheckBox;
    form->addRow(QStringLiteral("Tail"), m_tailCheck);

    m_outgoingCheck = new QCheckBox;
    form->addRow(QStringLiteral("Outgoing"), m_outgoingCheck);

    m_lineColorEdit = new QLineEdit;
    m_lineColorEdit->setPlaceholderText(QStringLiteral("#RRGGBB"));
    form->addRow(QStringLiteral("Line Clr"), m_lineColorEdit);

    m_lineWidthSpin = mkDouble(0, 50, 0.5, 1);
    form->addRow(QStringLiteral("Line W"), m_lineWidthSpin);
}

// ---------------------------------------------------------------------------
// Element palette
// ---------------------------------------------------------------------------

void DeveloperMode::onAddElement()
{
    UiDesign design = m_canvas->currentDesign();
    const double offset = design.elements.size() * 20.0;
    UiElement el = UiElement::create(m_pendingAddType, 50.0 + offset, 50.0 + offset, 100, 100);
    el.zValue = design.nextZ;

    design.elements.append(el);
    design.nextZ = design.nextZ + 1;

    m_canvas->setDesign(design);
    m_canvas->selectElement(el.id);
    refreshLayerList();
    emit designChanged();
}

// ---------------------------------------------------------------------------
// Canvas preset
// ---------------------------------------------------------------------------

void DeveloperMode::onCanvasPresetChanged()
{
    const QString label = m_canvasCombo->currentText();
    bool ok = false;
    CanvasSettings cs;
    cs.preset = CanvasSettings::presetFromLabel(label, &ok);
    if (!ok) cs.preset = CanvasSettings::Preset::Custom;

    const bool isCustom = (cs.preset == CanvasSettings::Preset::Custom);
    m_customWidth->setEnabled(isCustom);
    m_customHeight->setEnabled(isCustom);

    cs.customWidth = m_customWidth->value();
    cs.customHeight = m_customHeight->value();

    m_canvasSettings = cs;
    m_canvas->setCanvasSize(cs.logicalSize());
    emit canvasSettingsChanged(cs);
}

// ---------------------------------------------------------------------------
// Layer panel
// ---------------------------------------------------------------------------

void DeveloperMode::refreshLayerList()
{
    m_layerList->blockSignals(true);
    m_layerList->clear();

    const UiDesign design = m_canvas->currentDesign();

    QList<UiElement> sorted = design.elements;
    std::sort(sorted.begin(), sorted.end(), [](const UiElement &a, const UiElement &b) {
        return a.zValue < b.zValue;
    });

    for (const UiElement &el : sorted) {
        auto *item = new QListWidgetItem(el.name, m_layerList);
        item->setData(Qt::UserRole, el.id);
    }

    if (!m_selectedElementId.isEmpty()) {
        for (int i = 0; i < m_layerList->count(); ++i) {
            if (m_layerList->item(i)->data(Qt::UserRole).toString() == m_selectedElementId) {
                m_layerList->setCurrentRow(i);
                break;
            }
        }
    }

    m_layerList->blockSignals(false);
}

void DeveloperMode::onLayerSelected()
{
    QListWidgetItem *item = m_layerList->currentItem();
    if (!item) return;
    const QString id = item->data(Qt::UserRole).toString();

    QGraphicsScene *scene = m_canvas->scene();
    if (!scene) return;

    scene->clearSelection();
    for (QGraphicsItem *gi : scene->items()) {
        if (gi->type() != UiElementGraphicsItem::Type) continue;
        auto *uiItem = static_cast<UiElementGraphicsItem *>(gi);
        if (uiItem->element().id == id) {
            uiItem->setSelected(true);
            onSelectedElementChanged(id);
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// Element selection / move / resize
// ---------------------------------------------------------------------------

void DeveloperMode::onSelectedElementChanged(const QString &elementId)
{
    m_selectedElementId = elementId;

    const UiDesign design = m_canvas->currentDesign();
    const UiElement *el = design.findElement(elementId);
    if (el) {
        refreshInspector(*el);

        for (int i = 0; i < m_layerList->count(); ++i) {
            if (m_layerList->item(i)->data(Qt::UserRole).toString() == elementId) {
                m_layerList->blockSignals(true);
                m_layerList->setCurrentRow(i);
                m_layerList->blockSignals(false);
                break;
            }
        }
    }
}

void DeveloperMode::onSelectionCleared()
{
    m_selectedElementId.clear();
    m_layerList->blockSignals(true);
    m_layerList->clearSelection();
    m_layerList->blockSignals(false);

    m_inspectorPage->setEnabled(false);
}

void DeveloperMode::onElementMoved(const QString &id, double x, double y)
{
    if (id != m_selectedElementId) return;
    m_loadingInspector = true;
    m_xSpin->setValue(x);
    m_ySpin->setValue(y);
    m_loadingInspector = false;
}

void DeveloperMode::onElementResized(const QString &id, double w, double h)
{
    if (id != m_selectedElementId) return;
    m_loadingInspector = true;
    m_wSpin->setValue(w);
    m_hSpin->setValue(h);
    m_loadingInspector = false;
}

// ---------------------------------------------------------------------------
// Inspector
// ---------------------------------------------------------------------------

void DeveloperMode::refreshInspector(const UiElement &el)
{
    m_loadingInspector = true;
    m_inspectorPage->setEnabled(true);

    m_nameEdit->setText(el.name);
    m_xSpin->setValue(el.x);
    m_ySpin->setValue(el.y);
    m_wSpin->setValue(el.width);
    m_hSpin->setValue(el.height);
    m_zSpin->setValue(el.zValue);
    m_opacitySpin->setValue(el.opacity);
    m_visibleCheck->setChecked(el.visible);
    m_lockedCheck->setChecked(el.locked);
    m_fillEdit->setText(el.fill);
    m_strokeEdit->setText(el.stroke);
    m_strokeWidthSpin->setValue(el.strokeWidth);
    m_cornerSpin->setValue(el.cornerRadius);
    m_textEdit->setText(el.text);
    m_fontEdit->setText(el.fontFamily);
    m_fontSizeSpin->setValue(el.fontSize);
    m_fontWeightSpin->setValue(el.fontWeight);
    m_textColorEdit->setText(el.textColor);
    m_imageRefEdit->setText(el.imageRef);
    m_bubbleColorEdit->setText(el.bubbleColor);
    m_bubbleCornerSpin->setValue(el.bubbleCornerRadius);
    m_tailCheck->setChecked(el.tailEnabled);
    m_outgoingCheck->setChecked(el.bubbleOutgoing);
    m_lineColorEdit->setText(el.lineColor);
    m_lineWidthSpin->setValue(el.lineWidth);

    m_loadingInspector = false;
}

void DeveloperMode::syncElementToInspector(UiElement &el)
{
    el.name = m_nameEdit->text();
    el.x = m_xSpin->value();
    el.y = m_ySpin->value();
    el.width = m_wSpin->value();
    el.height = m_hSpin->value();
    el.zValue = m_zSpin->value();
    el.opacity = m_opacitySpin->value();
    el.visible = m_visibleCheck->isChecked();
    el.locked = m_lockedCheck->isChecked();
    el.fill = m_fillEdit->text();
    el.stroke = m_strokeEdit->text();
    el.strokeWidth = m_strokeWidthSpin->value();
    el.cornerRadius = m_cornerSpin->value();
    el.text = m_textEdit->text();
    el.fontFamily = m_fontEdit->text();
    el.fontSize = m_fontSizeSpin->value();
    el.fontWeight = m_fontWeightSpin->value();
    el.textColor = m_textColorEdit->text();
    el.imageRef = m_imageRefEdit->text();
    el.bubbleColor = m_bubbleColorEdit->text();
    el.bubbleCornerRadius = m_bubbleCornerSpin->value();
    el.tailEnabled = m_tailCheck->isChecked();
    el.bubbleOutgoing = m_outgoingCheck->isChecked();
    el.lineColor = m_lineColorEdit->text();
    el.lineWidth = m_lineWidthSpin->value();
}

void DeveloperMode::onInspectorChanged()
{
    if (m_loadingInspector || m_selectedElementId.isEmpty()) return;

    UiDesign design = m_canvas->currentDesign();
    UiElement *el = design.findElement(m_selectedElementId);
    if (!el) return;

    syncElementToInspector(*el);
    m_canvas->updateElement(m_selectedElementId, *el);

    if (!m_selectedElementId.isEmpty()) {
        for (int i = 0; i < m_layerList->count(); ++i) {
            if (m_layerList->item(i)->data(Qt::UserRole).toString() == m_selectedElementId) {
                m_layerList->blockSignals(true);
                m_layerList->item(i)->setText(el->name);
                m_layerList->blockSignals(false);
                break;
            }
        }
    }

    emit designChanged();
}

// ---------------------------------------------------------------------------
// Zoom
// ---------------------------------------------------------------------------

void DeveloperMode::onZoomChanged()
{
    auto *btn = qobject_cast<QToolButton *>(sender());
    if (!btn) return;

    const QString name = btn->objectName();
    if (name == QStringLiteral("zoomInBtn"))
        m_canvas->zoomIn();
    else if (name == QStringLiteral("zoomOutBtn"))
        m_canvas->zoomOut();
    else if (name == QStringLiteral("zoomFitBtn"))
        m_canvas->zoomFit();
    else if (name == QStringLiteral("zoom100Btn"))
        m_canvas->zoomReset();

    updateZoomLabel();
}

void DeveloperMode::updateZoomLabel()
{
    const int pct = static_cast<int>(m_canvas->zoom() * 100.0 + 0.5);
    m_zoomLabel->setText(QStringLiteral("%1%").arg(pct));
}

// ---------------------------------------------------------------------------
// Grid / Snap toggles
// ---------------------------------------------------------------------------

void DeveloperMode::onToggleGrid()
{
    const bool visible = m_gridBtn->isChecked();
    m_canvas->setGridVisible(visible);
}

void DeveloperMode::onToggleSnap()
{
    const bool enabled = m_snapBtn->isChecked();
    m_canvas->setSnapEnabled(enabled);
}

// ---------------------------------------------------------------------------
// Z-order operations
// ---------------------------------------------------------------------------

void DeveloperMode::onDeleteSelected()
{
    m_canvas->deleteSelected();
    refreshLayerList();
}

void DeveloperMode::onDuplicateSelected()
{
    m_canvas->duplicateSelected();
    refreshLayerList();
}

void DeveloperMode::onBringForward()
{
    m_canvas->bringForward();
    refreshLayerList();
}

void DeveloperMode::onSendBackward()
{
    m_canvas->sendBackward();
    refreshLayerList();
}

void DeveloperMode::onBringToFront()
{
    m_canvas->bringToFront();
    refreshLayerList();
}

void DeveloperMode::onSendToBack()
{
    m_canvas->sendToBack();
    refreshLayerList();
}
