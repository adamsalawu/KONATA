#pragma once

#include <QGraphicsView>
#include <QPointF>
#include <QSet>

#include "core/UiDesign.h"

#include "ui/UiElementGraphicsItem.h"

class DeveloperCanvas : public QGraphicsView
{
    Q_OBJECT

public:
    explicit DeveloperCanvas(QWidget *parent = nullptr);

    void setDesign(const Konata::Core::UiDesign &design);
    void refreshFromDesign();
    void updateElement(const QString &id, const Konata::Core::UiElement &el);
    Konata::Core::UiDesign currentDesign() const;

    void setCanvasSize(const QSize &size);
    QSize canvasSize() const { return m_canvasSize; }

    double zoom() const { return m_zoom; }

    void zoomIn();
    void zoomOut();
    void zoomFit();
    void zoomReset();

    void deleteSelected();
    void duplicateSelected();
    void bringForward();
    void sendBackward();
    void bringToFront();
    void sendToBack();
    void selectElement(const QString &id);

    void setSnapEnabled(bool enabled) { m_snapEnabled = enabled; }
    bool isSnapEnabled() const { return m_snapEnabled; }
    void setGridVisible(bool visible);
    bool isGridVisible() const { return m_gridVisible; }

signals:
    void elementSelected(const QString &elementId);
    void selectionCleared();
    void elementMoved(const QString &elementId, double x, double y);
    void elementResized(const QString &elementId, double w, double h);
    void designChanged();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void setupScene();
    void setZoom(double zoom);
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    QPointF snapPoint(const QPointF &point) const;
    void updateRubberBand(const QPoint &from, const QPoint &to);

    QGraphicsScene *m_scene;
    QSize m_canvasSize;
    double m_zoom = 1.0;
    bool m_snapEnabled = true;
    bool m_gridVisible = false;
    double m_gridSize = 20;

    Konata::Core::UiDesign m_design;
    QList<UiElementGraphicsItem *> m_items;

    // Drag/resize state
    bool m_resizing = false;
    UiElementGraphicsItem *m_resizeItem = nullptr;
    UiElementGraphicsItem::Handle m_resizeHandle = UiElementGraphicsItem::NoHandle;
    QPointF m_resizeStart;
    QRectF m_resizeOriginal;

    // Pan state
    bool m_panning = false;
    QPoint m_panStart;

    // Track item moves during drag
    bool m_itemMoved = false;

    QList<QGraphicsItem *> m_guideLines;
};
