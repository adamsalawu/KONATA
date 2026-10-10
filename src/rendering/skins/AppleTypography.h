#pragma once

#include <QFont>
#include <QStringList>

// Apple Messages typography (0.2C.3): measured from references.
// Body 17pt is documented iOS scale [V]; the stack prefers SF Pro on
// Apple platforms and falls back deterministically elsewhere [A].
struct AppleTypography {
    static QStringList familyStack()
    {
        return QStringList{QStringLiteral("SF Pro Text"), QStringLiteral("SF Pro Display"),
                           QStringLiteral("SF Pro"), QStringLiteral("Segoe UI"),
                           QStringLiteral("sans-serif")};
    }

    static int navLargeTitleSize() { return 34; }
    static int titleSize() { return 17; } // [V] body-scale titles (matches A3 test)
    static int bodySize() { return 17; } // [V] bubble text
    static int secondarySize() { return 15; }
    static int senderNameSize() { return 13; } // [R] "Jackk;)" ~13
    static int metadataSize() { return 12; } // timestamps
    static int headerBadgeSize() { return 11; }

    static QFont::Weight titleWeight() { return QFont::DemiBold; }
    static QFont::Weight bodyWeight() { return QFont::Normal; }
    static QFont::Weight metadataWeight() { return QFont::Normal; }

    static QFont font(int pointSize, QFont::Weight weight = QFont::Normal)
    {
        QFont font(familyStack(), qMax(1, pointSize));
        font.setWeight(weight);
        return font;
    }
};
