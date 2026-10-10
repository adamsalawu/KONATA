#pragma once

#include <QWidget>

#include "core/UiDesign.h"
#include "core/CanvasSettings.h"

class DeveloperCanvas;
class QComboBox;
class QLabel;
class QListWidget;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class QLineEdit;
class QDoubleSpinBox;
class QCheckBox;
class QToolButton;
class QScrollArea;
class QGridLayout;

class DeveloperMode : public QWidget
{
    Q_OBJECT

public:
    explicit DeveloperMode(QWidget *parent = nullptr);

    void setDesign(const Konata::Core::UiDesign &design);
    Konata::Core::UiDesign currentDesign() const;

    void setCanvasSettings(const Konata::Core::CanvasSettings &canvas);
    Konata::Core::CanvasSettings canvasSettings() const { return m_canvasSettings; }

signals:
    void designChanged();
    void exitDeveloperMode();
    void canvasSettingsChanged(const Konata::Core::CanvasSettings &);

private slots:
    void onAddElement();
    void onSelectedElementChanged(const QString &elementId);
    void onSelectionCleared();
    void onElementMoved(const QString &id, double x, double y);
    void onElementResized(const QString &id, double w, double h);
    void onInspectorChanged();
    void onLayerSelected();
    void onZoomChanged();
    void onCanvasPresetChanged();
    void onToggleGrid();
    void onToggleSnap();
    void onDeleteSelected();
    void onDuplicateSelected();
    void onBringForward();
    void onSendBackward();
    void onBringToFront();
    void onSendToBack();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void buildToolBar();
    void buildElementPalette();
    void buildLayerPanel();
    void buildInspector();
    void refreshLayerList();
    void refreshInspector(const Konata::Core::UiElement &el);
    void syncElementToInspector(Konata::Core::UiElement &el);
    void updateZoomLabel();

    DeveloperCanvas *m_canvas = nullptr;
    Konata::Core::CanvasSettings m_canvasSettings;

    // Toolbar
    QComboBox *m_canvasCombo = nullptr;
    QSpinBox *m_customWidth = nullptr;
    QSpinBox *m_customHeight = nullptr;
    QLabel *m_zoomLabel = nullptr;
    QToolButton *m_gridBtn = nullptr;
    QToolButton *m_snapBtn = nullptr;

    // Element palette
    QGridLayout *m_paletteGrid = nullptr;
    Konata::Core::UiElementType m_pendingAddType = Konata::Core::UiElementType::Rectangle;

    // Layer panel
    QListWidget *m_layerList = nullptr;

    // Inspector
    QWidget *m_inspectorPage = nullptr;
    QLineEdit *m_nameEdit = nullptr;
    QDoubleSpinBox *m_xSpin = nullptr;
    QDoubleSpinBox *m_ySpin = nullptr;
    QDoubleSpinBox *m_wSpin = nullptr;
    QDoubleSpinBox *m_hSpin = nullptr;
    QSpinBox *m_zSpin = nullptr;
    QDoubleSpinBox *m_opacitySpin = nullptr;
    QCheckBox *m_visibleCheck = nullptr;
    QCheckBox *m_lockedCheck = nullptr;
    QLineEdit *m_fillEdit = nullptr;
    QLineEdit *m_strokeEdit = nullptr;
    QDoubleSpinBox *m_strokeWidthSpin = nullptr;
    QDoubleSpinBox *m_cornerSpin = nullptr;
    QLineEdit *m_textEdit = nullptr;
    QLineEdit *m_fontEdit = nullptr;
    QSpinBox *m_fontSizeSpin = nullptr;
    QSpinBox *m_fontWeightSpin = nullptr;
    QLineEdit *m_textColorEdit = nullptr;
    QLineEdit *m_imageRefEdit = nullptr;
    QLineEdit *m_bubbleColorEdit = nullptr;
    QDoubleSpinBox *m_bubbleCornerSpin = nullptr;
    QCheckBox *m_tailCheck = nullptr;
    QCheckBox *m_outgoingCheck = nullptr;
    QLineEdit *m_lineColorEdit = nullptr;
    QDoubleSpinBox *m_lineWidthSpin = nullptr;

    QString m_selectedElementId;
    bool m_loadingInspector = false;
};
