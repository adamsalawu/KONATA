#pragma once

#include <QObject>
#include <QString>

#include "core/CanvasSettings.h"
#include "core/Character.h"
#include "core/Conversation.h"
#include "core/Scene.h"
#include "core/Story.h"
#include "serialization/KonataProject.h"
#include "serialization/ProjectManager.h"

// UI-facing document bridge. Owns a Serialization::ProjectManager and is the
// single place where UI code mutates project data:
//
//   UI widgets -> ProjectDocument -> ProjectManager -> KonataProject -> Core
//
// Panels and editors hold stable IDs only, never Core pointers. Every
// mutation marks the project dirty and emits projectChanged(). QtCore-only:
// no QWidget dependency, so document logic stays testable without a GUI.
class ProjectDocument : public QObject
{
    Q_OBJECT

public:
    enum class SelectionType {
        None,
        Character,
        Conversation,
        Message,
        Scene,
        SceneEvent
    };

    // Nested objects (Message, SceneEvent) carry their parent ID so the
    // selection uniquely identifies them. All IDs, never pointers.
    struct Selection {
        SelectionType type = SelectionType::None;
        QString id;
        QString parentId;
    };

    explicit ProjectDocument(QObject *parent = nullptr);

    // --- Project access (read-only; mutate through CRUD below) ---
    const Konata::Serialization::KonataProject &project() const;
    const Konata::Core::Story &story() const;

    // --- File workflow (forwarded to ProjectManager) ---
    bool newProject(const QString &storyTitle = QStringLiteral("Untitled Story"),
                    const QString &appName = QStringLiteral("Konata Chat"));
    bool open(const QString &filePath);
    bool save();
    bool saveAs(const QString &filePath);

    bool isDirty() const;
    bool hasFile() const;
    QString filePath() const;
    QString lastError() const;

    // --- Authoring CRUD (create real Core objects with real IDs) ---
    Konata::Core::Character addCharacter(
        const QString &displayName = QStringLiteral("New Character"),
        const QString &username = QStringLiteral("new_character"));
    bool updateCharacter(const Konata::Core::Character &character);
    bool removeCharacter(const QString &characterId);

    Konata::Core::Conversation addConversation(
        const QString &title = QStringLiteral("New Conversation"),
        Konata::Core::ConversationType type = Konata::Core::ConversationType::Private);
    bool updateConversation(const Konata::Core::Conversation &conversation);
    bool removeConversation(const QString &conversationId);

    Konata::Core::Scene addScene(const QString &name = QStringLiteral("New Scene"));
    bool updateScene(const Konata::Core::Scene &scene);
    bool removeScene(const QString &sceneId);

    // --- Message authoring (nested in Conversation, 0.1E) ---
    // New messages default to Text, authored by the first participant when
    // one exists; ordering/conversationId stamping reuses addMessage().
    Konata::Core::Message createMessage(
        const QString &conversationId,
        const QString &authorId = QString(),
        Konata::Core::MessageType type = Konata::Core::MessageType::Text);
    bool updateMessage(const QString &conversationId, const Konata::Core::Message &message);
    bool removeMessage(const QString &conversationId, const QString &messageId);
    // Direct chat editing: move swaps adjacent messages, duplicate inserts a
    // fresh-ID copy after the original. Same dirty/signal contract as above.
    bool moveMessage(const QString &conversationId, const QString &messageId, int delta);
    Konata::Core::Message duplicateMessage(const QString &conversationId,
                                           const QString &messageId);

    // --- SceneEvent authoring (nested in Scene, 0.1E) ---
    Konata::Core::SceneEvent createEvent(
        const QString &sceneId,
        Konata::Core::SceneEventType type = Konata::Core::SceneEventType::SendMessage);
    bool updateEvent(const QString &sceneId, const Konata::Core::SceneEvent &event);
    bool removeEvent(const QString &sceneId, const QString &eventId);

    // --- App skin (0.2C.1): replaces the whole AppDefinition, e.g. with a
    // skin factory's output. Normal dirty-marking mutation; persists via
    // the existing serialization. Story data is untouched.
    void applyAppDefinition(const Konata::Core::AppDefinition &app);

    // --- Canvas (presentation, not Core) ---
    Konata::Core::CanvasSettings canvas() const;
    bool setCanvas(const Konata::Core::CanvasSettings &canvas);

    // --- Developer Mode UI design (presentation, not Core) ---
    const Konata::Core::UiDesign &uiDesign() const;
    void setUiDesign(const Konata::Core::UiDesign &design);

    // --- Selection (stable IDs only) ---
    Selection selection() const { return m_selection; }
    void setSelection(SelectionType type, const QString &id,
                      const QString &parentId = QString());
    void clearSelection();

signals:
    void projectChanged();
    void selectionChanged();
    void dirtyChanged(bool dirty);
    void filePathChanged(const QString &filePath);
    void canvasChanged(const Konata::Core::CanvasSettings &canvas);

private:
    void emitDirtyIfChanged();
    void emitFilePathIfChanged(const QString &previous);
    void clearSelectionIfMatches(SelectionType type, const QString &id);
    void clearNestedSelections(const QString &parentId);

    Konata::Serialization::ProjectManager m_manager;
    Selection m_selection;
    bool m_dirtySignalled = false;
};
