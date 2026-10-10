#pragma once

#include <QColor>
#include <QWidget>

// Generic per-corner rounded bubble with optional tail (0.2C.3): paints a
// filled path with independent corner radii plus a small triangular tail
// on the last/single bubble of a group (reference-measured). Plain QWidget
// content inside (labels/layouts), no baked images. Resolution-independent.
class RoundedBubble : public QWidget
{
    Q_OBJECT

public:
    explicit RoundedBubble(QWidget *parent = nullptr);

    void setFillColor(const QColor &color);
    QColor fillColor() const { return m_fill; }
    void setCorners(int topLeft, int topRight, int bottomLeft, int bottomRight);
    void setTail(bool hasTail, bool outgoing);
    void setIsGroup(bool isGroup) { m_isGroup = isGroup; }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QColor m_fill = QColor(QStringLiteral("#262628"));
    int m_topLeft = 18;
    int m_topRight = 18;
    int m_bottomLeft = 18;
    int m_bottomRight = 18;
    bool m_hasTail = false;
    bool m_outgoing = false;
    bool m_isGroup = false;
};
