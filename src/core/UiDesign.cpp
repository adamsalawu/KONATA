#include "core/UiDesign.h"

#include <QJsonArray>

namespace Konata {
namespace Core {

UiElement UiElement::create(UiElementType type, double x, double y, double w, double h)
{
    UiElement el;
    el.type = type;
    el.x = x;
    el.y = y;
    el.width = w;
    el.height = h;
    switch (type) {
    case UiElementType::Rectangle:
        el.name = QStringLiteral("Rectangle");
        el.fill = QStringLiteral("#3A3A3C");
        break;
    case UiElementType::RoundedRect:
        el.name = QStringLiteral("Rounded Rect");
        el.fill = QStringLiteral("#3A3A3C");
        el.cornerRadius = 8;
        break;
    case UiElementType::Circle:
        el.name = QStringLiteral("Circle");
        el.fill = QStringLiteral("#3A3A3C");
        break;
    case UiElementType::Line:
        el.name = QStringLiteral("Line");
        el.lineColor = QStringLiteral("#555555");
        el.lineWidth = 1;
        break;
    case UiElementType::Image:
        el.name = QStringLiteral("Image");
        el.fill = QStringLiteral("#2C2C2E");
        break;
    case UiElementType::Text:
        el.name = QStringLiteral("Text");
        el.text = QStringLiteral("Text");
        el.textColor = QStringLiteral("#FFFFFF");
        el.fontFamily = QStringLiteral("Segoe UI");
        el.fontSize = 14;
        break;
    case UiElementType::MessageBubble:
        el.name = QStringLiteral("Message Bubble");
        el.fill = QStringLiteral("#2C2C2E");
        el.cornerRadius = 18;
        el.groupedCornerRadius = 6;
        el.tailEnabled = true;
        el.tailWidth = 6;
        el.tailHeight = 4;
        el.bubblePaddingH = 12;
        el.bubblePaddingV = 7;
        el.bubbleColor = QStringLiteral("#2C2C2E");
        el.text = QStringLiteral("Hello");
        el.textColor = QStringLiteral("#FFFFFF");
        el.fontFamily = QStringLiteral("Segoe UI");
        el.fontSize = 17;
        break;
    case UiElementType::Avatar:
        el.name = QStringLiteral("Avatar");
        el.fill = QStringLiteral("#3A3A3C");
        el.width = 30;
        el.height = 30;
        break;
    case UiElementType::SenderName:
        el.name = QStringLiteral("Sender Name");
        el.text = QStringLiteral("Name");
        el.textColor = QStringLiteral("#8E8E93");
        el.fontSize = 12;
        break;
    case UiElementType::Timestamp:
        el.name = QStringLiteral("Timestamp");
        el.text = QStringLiteral("10:30 AM");
        el.textColor = QStringLiteral("#8E8E93");
        el.fontSize = 12;
        break;
    case UiElementType::Composer:
        el.name = QStringLiteral("Composer");
        el.fill = QStringLiteral("#000000");
        el.height = 52;
        break;
    case UiElementType::InputField:
        el.name = QStringLiteral("Input Field");
        el.fill = QStringLiteral("#1C1C1E");
        el.cornerRadius = 16;
        el.text = QStringLiteral("iMessage");
        el.textColor = QStringLiteral("#8E8E93");
        el.fontSize = 14;
        el.height = 36;
        break;
    case UiElementType::Button:
        el.name = QStringLiteral("Button");
        el.fill = QStringLiteral("#0A84FF");
        el.cornerRadius = 8;
        el.text = QStringLiteral("Send");
        el.textColor = QStringLiteral("#FFFFFF");
        el.fontSize = 14;
        el.width = 60;
        el.height = 36;
        break;
    case UiElementType::Icon:
        el.name = QStringLiteral("Icon");
        el.textColor = QStringLiteral("#0A84FF");
        el.width = 24;
        el.height = 24;
        break;
    case UiElementType::Divider:
        el.name = QStringLiteral("Divider");
        el.lineColor = QStringLiteral("#38383A");
        el.lineWidth = 0.5;
        el.height = 1;
        break;
    }
    return el;
}

QString UiElement::typeName() const
{
    switch (type) {
    case UiElementType::Rectangle:    return QStringLiteral("Rectangle");
    case UiElementType::RoundedRect:  return QStringLiteral("Rounded Rect");
    case UiElementType::Circle:       return QStringLiteral("Circle");
    case UiElementType::Line:         return QStringLiteral("Line");
    case UiElementType::Image:        return QStringLiteral("Image");
    case UiElementType::Text:         return QStringLiteral("Text");
    case UiElementType::MessageBubble:return QStringLiteral("Message Bubble");
    case UiElementType::Avatar:       return QStringLiteral("Avatar");
    case UiElementType::SenderName:   return QStringLiteral("Sender Name");
    case UiElementType::Timestamp:    return QStringLiteral("Timestamp");
    case UiElementType::Composer:     return QStringLiteral("Composer");
    case UiElementType::InputField:   return QStringLiteral("Input Field");
    case UiElementType::Button:       return QStringLiteral("Button");
    case UiElementType::Icon:         return QStringLiteral("Icon");
    case UiElementType::Divider:      return QStringLiteral("Divider");
    }
    return QStringLiteral("Unknown");
}

UiElementType UiElement::typeFromName(const QString &name)
{
    if (name == QStringLiteral("Rectangle"))    return UiElementType::Rectangle;
    if (name == QStringLiteral("Rounded Rect")) return UiElementType::RoundedRect;
    if (name == QStringLiteral("Circle"))       return UiElementType::Circle;
    if (name == QStringLiteral("Line"))         return UiElementType::Line;
    if (name == QStringLiteral("Image"))        return UiElementType::Image;
    if (name == QStringLiteral("Text"))         return UiElementType::Text;
    if (name == QStringLiteral("Message Bubble"))return UiElementType::MessageBubble;
    if (name == QStringLiteral("Avatar"))       return UiElementType::Avatar;
    if (name == QStringLiteral("Sender Name"))  return UiElementType::SenderName;
    if (name == QStringLiteral("Timestamp"))    return UiElementType::Timestamp;
    if (name == QStringLiteral("Composer"))     return UiElementType::Composer;
    if (name == QStringLiteral("Input Field"))  return UiElementType::InputField;
    if (name == QStringLiteral("Button"))       return UiElementType::Button;
    if (name == QStringLiteral("Icon"))         return UiElementType::Icon;
    if (name == QStringLiteral("Divider"))      return UiElementType::Divider;
    return UiElementType::Rectangle;
}

UiElement *UiDesign::findElement(const QString &id)
{
    for (int i = 0; i < elements.size(); ++i)
        if (elements[i].id == id) return &elements[i];
    return nullptr;
}

const UiElement *UiDesign::findElement(const QString &id) const
{
    for (int i = 0; i < elements.size(); ++i)
        if (elements[i].id == id) return &elements[i];
    return nullptr;
}

void UiDesign::bringForward(const QString &id)
{
    for (int i = 0; i < elements.size() - 1; ++i) {
        if (elements[i].id == id) {
            elements[i].zValue = nextZ++;
            return;
        }
    }
}

void UiDesign::sendBackward(const QString &id)
{
    for (int i = 1; i < elements.size(); ++i) {
        if (elements[i].id == id) {
            elements[i].zValue = nextZ++;
            return;
        }
    }
}

void UiDesign::bringToFront(const QString &id)
{
    int maxZ = -1000000;
    for (const auto &el : elements)
        if (el.zValue > maxZ) maxZ = el.zValue;
    UiElement *el = findElement(id);
    if (el) el->zValue = maxZ + 1;
}

void UiDesign::sendToBack(const QString &id)
{
    int minZ = 1000000;
    for (const auto &el : elements)
        if (el.zValue < minZ) minZ = el.zValue;
    UiElement *el = findElement(id);
    if (el) el->zValue = minZ - 1;
}

void UiDesign::removeElement(const QString &id)
{
    for (int i = elements.size() - 1; i >= 0; --i) {
        if (elements[i].id == id) {
            elements.removeAt(i);
            return;
        }
    }
}

static QString typeStr(UiElementType t)
{
    switch (t) {
    case UiElementType::Rectangle:     return QStringLiteral("rectangle");
    case UiElementType::RoundedRect:   return QStringLiteral("roundedRect");
    case UiElementType::Circle:        return QStringLiteral("circle");
    case UiElementType::Line:          return QStringLiteral("line");
    case UiElementType::Image:         return QStringLiteral("image");
    case UiElementType::Text:          return QStringLiteral("text");
    case UiElementType::MessageBubble: return QStringLiteral("messageBubble");
    case UiElementType::Avatar:        return QStringLiteral("avatar");
    case UiElementType::SenderName:    return QStringLiteral("senderName");
    case UiElementType::Timestamp:     return QStringLiteral("timestamp");
    case UiElementType::Composer:      return QStringLiteral("composer");
    case UiElementType::InputField:    return QStringLiteral("inputField");
    case UiElementType::Button:        return QStringLiteral("button");
    case UiElementType::Icon:          return QStringLiteral("icon");
    case UiElementType::Divider:       return QStringLiteral("divider");
    }
    return QStringLiteral("rectangle");
}

static UiElementType parseType(const QString &s)
{
    if (s == QStringLiteral("rectangle"))     return UiElementType::Rectangle;
    if (s == QStringLiteral("roundedRect"))   return UiElementType::RoundedRect;
    if (s == QStringLiteral("circle"))        return UiElementType::Circle;
    if (s == QStringLiteral("line"))          return UiElementType::Line;
    if (s == QStringLiteral("image"))         return UiElementType::Image;
    if (s == QStringLiteral("text"))          return UiElementType::Text;
    if (s == QStringLiteral("messageBubble")) return UiElementType::MessageBubble;
    if (s == QStringLiteral("avatar"))        return UiElementType::Avatar;
    if (s == QStringLiteral("senderName"))    return UiElementType::SenderName;
    if (s == QStringLiteral("timestamp"))     return UiElementType::Timestamp;
    if (s == QStringLiteral("composer"))      return UiElementType::Composer;
    if (s == QStringLiteral("inputField"))    return UiElementType::InputField;
    if (s == QStringLiteral("button"))        return UiElementType::Button;
    if (s == QStringLiteral("icon"))          return UiElementType::Icon;
    if (s == QStringLiteral("divider"))       return UiElementType::Divider;
    return UiElementType::Rectangle;
}

QJsonObject UiDesign::serialize() const
{
    QJsonObject root;
    root[QStringLiteral("nextZ")] = nextZ;
    QJsonArray arr;
    for (const UiElement &el : elements) {
        QJsonObject o;
        o[QStringLiteral("id")] = el.id;
        o[QStringLiteral("name")] = el.name;
        o[QStringLiteral("type")] = typeStr(el.type);
        o[QStringLiteral("x")] = el.x;
        o[QStringLiteral("y")] = el.y;
        o[QStringLiteral("width")] = el.width;
        o[QStringLiteral("height")] = el.height;
        o[QStringLiteral("rotation")] = el.rotation;
        o[QStringLiteral("zValue")] = el.zValue;
        o[QStringLiteral("opacity")] = el.opacity;
        o[QStringLiteral("visible")] = el.visible;
        o[QStringLiteral("locked")] = el.locked;
        if (!el.fill.isEmpty()) o[QStringLiteral("fill")] = el.fill;
        if (!el.stroke.isEmpty()) o[QStringLiteral("stroke")] = el.stroke;
        if (el.strokeWidth > 0) o[QStringLiteral("strokeWidth")] = el.strokeWidth;
        if (el.cornerRadius > 0) o[QStringLiteral("cornerRadius")] = el.cornerRadius;
        if (el.cornerRadiusTL >= 0) o[QStringLiteral("cornerRadiusTL")] = el.cornerRadiusTL;
        if (el.cornerRadiusTR >= 0) o[QStringLiteral("cornerRadiusTR")] = el.cornerRadiusTR;
        if (el.cornerRadiusBL >= 0) o[QStringLiteral("cornerRadiusBL")] = el.cornerRadiusBL;
        if (el.cornerRadiusBR >= 0) o[QStringLiteral("cornerRadiusBR")] = el.cornerRadiusBR;
        if (!el.text.isEmpty()) o[QStringLiteral("text")] = el.text;
        if (!el.fontFamily.isEmpty()) o[QStringLiteral("fontFamily")] = el.fontFamily;
        if (el.fontSize != 14) o[QStringLiteral("fontSize")] = el.fontSize;
        if (el.fontWeight != 400) o[QStringLiteral("fontWeight")] = el.fontWeight;
        if (el.fontItalic) o[QStringLiteral("fontItalic")] = el.fontItalic;
        if (el.textAlign != 0) o[QStringLiteral("textAlign")] = el.textAlign;
        if (el.lineHeight != 1.0) o[QStringLiteral("lineHeight")] = el.lineHeight;
        if (!el.textColor.isEmpty()) o[QStringLiteral("textColor")] = el.textColor;
        if (!el.imageRef.isEmpty()) o[QStringLiteral("imageRef")] = el.imageRef;
        if (el.imageScaling != 0) o[QStringLiteral("imageScaling")] = el.imageScaling;
        if (el.bubbleOutgoing) o[QStringLiteral("bubbleOutgoing")] = el.bubbleOutgoing;
        if (!el.bubbleColor.isEmpty()) o[QStringLiteral("bubbleColor")] = el.bubbleColor;
        if (el.bubbleCornerRadius != 18) o[QStringLiteral("bubbleCornerRadius")] = el.bubbleCornerRadius;
        if (el.groupedCornerRadius != 6) o[QStringLiteral("groupedCornerRadius")] = el.groupedCornerRadius;
        if (!el.tailEnabled) o[QStringLiteral("tailEnabled")] = el.tailEnabled;
        if (el.tailWidth != 6) o[QStringLiteral("tailWidth")] = el.tailWidth;
        if (el.tailHeight != 4) o[QStringLiteral("tailHeight")] = el.tailHeight;
        if (el.bubblePaddingH != 12) o[QStringLiteral("bubblePaddingH")] = el.bubblePaddingH;
        if (el.bubblePaddingV != 7) o[QStringLiteral("bubblePaddingV")] = el.bubblePaddingV;
        if (!el.iconType.isEmpty()) o[QStringLiteral("iconType")] = el.iconType;
        if (!el.buttonLabel.isEmpty()) o[QStringLiteral("buttonLabel")] = el.buttonLabel;
        if (!el.lineColor.isEmpty()) o[QStringLiteral("lineColor")] = el.lineColor;
        if (el.lineWidth != 1) o[QStringLiteral("lineWidth")] = el.lineWidth;
        if (!el.refMessageId.isEmpty()) o[QStringLiteral("refMessageId")] = el.refMessageId;
        if (!el.refCharacterId.isEmpty()) o[QStringLiteral("refCharacterId")] = el.refCharacterId;
        if (!el.refConversationId.isEmpty()) o[QStringLiteral("refConversationId")] = el.refConversationId;
        arr.append(o);
    }
    root[QStringLiteral("elements")] = arr;
    return root;
}

UiDesign UiDesign::deserialize(const QJsonObject &obj)
{
    UiDesign design;
    design.nextZ = obj.value(QStringLiteral("nextZ")).toInt(0);
    const QJsonArray arr = obj.value(QStringLiteral("elements")).toArray();
    for (const QJsonValue &v : arr) {
        const QJsonObject o = v.toObject();
        UiElement el;
        el.id = o.value(QStringLiteral("id")).toString();
        el.name = o.value(QStringLiteral("name")).toString();
        el.type = parseType(o.value(QStringLiteral("type")).toString());
        el.x = o.value(QStringLiteral("x")).toDouble();
        el.y = o.value(QStringLiteral("y")).toDouble();
        el.width = o.value(QStringLiteral("width")).toDouble(100);
        el.height = o.value(QStringLiteral("height")).toDouble(100);
        el.rotation = o.value(QStringLiteral("rotation")).toDouble();
        el.zValue = o.value(QStringLiteral("zValue")).toInt();
        el.opacity = o.value(QStringLiteral("opacity")).toDouble(1.0);
        el.visible = o.value(QStringLiteral("visible")).toBool(true);
        el.locked = o.value(QStringLiteral("locked")).toBool(false);
        el.fill = o.value(QStringLiteral("fill")).toString();
        el.stroke = o.value(QStringLiteral("stroke")).toString();
        el.strokeWidth = o.value(QStringLiteral("strokeWidth")).toDouble();
        el.cornerRadius = o.value(QStringLiteral("cornerRadius")).toDouble();
        el.cornerRadiusTL = o.value(QStringLiteral("cornerRadiusTL")).toDouble(-1);
        el.cornerRadiusTR = o.value(QStringLiteral("cornerRadiusTR")).toDouble(-1);
        el.cornerRadiusBL = o.value(QStringLiteral("cornerRadiusBL")).toDouble(-1);
        el.cornerRadiusBR = o.value(QStringLiteral("cornerRadiusBR")).toDouble(-1);
        el.text = o.value(QStringLiteral("text")).toString();
        el.fontFamily = o.value(QStringLiteral("fontFamily")).toString();
        el.fontSize = o.value(QStringLiteral("fontSize")).toInt(14);
        el.fontWeight = o.value(QStringLiteral("fontWeight")).toInt(400);
        el.fontItalic = o.value(QStringLiteral("fontItalic")).toBool();
        el.textAlign = o.value(QStringLiteral("textAlign")).toInt();
        el.lineHeight = o.value(QStringLiteral("lineHeight")).toDouble(1.0);
        el.textColor = o.value(QStringLiteral("textColor")).toString();
        el.imageRef = o.value(QStringLiteral("imageRef")).toString();
        el.imageScaling = o.value(QStringLiteral("imageScaling")).toInt();
        el.bubbleOutgoing = o.value(QStringLiteral("bubbleOutgoing")).toBool();
        el.bubbleColor = o.value(QStringLiteral("bubbleColor")).toString();
        el.bubbleCornerRadius = o.value(QStringLiteral("bubbleCornerRadius")).toDouble(18);
        el.groupedCornerRadius = o.value(QStringLiteral("groupedCornerRadius")).toDouble(6);
        el.tailEnabled = o.value(QStringLiteral("tailEnabled")).toBool(true);
        el.tailWidth = o.value(QStringLiteral("tailWidth")).toDouble(6);
        el.tailHeight = o.value(QStringLiteral("tailHeight")).toDouble(4);
        el.bubblePaddingH = o.value(QStringLiteral("bubblePaddingH")).toDouble(12);
        el.bubblePaddingV = o.value(QStringLiteral("bubblePaddingV")).toDouble(7);
        el.iconType = o.value(QStringLiteral("iconType")).toString();
        el.buttonLabel = o.value(QStringLiteral("buttonLabel")).toString();
        el.lineColor = o.value(QStringLiteral("lineColor")).toString();
        el.lineWidth = o.value(QStringLiteral("lineWidth")).toDouble(1);
        el.refMessageId = o.value(QStringLiteral("refMessageId")).toString();
        el.refCharacterId = o.value(QStringLiteral("refCharacterId")).toString();
        el.refConversationId = o.value(QStringLiteral("refConversationId")).toString();
        if (!el.id.isEmpty())
            design.elements.append(el);
    }
    if (design.nextZ == 0 && !design.elements.isEmpty()) {
        int maxZ = 0;
        for (const auto &el : design.elements)
            if (el.zValue > maxZ) maxZ = el.zValue;
        design.nextZ = maxZ + 1;
    }
    return design;
}

} // namespace Core
} // namespace Konata
