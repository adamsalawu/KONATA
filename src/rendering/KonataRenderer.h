#pragma once

#include <QString>

#include "core/AppDefinition.h"
#include "core/Story.h"
#include "rendering/RenderModel.h"

// First real KONATA renderer (0.2A). Consumes structured Core data and
// produces a RenderModel describing the visual representation.
//
// Boundaries (deliberate):
// - read-only: takes `const Story&`, never mutates, never marks dirty
// - no second data model: references Core IDs, resolves names for display
// - no authoring: no mutation APIs, no ProjectDocument access
// - deterministic: same (Story, conversationId) always yields the same model
class KonataRenderer
{
public:
    // Renders the conversation's messages in Core order. Unknown conversation
    // IDs yield `valid == false` rather than invented content. Unknown authors
    // yield fallback names, never crashes or data repair. Presentation fields
    // resolve from a neutral AppDefinition; pass an explicit definition to
    // render through a skin.
    static RenderedConversation renderConversation(const Konata::Core::Story &story,
                                                   const QString &conversationId);
    static RenderedConversation renderConversation(const Konata::Core::Story &story,
                                                   const Konata::Core::AppDefinition &app,
                                                   const QString &conversationId);

    // Resolves a Character ID to a display name. System/empty authors and
    // unresolvable IDs produce safe fallbacks; `missing` reports whether the
    // ID could be resolved (null when the caller does not care).
    static QString authorDisplayName(const Konata::Core::Story &story,
                                     const Konata::Core::Message &message,
                                     bool *missing = nullptr);

    // Conversation-list screen model: one row per conversation in Story
    // order, latest-message preview + timestamp. Never invents conversations.
    static RenderedConversationList renderConversationList(
        const Konata::Core::Story &story, const Konata::Core::AppDefinition &app);

    // Contact/profile screen model for one Character. Action labels are
    // structural only — no behavior, no contact management.
    static RenderedProfile renderProfile(const Konata::Core::Story &story,
                                         const QString &characterId);

    // Static metadata formatting from Core timestamps (no behavior).
    static QString shortTime(const QDateTime &when);
    static QString dayLabel(const QDateTime &when);

    // Generic App UI entry point (0.2B). Resolves Story + AppDefinition into
    // a presentation model: Chat screen with conversation payload when the
    // conversation resolves, otherwise the ConversationList screen. Screen
    // choice and components derive from structure + payload only — never
    // from application names. Pure and read-only like renderConversation.
    static RenderedAppView renderAppView(const Konata::Core::Story &story,
                                         const Konata::Core::AppDefinition &app,
                                         const QString &conversationId);

private:
    KonataRenderer() = delete;

    static void resolveGrouping(const QList<Konata::Core::Message> &messages,
                                const Konata::Core::AppDefinition &app,
                                QList<RenderedMessage> &rows);
    static void resolveDaySeparators(const QList<Konata::Core::Message> &messages,
                                     QList<RenderedMessage> &rows);
};
