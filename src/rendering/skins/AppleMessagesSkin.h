#pragma once

#include "core/AppDefinition.h"

// Apple Messages skin (0.2C.1): the first real app configuration built purely
// from the generic App UI vocabulary. A deterministic factory — no branches
// on application names anywhere; the generic renderer consumes the resulting
// AppDefinition like any other. Core/Story types are untouched.
class AppleMessagesSkin
{
public:
    // Builds a complete Apple Messages AppDefinition: Stack navigation,
    // header/avatar/back affordances, iMessage composer, centralized palette,
    // iOS type scale. Dark is the verified target; Light is provided so no
    // component hardwires one appearance.
    static Konata::Core::AppDefinition createAppDefinition(
        Konata::Core::AppearanceMode mode = Konata::Core::AppearanceMode::Dark);

    // Neutral Konata skin: restores the stock generic presentation.
    static Konata::Core::AppDefinition createNeutralAppDefinition(
        const QString &appName = QStringLiteral("Konata Chat"));

private:
    AppleMessagesSkin() = delete;
};
