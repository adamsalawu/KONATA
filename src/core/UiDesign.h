#pragma once

#include <QJsonObject>
#include <QList>
#include <QString>

#include "core/Id.h"

namespace Konata {
namespace Core {

enum class UiElementType {
    Rectangle,
    RoundedRect,
    Circle,
    Line,
    Image,
    Text,
    MessageBubble,
    Avatar,
    SenderName,
    Timestamp,
    Composer,
    InputField,
    Button,
    Icon,
    Divider
};

enum class UiAnchor { TopLeft, TopCenter, TopRight, MiddleLeft, MiddleCenter, MiddleRight, BottomLeft, BottomCenter, BottomRight };

struct UiElement {
    QString id;
    QString name;
    UiElementType type = UiElementType::Rectangle;

    double x = 0;
    double y = 0;
    double width = 100;
    double height = 100;
    double rotation = 0;
    int zValue = 0;
    double opacity = 1.0;
    bool visible = true;
    bool locked = false;

    QString fill;
    QString stroke;
    double strokeWidth = 0;
    double cornerRadius = 0;
    double cornerRadiusBL = -1;
    double cornerRadiusBR = -1;
    double cornerRadiusTL = -1;
    double cornerRadiusTR = -1;

    QString text;
    QString fontFamily;
    int fontSize = 14;
    int fontWeight = 400;
    bool fontItalic = false;
    int textAlign = 0;
    double lineHeight = 1.0;
    QString textColor;

    QString imageRef;
    int imageScaling = 0;

    bool bubbleOutgoing = false;
    QString bubbleColor;
    double bubbleCornerRadius = 18;
    double groupedCornerRadius = 6;
    bool tailEnabled = true;
    double tailWidth = 6;
    double tailHeight = 4;
    double bubblePaddingH = 12;
    double bubblePaddingV = 7;

    QString iconType;
    QString buttonLabel;

    QString lineColor;
    double lineWidth = 1;

    QString refMessageId;
    QString refCharacterId;
    QString refConversationId;

    UiElement() : id(IdGenerator::create(QStringLiteral("uiel"))) {}

    static UiElement create(UiElementType type, double x, double y, double w, double h);
    QString typeName() const;
    static UiElementType typeFromName(const QString &name);
};

struct UiDesign {
    QList<UiElement> elements;
    int nextZ = 0;

    UiElement *findElement(const QString &id);
    const UiElement *findElement(const QString &id) const;
    void bringForward(const QString &id);
    void sendBackward(const QString &id);
    void bringToFront(const QString &id);
    void sendToBack(const QString &id);
    void removeElement(const QString &id);

    QJsonObject serialize() const;
    static UiDesign deserialize(const QJsonObject &obj);
};

} // namespace Core
} // namespace Konata
