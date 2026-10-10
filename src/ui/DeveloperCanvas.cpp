#include "ui/DeveloperCanvas.h"

#include <QGraphicsScene>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QTransform>
#include <QWheelEvent>

#include "ui/UiElementGraphicsItem.h"

using namespace Konata::Core;

DeveloperCanvas::DeveloperCanvas(QWidget *parent)
    : QGraphicsView(parent)
{
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);

    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);
    setDragMode(QGraphicsView::NoDrag);
    setViewportUpdateMode(FullViewportUpdate);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setTransformationAnchor(AnchorUnderMouse);
    setResizeAnchor(AnchorUnderMouse);
    setStyleSheet(QStringLiteral("QGraphicsView { background: #2C2C2E; border: none; }"));

    setupScene();

    connect(m_scene, &QGraphicsScene::selectionChanged, this, [this]() {
        QList<QGraphicsItem *> sel = m_scene->selectedItems();
        if (sel.isEmpty()) {
            emit selectionCleared();
            return;
        }
        for (QGraphicsItem *gi : sel) {
            auto *item = qgraphicsitem_cast<UiElementGraphicsItem *>(gi);
            if (item) {
                emit elementSelected(item->element().id);
                return;
            }
        }
    });
}

void DeveloperCanvas::setupScene()
{
    m_scene->clear();
    m_canvasSize = QSize(1080, 1920);
    const double pad = 200;
    m_scene->setSceneRect(QRectF(QPointF(-pad, -pad),
                                  QSizeF(m_canvasSize.width() + 2 * pad,
                                         m_canvasSize.height() + 2 * pad)));
    viewport()->update();
    zoomFit();
}

void DeveloperCanvas::setDesign(const UiDesign &design)
{
    m_design = design;
    m_scene->clear();
    m_items.clear();

    for (const UiElement &el : m_design.elements) {
        auto *item = new UiElementGraphicsItem(el);
        m_items.append(item);
        m_scene->addItem(item);
    }
    viewport()->update();
}

void DeveloperCanvas::refreshFromDesign()
{
    for (UiElementGraphicsItem *item : std::as_const(m_items)) {
        const UiElement *el = m_design.findElement(item->element().id);
        if (el) {
            item->setElement(*el);
        }
    }
}

void DeveloperCanvas::updateElement(const QString &id, const UiElement &el)
{
    UiElement *designEl = m_design.findElement(id);
    if (designEl) *designEl = el;

    for (UiElementGraphicsItem *item : std::as_const(m_items)) {
        if (item->element().id == id) {
            item->setElement(el);
            break;
        }
    }
    viewport()->update();
}

UiDesign DeveloperCanvas::currentDesign() const
{
    UiDesign design = m_design;
    design.elements.clear();
    design.nextZ = m_design.nextZ;

    for (UiElementGraphicsItem *item : m_items) {
        UiElement el = item->element();
        el.x = item->pos().x();
        el.y = item->pos().y();
        el.zValue = static_cast<int>(item->zValue());
        el.opacity = item->opacity();
        el.visible = item->isVisible();
        design.elements.append(el);
    }
    return design;
}

void DeveloperCanvas::setCanvasSize(const QSize &size)
{
    m_canvasSize = size;
    const double pad = 200;
    m_scene->setSceneRect(QRectF(QPointF(-pad, -pad),
                                  QSizeF(m_canvasSize.width() + 2 * pad,
                                         m_canvasSize.height() + 2 * pad)));
    viewport()->update();
}

void DeveloperCanvas::setZoom(double zoom)
{
    m_zoom = qBound(0.05, zoom, 10.0);
    QTransform t;
    t.scale(m_zoom, m_zoom);
    setTransform(t);
    viewport()->update();
}

void DeveloperCanvas::zoomIn()
{
    setZoom(m_zoom * 1.25);
}

void DeveloperCanvas::zoomOut()
{
    setZoom(m_zoom / 1.25);
}

void DeveloperCanvas::zoomFit()
{
    QRectF canvasRect(0, 0, m_canvasSize.width(), m_canvasSize.height());

    if (m_items.isEmpty()) {
        double scaleX = viewport()->width() / (canvasRect.width() + 80.0);
        double scaleY = viewport()->height() / (canvasRect.height() + 80.0);
        setZoom(qMin(scaleX, scaleY));
        centerOn(canvasRect.center());
        return;
    }

    QRectF itemsBounds;
    for (QGraphicsItem *item : std::as_const(m_items)) {
        itemsBounds |= item->mapToScene(item->boundingRect()).boundingRect();
    }

    QRectF viewBounds = itemsBounds.united(canvasRect);
    viewBounds.adjust(-40, -40, 40, 40);

    double scaleX = viewport()->width() / viewBounds.width();
    double scaleY = viewport()->height() / viewBounds.height();
    setZoom(qMin(scaleX, scaleY));
    centerOn(viewBounds.center());
}

void DeveloperCanvas::zoomReset()
{
    setZoom(1.0);
    centerOn(QPointF(m_canvasSize.width() / 2.0, m_canvasSize.height() / 2.0));
}

void DeveloperCanvas::deleteSelected()
{
    QList<QGraphicsItem *> selected = m_scene->selectedItems();
    if (selected.isEmpty()) return;

    for (QGraphicsItem *gi : selected) {
        auto *item = qgraphicsitem_cast<UiElementGraphicsItem *>(gi);
        if (!item) continue;
        m_design.removeElement(item->element().id);
        m_items.removeAll(item);
        m_scene->removeItem(item);
        delete item;
    }
    emit selectionCleared();
    emit designChanged();
}

void DeveloperCanvas::duplicateSelected()
{
    QList<QGraphicsItem *> selected = m_scene->selectedItems();
    if (selected.isEmpty()) return;

    QList<QString> newIds;

    for (QGraphicsItem *gi : selected) {
        auto *src = qgraphicsitem_cast<UiElementGraphicsItem *>(gi);
        if (!src) continue;

        UiElement el = src->element();
        el.id = IdGenerator::create(QStringLiteral("uiel"));
        el.x += 20;
        el.y += 20;
        el.name += QStringLiteral(" (copy)");

        m_design.elements.append(el);
        auto *item = new UiElementGraphicsItem(el);
        m_items.append(item);
        m_scene->addItem(item);
        item->setSelected(true);
        newIds.append(el.id);
    }

    if (!newIds.isEmpty()) {
        emit elementSelected(newIds.last());
        emit designChanged();
    }
}

void DeveloperCanvas::bringForward()
{
    QList<QGraphicsItem *> selected = m_scene->selectedItems();
    for (QGraphicsItem *gi : selected) {
        auto *item = qgraphicsitem_cast<UiElementGraphicsItem *>(gi);
        if (!item) continue;
        m_design.bringForward(item->element().id);
        const UiElement *el = m_design.findElement(item->element().id);
        if (el) item->setZValue(el->zValue);
    }
    emit designChanged();
}

void DeveloperCanvas::sendBackward()
{
    QList<QGraphicsItem *> selected = m_scene->selectedItems();
    for (QGraphicsItem *gi : selected) {
        auto *item = qgraphicsitem_cast<UiElementGraphicsItem *>(gi);
        if (!item) continue;
        m_design.sendBackward(item->element().id);
        const UiElement *el = m_design.findElement(item->element().id);
        if (el) item->setZValue(el->zValue);
    }
    emit designChanged();
}

void DeveloperCanvas::bringToFront()
{
    QList<QGraphicsItem *> selected = m_scene->selectedItems();
    for (QGraphicsItem *gi : selected) {
        auto *item = qgraphicsitem_cast<UiElementGraphicsItem *>(gi);
        if (!item) continue;
        m_design.bringToFront(item->element().id);
        const UiElement *el = m_design.findElement(item->element().id);
        if (el) item->setZValue(el->zValue);
    }
    emit designChanged();
}

void DeveloperCanvas::sendToBack()
{
    QList<QGraphicsItem *> selected = m_scene->selectedItems();
    for (QGraphicsItem *gi : selected) {
        auto *item = qgraphicsitem_cast<UiElementGraphicsItem *>(gi);
        if (!item) continue;
        m_design.sendToBack(item->element().id);
        const UiElement *el = m_design.findElement(item->element().id);
        if (el) item->setZValue(el->zValue);
    }
    emit designChanged();
}

void DeveloperCanvas::selectElement(const QString &id)
{
    m_scene->clearSelection();
    for (UiElementGraphicsItem *item : std::as_const(m_items)) {
        if (item->element().id == id) {
            item->setSelected(true);
            emit elementSelected(id);
            return;
        }
    }
}

void DeveloperCanvas::setGridVisible(bool visible)
{
    m_gridVisible = visible;
    viewport()->update();
}

QPointF DeveloperCanvas::snapPoint(const QPointF &point) const
{
    QPointF result = point;

    if (m_snapEnabled) {
        // Snap to grid
        const double gx = qRound(result.x() / m_gridSize) * m_gridSize;
        const double gy = qRound(result.y() / m_gridSize) * m_gridSize;
        if (qAbs(result.x() - gx) < m_gridSize / 4.0)
            result.setX(gx);
        if (qAbs(result.y() - gy) < m_gridSize / 4.0)
            result.setY(gy);

        // Snap to canvas center
        const double cx = m_canvasSize.width() / 2.0;
        const double cy = m_canvasSize.height() / 2.0;
        if (qAbs(result.x() - cx) < 6.0) result.setX(cx);
        if (qAbs(result.y() - cy) < 6.0) result.setY(cy);

        // Snap to canvas edges
        const double threshold = 6.0;
        if (qAbs(result.x()) < threshold) result.setX(0);
        if (qAbs(result.y()) < threshold) result.setY(0);
        if (qAbs(result.x() - m_canvasSize.width()) < threshold)
            result.setX(m_canvasSize.width());
        if (qAbs(result.y() - m_canvasSize.height()) < threshold)
            result.setY(m_canvasSize.height());
    }

    return result;
}

void DeveloperCanvas::updateRubberBand(const QPoint &from, const QPoint &to)
{
    Q_UNUSED(from);
    Q_UNUSED(to);
}

void DeveloperCanvas::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton) {
        m_panning = true;
        m_panStart = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton) {
        QPointF scenePos = mapToScene(event->pos());

        QList<QGraphicsItem *> selected = m_scene->selectedItems();
        for (QGraphicsItem *gi : selected) {
            auto *item = qgraphicsitem_cast<UiElementGraphicsItem *>(gi);
            if (!item) continue;

            QPointF localPos = item->mapFromScene(scenePos);
            UiElementGraphicsItem::Handle handle = item->handleAt(localPos);
            if (handle != UiElementGraphicsItem::NoHandle) {
                m_resizing = true;
                m_resizeItem = item;
                m_resizeHandle = handle;
                m_resizeStart = scenePos;
                m_resizeOriginal = QRectF(item->pos().x(), item->pos().y(),
                                          item->element().width, item->element().height);
                event->accept();
                return;
            }
        }

        m_itemMoved = false;
    }

    QGraphicsView::mousePressEvent(event);
}

void DeveloperCanvas::mouseMoveEvent(QMouseEvent *event)
{
    if (m_panning) {
        QPoint delta = event->pos() - m_panStart;
        m_panStart = event->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        event->accept();
        return;
    }

    if (m_resizing && m_resizeItem) {
        QPointF current = mapToScene(event->pos());
        QPointF delta = current - m_resizeStart;

        UiElement el = m_resizeItem->element();
        double newX = m_resizeOriginal.x();
        double newY = m_resizeOriginal.y();
        double newW = m_resizeOriginal.width();
        double newH = m_resizeOriginal.height();

        switch (m_resizeHandle) {
        case UiElementGraphicsItem::BottomRight:
            newW = qMax(10.0, newW + delta.x());
            newH = qMax(10.0, newH + delta.y());
            break;
        case UiElementGraphicsItem::BottomLeft:
            newX += delta.x();
            newW = qMax(10.0, newW - delta.x());
            newH = qMax(10.0, newH + delta.y());
            break;
        case UiElementGraphicsItem::TopRight:
            newY += delta.y();
            newW = qMax(10.0, newW + delta.x());
            newH = qMax(10.0, newH - delta.y());
            break;
        case UiElementGraphicsItem::TopLeft:
            newX += delta.x();
            newY += delta.y();
            newW = qMax(10.0, newW - delta.x());
            newH = qMax(10.0, newH - delta.y());
            break;
        case UiElementGraphicsItem::Left:
            newX += delta.x();
            newW = qMax(10.0, newW - delta.x());
            break;
        case UiElementGraphicsItem::Right:
            newW = qMax(10.0, newW + delta.x());
            break;
        case UiElementGraphicsItem::Top:
            newY += delta.y();
            newH = qMax(10.0, newH - delta.y());
            break;
        case UiElementGraphicsItem::Bottom:
            newH = qMax(10.0, newH + delta.y());
            break;
        default:
            break;
        }

        QPointF snappedPos = snapPoint(QPointF(newX, newY));
        newX = snappedPos.x();
        newY = snappedPos.y();

        el.x = newX;
        el.y = newY;
        el.width = newW;
        el.height = newH;
        m_resizeItem->setElement(el);
        setCursor(UiElementGraphicsItem::cursorForHandle(m_resizeHandle));
        event->accept();
        return;
    }

    if (event->buttons() & Qt::LeftButton && !m_resizing) {
        QList<QGraphicsItem *> sel = m_scene->selectedItems();
        if (!sel.isEmpty()) m_itemMoved = true;
    }

    // Update cursor when hovering over resize handles on selected items
    if (event->buttons() == Qt::NoButton) {
        QPointF scenePos = mapToScene(event->pos());
        QList<QGraphicsItem *> selected = m_scene->selectedItems();
        bool overHandle = false;
        for (QGraphicsItem *gi : selected) {
            auto *item = qgraphicsitem_cast<UiElementGraphicsItem *>(gi);
            if (!item) continue;
            QPointF localPos = item->mapFromScene(scenePos);
            UiElementGraphicsItem::Handle handle = item->handleAt(localPos);
            if (handle != UiElementGraphicsItem::NoHandle) {
                setCursor(UiElementGraphicsItem::cursorForHandle(handle));
                overHandle = true;
                break;
            }
        }
        if (!overHandle) {
            unsetCursor();
        }
    }

    QGraphicsView::mouseMoveEvent(event);
}

void DeveloperCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton && m_panning) {
        m_panning = false;
        unsetCursor();
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton && m_resizing && m_resizeItem) {
        QPointF scenePos = mapToScene(event->pos());
        QPointF snappedPos = snapPoint(scenePos);

        UiElement el = m_resizeItem->element();
        el.x = snappedPos.x();
        el.y = snappedPos.y();

        m_resizeItem->setElement(el);
        m_resizing = false;
        m_resizeItem = nullptr;
        m_resizeHandle = UiElementGraphicsItem::NoHandle;
        unsetCursor();

        emit elementResized(el.id, el.width, el.height);
        emit designChanged();
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton && m_itemMoved) {
        m_itemMoved = false;
        emit designChanged();
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void DeveloperCanvas::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        const double factor = (event->angleDelta().y() > 0) ? 1.15 : 1.0 / 1.15;
        setZoom(m_zoom * factor);
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void DeveloperCanvas::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        deleteSelected();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_D && (event->modifiers() & Qt::ControlModifier)) {
        duplicateSelected();
        event->accept();
        return;
    }

    QGraphicsView::keyPressEvent(event);
}

void DeveloperCanvas::drawBackground(QPainter *painter, const QRectF &rect)
{
    painter->fillRect(rect, QColor(QStringLiteral("#1C1C1E")));

    QRectF canvasRect(0, 0, m_canvasSize.width(), m_canvasSize.height());

    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(30, 30, 32));
    painter->drawRect(canvasRect.adjusted(-2, -2, 2, 2));

    painter->setPen(Qt::NoPen);
    painter->setBrush(Qt::white);
    painter->drawRect(canvasRect);

    if (m_gridVisible) {
        painter->setPen(QPen(QColor(200, 200, 200, 80), 0.5));
        for (double x = m_gridSize; x < m_canvasSize.width(); x += m_gridSize) {
            painter->drawLine(QPointF(x, 0), QPointF(x, m_canvasSize.height()));
        }
        for (double y = m_gridSize; y < m_canvasSize.height(); y += m_gridSize) {
            painter->drawLine(QPointF(0, y), QPointF(m_canvasSize.width(), y));
        }
    }

    painter->setPen(QPen(QColor(80, 80, 80), 1.0));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(canvasRect);
}
