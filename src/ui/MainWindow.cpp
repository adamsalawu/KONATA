#include "ui/MainWindow.h"

#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>

#include "core/CanvasSettings.h"
#include "export/ExportService.h"
#include "rendering/skins/AppleMessagesSkin.h"
#include "ui/DeveloperMode.h"
#include "ui/InspectorPanel.h"
#include "ui/PreviewArea.h"
#include "ui/ProjectPanel.h"
#include "ui/TimelinePanel.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setMinimumSize(1024, 640);
    setObjectName(QStringLiteral("KonataMainWindow"));

    loadStyleSheet();
    setupMenuBar();
    setupToolBar();
    setupCentralLayout();
    setupStatusBar();
    wireDocument();

    // Fresh untitled project so the tree/editors have structured data immediately.
    m_document.newProject();
    updateChrome();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeDiscard())
        event->accept();
    else
        event->ignore();
}

void MainWindow::onNewProject()
{
    if (!maybeDiscard())
        return;
    if (m_inDeveloperMode) onToggleDeveloperMode();
    m_document.newProject();
}

void MainWindow::onOpenProject()
{
    if (!maybeDiscard())
        return;
    if (m_inDeveloperMode) onToggleDeveloperMode();
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Open KONATA Project"), QString(), tr("KONATA Projects (*.konata)"));
    if (path.isEmpty())
        return;
    if (!m_document.open(path)) {
        QMessageBox::warning(this, tr("Open Failed"),
                             tr("Could not open project:\n%1").arg(m_document.lastError()));
    }
}

void MainWindow::onSaveProject()
{
    if (m_inDeveloperMode) {
        m_document.setUiDesign(m_developerMode->currentDesign());
    }
    if (!m_document.hasFile()) {
        onSaveProjectAs();
        return;
    }
    if (!m_document.save()) {
        QMessageBox::warning(this, tr("Save Failed"),
                             tr("Could not save project:\n%1").arg(m_document.lastError()));
    }
}

void MainWindow::onSaveProjectAs()
{
    if (m_inDeveloperMode) {
        m_document.setUiDesign(m_developerMode->currentDesign());
    }
    QString path = QFileDialog::getSaveFileName(
        this, tr("Save KONATA Project"), QString(), tr("KONATA Projects (*.konata)"));
    if (path.isEmpty())
        return;
    if (!path.endsWith(QStringLiteral(".konata"), Qt::CaseInsensitive))
        path += QStringLiteral(".konata");
    if (!m_document.saveAs(path)) {
        QMessageBox::warning(this, tr("Save Failed"),
                             tr("Could not save project:\n%1").arg(m_document.lastError()));
    }
}

void MainWindow::onExportPng()
{
    // Determine conversation to export
    QString convId;
    const auto sel = m_document.selection();
    if (sel.type == ProjectDocument::SelectionType::Conversation) {
        convId = sel.id;
    } else if (sel.type == ProjectDocument::SelectionType::Message) {
        convId = sel.parentId;
    } else if (!m_document.story().conversations().isEmpty()) {
        convId = m_document.story().conversations().first().id();
    }
    if (convId.isEmpty() || !m_document.story().findConversation(convId)) {
        QMessageBox::information(this, tr("Export"), tr("No conversation to export. Create a conversation first."));
        return;
    }

    // Export dialog: preset + custom size, dark style
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Export PNG"));
    dialog.setObjectName(QStringLiteral("ExportDialog"));
    auto *form = new QFormLayout(&dialog);
    form->setSpacing(12);
    form->setContentsMargins(16,16,16,16);

    auto *presetCombo = new QComboBox(&dialog);
    presetCombo->addItems(QStringList{QStringLiteral("1:1"), QStringLiteral("4:5"), QStringLiteral("9:16"), QStringLiteral("16:9"), QStringLiteral("Custom")});
    auto currentCanvas = m_document.canvas();
    const QString currentLabel = Konata::Core::CanvasSettings::presetLabel(currentCanvas.preset);
    const int presetIdx = presetCombo->findText(currentLabel);
    if (presetIdx >=0) presetCombo->setCurrentIndex(presetIdx);

    auto *widthSpin = new QSpinBox(&dialog);
    widthSpin->setRange(1, 8192);
    widthSpin->setValue(currentCanvas.customWidth);
    widthSpin->setSuffix(QStringLiteral(" px"));
    auto *heightSpin = new QSpinBox(&dialog);
    heightSpin->setRange(1, 8192);
    heightSpin->setValue(currentCanvas.customHeight);
    heightSpin->setSuffix(QStringLiteral(" px"));
    const bool isCustom = currentCanvas.preset == Konata::Core::CanvasSettings::Preset::Custom;
    widthSpin->setEnabled(isCustom);
    heightSpin->setEnabled(isCustom);

    form->addRow(tr("Aspect Ratio:"), presetCombo);
    form->addRow(tr("Width:"), widthSpin);
    form->addRow(tr("Height:"), heightSpin);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Export"));
    form->addWidget(buttonBox);

    // Preview label for current logical size
    auto *sizeLabel = new QLabel(&dialog);
    sizeLabel->setObjectName(QStringLiteral("CanvasSizeLabel"));
    const auto updateSizeLabel = [&]() {
        Konata::Core::CanvasSettings tmp;
        bool ok=false;
        tmp.preset = Konata::Core::CanvasSettings::presetFromLabel(presetCombo->currentText(), &ok);
        if (!ok) tmp.preset = Konata::Core::CanvasSettings::Preset::Portrait_9x16;
        tmp.customWidth = widthSpin->value();
        tmp.customHeight = heightSpin->value();
        const QSize sz = tmp.logicalSize();
        sizeLabel->setText(tr("Output: %1 × %2").arg(sz.width()).arg(sz.height()));
    };
    form->addRow(sizeLabel);
    updateSizeLabel();

    connect(presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), &dialog, [&](int){
        const QString label = presetCombo->currentText();
        bool ok=false;
        auto p = Konata::Core::CanvasSettings::presetFromLabel(label, &ok);
        const bool custom = p == Konata::Core::CanvasSettings::Preset::Custom;
        widthSpin->setEnabled(custom);
        heightSpin->setEnabled(custom);
        if (!custom) {
            const QSize ps = Konata::Core::CanvasSettings::presetSize(p);
            QSignalBlocker b1(widthSpin), b2(heightSpin);
            widthSpin->setValue(ps.width());
            heightSpin->setValue(ps.height());
        }
        updateSizeLabel();
    });
    connect(widthSpin, QOverload<int>::of(&QSpinBox::valueChanged), &dialog, [&](int){ updateSizeLabel(); });
    connect(heightSpin, QOverload<int>::of(&QSpinBox::valueChanged), &dialog, [&](int){ updateSizeLabel(); });
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted)
        return;

    // Apply canvas settings
    Konata::Core::CanvasSettings newCanvas;
    bool ok=false;
    newCanvas.preset = Konata::Core::CanvasSettings::presetFromLabel(presetCombo->currentText(), &ok);
    newCanvas.customWidth = widthSpin->value();
    newCanvas.customHeight = heightSpin->value();
    if (!newCanvas.isValid()) {
        QMessageBox::warning(this, tr("Export"), tr("Invalid canvas dimensions. Width and height must be 1..8192 and area ≤ %1.").arg(Konata::Core::CanvasSettings::kMaxPixels));
        return;
    }
    m_document.setCanvas(newCanvas);

    const QString savePath = QFileDialog::getSaveFileName(this, tr("Export PNG"), QStringLiteral("konata-export.png"), tr("PNG Images (*.png)"));
    if (savePath.isEmpty())
        return;
    QString finalPath = savePath;
    if (!finalPath.endsWith(QStringLiteral(".png"), Qt::CaseInsensitive))
        finalPath += QStringLiteral(".png");

    const auto result = ExportService::exportPng(m_document.project(), convId, finalPath);
    if (!result.ok) {
        QMessageBox::warning(this, tr("Export Failed"), result.error);
        return;
    }
    QMessageBox::information(this, tr("Export Complete"),
                             tr("Exported %1 × %2 to:\n%3").arg(result.size.width()).arg(result.size.height()).arg(result.path));
    statusBar()->showMessage(tr("Exported %1 × %2 → %3").arg(result.size.width()).arg(result.size.height()).arg(result.path), 5000);
}

void MainWindow::updateChrome()
{
    QString base = tr("KONATA 0.1D");
    if (m_document.hasFile())
        base += QStringLiteral(" — ") + QFileInfo(m_document.filePath()).fileName();
    else
        base += QStringLiteral(" — Untitled");
    if (m_document.isDirty())
        base += QStringLiteral(" *");
    setWindowTitle(base);

    const Konata::Core::Story &story = m_document.story();
    statusBar()->showMessage(
        tr("%1  ·  %2 characters, %3 conversations, %4 scenes%5")
            .arg(m_document.hasFile() ? m_document.filePath() : tr("Unsaved project"))
            .arg(story.characterCount())
            .arg(story.conversationCount())
            .arg(story.sceneCount())
            .arg(m_document.isDirty() ? tr("  ·  unsaved changes") : QString()));
}

void MainWindow::setupMenuBar()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QAction *newAction = fileMenu->addAction(tr("&New Project..."));
    connect(newAction, &QAction::triggered, this, &MainWindow::onNewProject);
    QAction *openAction = fileMenu->addAction(tr("&Open..."));
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenProject);
    QAction *saveAction = fileMenu->addAction(tr("&Save"));
    connect(saveAction, &QAction::triggered, this, &MainWindow::onSaveProject);
    QAction *saveAsAction = fileMenu->addAction(tr("Save &As..."));
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::onSaveProjectAs);
    fileMenu->addSeparator();
    QAction *exportAction = fileMenu->addAction(tr("Export PNG..."));
    connect(exportAction, &QAction::triggered, this, &MainWindow::onExportPng);
    fileMenu->addSeparator();
    QAction *exitAction = fileMenu->addAction(tr("E&xit"));
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->setEnabled(true);

    // Preview skin presets: replace the project's AppDefinition through the
    // document (normal dirty-marking mutation, serializes into .konata).
    // The renderer stays generic; these actions only choose the definition.
    QMenu *skinMenu = viewMenu->addMenu(tr("Preview Skin"));
    QAction *appleAction = skinMenu->addAction(tr("Apple Messages"));
    connect(appleAction, &QAction::triggered, this, [this]() {
        m_document.applyAppDefinition(AppleMessagesSkin::createAppDefinition());
    });
    QAction *neutralAction = skinMenu->addAction(tr("Neutral (Konata)"));
    connect(neutralAction, &QAction::triggered, this, [this]() {
        m_document.applyAppDefinition(AppleMessagesSkin::createNeutralAppDefinition());
    });

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    QAction *aboutAction = helpMenu->addAction(tr("&About KONATA"));
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, tr("About KONATA"),
                           tr("KONATA 0.1D — Structured project authoring.\n"
                              "Qt 6.8.1 + CMake + MSVC 2022."));
    });
}

void MainWindow::setupToolBar()
{
    QToolBar *toolbar = addToolBar(tr("Main"));
    toolbar->setObjectName(QStringLiteral("KonataMainToolBar"));
    toolbar->setMovable(false);
    toolbar->setFloatable(false);

    QAction *newAction = toolbar->addAction(tr("New"));
    connect(newAction, &QAction::triggered, this, &MainWindow::onNewProject);
    QAction *openAction = toolbar->addAction(tr("Open"));
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenProject);
    QAction *saveAction = toolbar->addAction(tr("Save"));
    connect(saveAction, &QAction::triggered, this, &MainWindow::onSaveProject);
    QAction *exportAction = toolbar->addAction(tr("Export"));
    connect(exportAction, &QAction::triggered, this, &MainWindow::onExportPng);
}

void MainWindow::setupCentralLayout()
{
    auto *central = new QWidget(this);
    central->setObjectName(QStringLiteral("KonataCentral"));
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(6);

    m_verticalSplitter = new QSplitter(Qt::Vertical, central);
    m_verticalSplitter->setObjectName(QStringLiteral("KonataVerticalSplitter"));
    m_verticalSplitter->setChildrenCollapsible(false);

    m_horizontalSplitter = new QSplitter(Qt::Horizontal);
    m_horizontalSplitter->setObjectName(QStringLiteral("KonataHorizontalSplitter"));
    m_horizontalSplitter->setChildrenCollapsible(false);

    m_projectPanel = new ProjectPanel(m_horizontalSplitter);
    m_previewArea = new PreviewArea(m_horizontalSplitter);
    m_inspectorPanel = new InspectorPanel(m_horizontalSplitter);
    m_developerMode = new DeveloperMode(m_horizontalSplitter);
    m_developerMode->setVisible(false);

    m_centerStack = new QStackedWidget(m_horizontalSplitter);
    m_centerStack->addWidget(m_previewArea);
    m_centerStack->addWidget(m_developerMode);
    m_centerStack->setCurrentIndex(0);

    m_horizontalSplitter->addWidget(m_projectPanel);
    m_horizontalSplitter->addWidget(m_centerStack);
    m_horizontalSplitter->addWidget(m_inspectorPanel);

    // Center preview receives the majority of space.
    m_horizontalSplitter->setStretchFactor(0, 1);
    m_horizontalSplitter->setStretchFactor(1, 3);
    m_horizontalSplitter->setStretchFactor(2, 1);
    m_horizontalSplitter->setSizes({220, 720, 260});

    m_timelinePanel = new TimelinePanel(m_verticalSplitter);

    m_verticalSplitter->addWidget(m_horizontalSplitter);
    m_verticalSplitter->addWidget(m_timelinePanel);
    m_verticalSplitter->setStretchFactor(0, 4);
    m_verticalSplitter->setStretchFactor(1, 1);
    m_verticalSplitter->setSizes({580, 180});

    layout->addWidget(m_verticalSplitter);
    setCentralWidget(central);

    for (QAction *menuAction : menuBar()->actions()) {
        if (menuAction->text() == tr("&View")) {
            if (QMenu *viewMenu = menuAction->menu()) {
                QAction *projectToggle = viewMenu->addAction(tr("Project Panel"));
                projectToggle->setCheckable(true);
                projectToggle->setChecked(true);
                connect(projectToggle, &QAction::toggled, m_projectPanel, &QWidget::setVisible);

                QAction *inspectorToggle = viewMenu->addAction(tr("Inspector Panel"));
                inspectorToggle->setCheckable(true);
                inspectorToggle->setChecked(true);
                connect(inspectorToggle, &QAction::toggled, m_inspectorPanel, &QWidget::setVisible);

                QAction *timelineToggle = viewMenu->addAction(tr("Timeline Panel"));
                timelineToggle->setCheckable(true);
                timelineToggle->setChecked(true);
                connect(timelineToggle, &QAction::toggled, m_timelinePanel, &QWidget::setVisible);

                viewMenu->addSeparator();
                QAction *devModeAction = viewMenu->addAction(tr("Developer Mode"));
                devModeAction->setCheckable(true);
                devModeAction->setChecked(false);
                connect(devModeAction, &QAction::toggled, this, &MainWindow::onToggleDeveloperMode);
            }
        }
    }
}

void MainWindow::setupStatusBar()
{
    statusBar()->setObjectName(QStringLiteral("KonataStatusBar"));
}

void MainWindow::loadStyleSheet()
{
    // Prefer embedded resource so the exe works from any working directory.
    const QStringList candidates = {
        QStringLiteral(":/styles/dark.qss"),
        QStringLiteral("resources/styles/dark.qss"),
        QStringLiteral("../resources/styles/dark.qss")
    };

    for (const QString &path : candidates) {
        QFile file(path);
        if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QString style = QString::fromUtf8(file.readAll());
            qApp->setStyleSheet(style);
            return;
        }
    }
}

void MainWindow::wireDocument()
{
    m_projectPanel->setDocument(&m_document);
    m_inspectorPanel->setDocument(&m_document);
    m_previewArea->setDocument(&m_document);
    m_timelinePanel->setDocument(&m_document);

    connect(m_previewArea, &PreviewArea::messageEditRequested, m_inspectorPanel,
            &InspectorPanel::focusEditableField);

    connect(&m_document, &ProjectDocument::projectChanged, this, &MainWindow::updateChrome);
    connect(&m_document, &ProjectDocument::dirtyChanged, this, &MainWindow::updateChrome);
    connect(&m_document, &ProjectDocument::filePathChanged, this, &MainWindow::updateChrome);

    connect(m_developerMode, &DeveloperMode::exitDeveloperMode, this, [this]() {
        m_inDeveloperMode = false;
        m_centerStack->setCurrentIndex(0);
        m_timelinePanel->setVisible(true);
        updateChrome();
    });
    connect(m_developerMode, &DeveloperMode::designChanged, this, [this]() {
        m_document.setUiDesign(m_developerMode->currentDesign());
    });
    connect(m_developerMode, &DeveloperMode::canvasSettingsChanged, this, [this](const Konata::Core::CanvasSettings &cs) {
        m_document.setCanvas(cs);
    });
}

bool MainWindow::maybeDiscard()
{
    if (!m_document.isDirty())
        return true;
    const QMessageBox::StandardButton answer = QMessageBox::question(
        this, tr("Unsaved Changes"),
        tr("The project has unsaved changes. Save them before continuing?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (answer == QMessageBox::Cancel)
        return false;
    if (answer == QMessageBox::Save) {
        onSaveProject();
        return !m_document.isDirty(); // user may have cancelled the Save As dialog
    }
    return true; // Discard
}

void MainWindow::onToggleDeveloperMode()
{
    m_inDeveloperMode = !m_inDeveloperMode;
    if (m_inDeveloperMode) {
        m_developerMode->setCanvasSettings(m_document.canvas());
        m_developerMode->setDesign(m_document.uiDesign());
        m_centerStack->setCurrentIndex(1);
        m_timelinePanel->setVisible(false);
    } else {
        m_document.setUiDesign(m_developerMode->currentDesign());
        m_centerStack->setCurrentIndex(0);
        m_timelinePanel->setVisible(true);
    }
    updateChrome();
}
