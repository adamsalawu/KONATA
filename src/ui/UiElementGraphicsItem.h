#pragma once

#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsView>

#include "core/UiDesign.h"

class UiElementGraphicsItem : public QGraphicsItem
{
public:
    enum { Type = QGraphicsItem::UserType + 1 };
    int type() const override { return Type; }

    explicit UiElementGraphicsItem(const Konata::Core::UiElement &element, QGraphicsItem *parent = nullptr);

    const Konata::Core::UiElement &element() const { return m_element; }
    Konata::Core::UiElement &mutableElement() { return m_element; }
    void setElement(const Konata::Core::UiElement &el);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    enum Handle { NoHandle, TopLeft, TopRight, BottomLeft, BottomRight, Top, Bottom, Left, Right };
    Handle handleAt(const QPointF &pos) const;
    static QCursor cursorForHandle(Handle h);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    Konata::Core::UiElement m_element;
    static constexpr double kHandleSize = 8;
    void paintElement(QPainter *painter);
    void paintSelection(QPainter *painter);
};
