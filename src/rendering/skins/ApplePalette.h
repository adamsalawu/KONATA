#pragma once

#include <QString>

// Apple Messages palette (0.2C.2): centralized target colors. Values marked
// [V] are publicly documented Apple system colors; [A] marks reasoned
// choices called out as unverified in the fidelity audit. Dark is the
// verified target; Light exists so colors are never hardwired into
// components — it is architecture, audited as PARTIAL.
struct ApplePalette {
    QString background;
    QString secondaryBackground;
    QString primaryText;
    QString secondaryText;
    QString tertiaryText;
    QString incomingBubble;
    QString incomingText;
    QString outgoingBubble;
    QString outgoingText;
    QString accent;
    QString separators;
    QString composerBackground;
    QString composerField;
    QString icon;
    QString iconDisabled;

    static ApplePalette dark();
    static ApplePalette light();
};

inline ApplePalette ApplePalette::dark()
{
    ApplePalette palette;
    palette.background = QStringLiteral("#000000"); // [V] dark base
    palette.secondaryBackground = QStringLiteral("#1C1C1E"); // [V] dark surface
    palette.primaryText = QStringLiteral("#FFFFFF"); // [V]
    palette.secondaryText = QStringLiteral("#8E8E93"); // [V] secondary label
    palette.tertiaryText = QStringLiteral("#48484A"); // [A]
    palette.incomingBubble = QStringLiteral("#2C2C2E"); // [V] dark received
    palette.incomingText = QStringLiteral("#FFFFFF"); // [V]
    palette.outgoingBubble = QStringLiteral("#0A84FF"); // [V] systemBlue dark
    palette.outgoingText = QStringLiteral("#FFFFFF"); // [V]
    palette.accent = QStringLiteral("#0A84FF"); // [V]
    palette.separators = QStringLiteral("#38383A"); // [V] dark separator
    palette.composerBackground = QStringLiteral("#000000"); // [A]
    palette.composerField = QStringLiteral("#1C1C1E"); // [A]
    palette.icon = QStringLiteral("#0A84FF"); // [A] action icons
    palette.iconDisabled = QStringLiteral("#3A3A3C"); // [A]
    return palette;
}

inline ApplePalette ApplePalette::light()
{
    ApplePalette palette;
    palette.background = QStringLiteral("#FFFFFF"); // [V]
    palette.secondaryBackground = QStringLiteral("#F2F2F7"); // [V] systemGray6
    palette.primaryText = QStringLiteral("#000000"); // [V]
    palette.secondaryText = QStringLiteral("#8E8E93"); // [A] approx secondary
    palette.tertiaryText = QStringLiteral("#C7C7CC"); // [A]
    palette.incomingBubble = QStringLiteral("#E5E5EB"); // [R] sampled private light — was #E9E9EB (JPEG AA bias)
    palette.incomingText = QStringLiteral("#000000"); // [V]
    palette.outgoingBubble = QStringLiteral("#007AFF"); // [V] systemBlue
    palette.outgoingText = QStringLiteral("#FFFFFF"); // [V]
    palette.accent = QStringLiteral("#007AFF"); // [V]
    palette.separators = QStringLiteral("#C6C6C8"); // [V] light separator
    palette.composerBackground = QStringLiteral("#F2F2F7"); // [A]
    palette.composerField = QStringLiteral("#FFFFFF"); // [A]
    palette.icon = QStringLiteral("#007AFF"); // [A]
    palette.iconDisabled = QStringLiteral("#C7C7CC"); // [A]
    return palette;
}
