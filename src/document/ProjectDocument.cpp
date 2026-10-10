#include "document/ProjectDocument.h"

using namespace Konata;
using namespace Konata::Core;

ProjectDocument::ProjectDocument(QObject *parent)
    : QObject(parent)
{
    m_manager.newProject();
    m_dirtySignalled = false;
}

const Serialization::KonataProject &ProjectDocument::project() const
{
    return m_manager.project();
}

const Story &ProjectDocument::story() const
{
    return m_manager.project().story;
}

bool ProjectDocument::newProject(const QString &storyTitle, const QString &appName)
{
    const QString previousPath = m_manager.filePath();
    if (!m_manager.newProject(storyTitle, appName).ok)
        return false;
    clearSelection();
    emit projectChanged();
    emitFilePathIfChanged(previousPath);
    emitDirtyIfChanged();
    return true;
}

bool ProjectDocument::open(const QString &filePath)
{
    const QString previousPath = m_manager.filePath();
    if (!m_manager.open(filePath).ok)
        return false;
    clearSelection();
    emit projectChanged();
    emitFilePathIfChanged(previousPath);
    emitDirtyIfChanged();
    return true;
}

bool ProjectDocument::save()
{
    if (!m_manager.save().ok)
        return false;
    emitDirtyIfChanged();
    return true;
}

bool ProjectDocument::saveAs(const QString &filePath)
{
    const QString previousPath = m_manager.filePath();
    if (!m_manager.saveAs(filePath).ok)
        return false;
    emitFilePathIfChanged(previousPath);
    emitDirtyIfChanged();
    return true;
}

bool ProjectDocument::isDirty() const
{
    return m_manager.isDirty();
}

bool ProjectDocument::hasFile() const
{
    return m_manager.hasFile();
}

QString ProjectDocument::filePath() const
{
    return m_manager.filePath();
}

QString ProjectDocument::lastError() const
{
    return m_manager.lastError();
}

Character ProjectDocument::addCharacter(const QString &displayName, const QString &username)
{
    Character c = Character::create(displayName, username);
    m_manager.mutableProject().story.addCharacter(c);
    m_manager.markDirty();
    emit projectChanged();
    emitDirtyIfChanged();
    return c;
}

bool ProjectDocument::updateCharacter(const Character &character)
{
    if (!m_manager.mutableProject().story.updateCharacter(character))
        return false;
    m_manager.markDirty();
    emit projectChanged();
    emitDirtyIfChanged();
    return true;
}

bool ProjectDocument::removeCharacter(const QString &characterId)
{
    if (!m_manager.mutableProject().story.removeCharacter(characterId))
        return false;
    clearSelectionIfMatches(SelectionType::Character, characterId);
    m_manager.markDirty();
    emit projectChanged();
    emitDirtyIfChanged();
    return true;
}

Conversation ProjectDocument::addConversation(const QString &title, ConversationType type)
{
    Conversation c = Conversation::create(title, type);
    m_manager.mutableProject().story.addConversation(c);
    m_manager.markDirty();
    emit projectChanged();
    emitDirtyIfChanged();
    return c;
}

bool ProjectDocument::updateConversation(const Conversation &conversation)
{
    if (!m_manager.mutableProject().story.updateConversation(conversation))
        return false;
    m_manager.markDirty();
    emit projectChanged();
    emitDirtyIfChanged();
    return true;
}

bool ProjectDocument::removeConversation(const QString &conversationId)
{
    if (!m_manager.mutableProject().story.removeConversation(conversationId))
        return false;
    clearSelectionIfMatches(SelectionType::Conversation, conversationId);
    clearNestedSelections(conversationId);
    m_manager.markDirty();
    emit projectChanged();
    emitDirtyIfChanged();
    return true;
}

Scene ProjectDocument::addScene(const QString &name)
{
    Scene s = Scene::create(name);
    m_manager.mutableProject().story.addScene(s);
    m_manager.markDirty();
    emit projectChanged();
    emitDirtyIfChanged();
    return s;
}

bool ProjectDocument::updateScene(const Scene &scene)
{
    if (!m_manager.mutableProject().story.updateScene(scene))
        return false;
    m_manager.markDirty();
    emit projectChanged();
    emitDirtyIfChanged();
    return true;
}

bool ProjectDocument::removeScene(const QString &sceneId)
{
    if (!m_manager.mutableProject().story.removeScene(sceneId))
        return false;
    clearSelectionIfMatches(SelectionType::Scene, sceneId);
    clearNestedSelections(sceneId);
    m_manager.markDirty();
    emit projectChanged();
    emitDirtyIfChanged();
    return true;
}

Message ProjectDocument::createMessage(const QString &conversationId, const QString &authorId,
                                       MessageType type)
{
    Message created;
    Story &story = m_manager.mutableProject().story;
    const Conversation *found = story.findConversation(conversationId);
    if (!found)
        return created; // unknown conversation: empty message, no mutation
    Conversation copy = *found;
    QString author = authorId;
    if (author.isEmpty() && !copy.participantIds().isEmpty())
        author = copy.participantIds().first();
    Message draft;
    switch (type) {
    case MessageType::Image:
    case MessageType::File:
        draft = Message::createMedia(author, type);
        break;
    case MessageType::System:
        draft = Message::createSystem(QString());
        break;
    case MessageType::Text:
        draft = Message::createText(author, QString());
        break;
    }
    copy.addMessage(draft); // stamps conversationId + order
    if (!story.updateConversation(copy))
        return Message();
    created = copy.messages().last();
    m_manager.markDirty();
    emit projectChanged();
    emitDirtyIfChanged();
    return created;
}

bool ProjectDocument::updateMessage(const QString &conversationId, const Message &message)
{
    Story &story = m_manager.mutableProject().story;
    const Conversation *found = story.findConversation(conversationId);
    if (!found)
        return false;
    Conversation copy = *found;
    if (!copy.updateMessage(message))
        return false;
    story.updateConversation(copy);
    m_manager.markDirty();
    emit projectChanged();
    emitDirtyIfChanged();
    return true;
}

bool ProjectDocument::removeMessage(const QString &conversationId, const QString &messageId)
{
    Story &story = m_manager.mutableProject().story;
    const Conversation *found = story.findConversation(conversationId);
    if (!found)
        return false;
    Conversation copy = *found;
    if (!copy.removeMessage(messageId))
        return false;
    story.updateConversation(copy);
    if (m_selection.type == SelectionType::Message && m_selection.id == messageId
        && m_selection.parentId == conversationId)
        clearSelection();
    m_manager.markDirty();
    emit projectChanged();
    emitDirtyIfChanged();
    return true;
}

bool ProjectDocument::moveMessage(const QString &conversationId, const QString &messageId,
                                  int delta)
{
    Story &story = m_manager.mutableProject().story;
    const Conversation *found = story.findConversation(conversationId);
    if (!found)
        return false;
    Conversation copy = *found;
    if (!copy.moveMessage(messageId, delta))
        return false;
    story.updateConversation(copy);
    m_manager.markDirty();
    emit projectChanged();
    emitDirtyIfChanged();
    return true;
}

Message ProjectDocument::duplicateMessage(const QString &conversationId,
                                          const QString &messageId)
{
    Message created;
    Story &story = m_manager.mutableProject().story;
    const Conversation *found = story.findConversation(conversationId);
    if (!found)
        return created;
    Conversation copy = *found;
    created = copy.duplicateMessage(messageId);
    if (created.id.isEmpty())
        return created;
    story.updateConversation(copy);
    m_manager.markDirty();
    emit projectChanged();
    emitDirtyIfChanged();
    return created;
}

SceneEvent ProjectDocument::createEvent(const QString &sceneId, SceneEventType type)
{
    SceneEvent created;
    Story &story = m_manager.mutableProject().story;
    const Scene *found = story.findScene(sceneId);
    if (!found)
        return created;
    Scene copy = *found;
    switch (type) {
    case SceneEventType::SendMessage:
        created = SceneEvent::sendMessage(QString(), QString());
        break;
    case SceneEventType::Wait:
        created = SceneEvent::wait(1000);
        break;
    case SceneEventType::StartTyping:
        created = SceneEvent::typing(QString(), QString(), true);
        break;
    case SceneEventType::StopTyping:
        created = SceneEvent::typing(QString(), QString(), false);
        break;
    case SceneEventType::Notification:
        created = SceneEvent::notification(QString());
        break;
    }
    copy.addEvent(created);
    if (!story.updateScene(copy))
        return SceneEvent();
    m_manager.markDirty();
    emit projectChanged();
    emitDirtyIfChanged();
    return created;
}

bool ProjectDocument::updateEvent(const QString &sceneId, const SceneEvent &event)
{
    Story &story = m_manager.mutableProject().story;
    const Scene *found = story.findScene(sceneId);
    if (!found)
        return false;
    Scene copy = *found;
    if (!copy.updateEvent(event))
        return false;
    story.updateScene(copy);
    m_manager.markDirty();
    emit projectChanged();
    emitDirtyIfChanged();
    return true;
}

bool ProjectDocument::removeEvent(const QString &sceneId, const QString &eventId)
{
    Story &story = m_manager.mutableProject().story;
    const Scene *found = story.findScene(sceneId);
    if (!found)
        return false;
    Scene copy = *found;
    if (!copy.removeEvent(eventId))
        return false;
    story.updateScene(copy);
    if (m_selection.type == SelectionType::SceneEvent && m_selection.id == eventId
        && m_selection.parentId == sceneId)
        clearSelection();
    m_manager.markDirty();
    emit projectChanged();
    emitDirtyIfChanged();
    return true;
}

void ProjectDocument::applyAppDefinition(const Core::AppDefinition &app)
{
    m_manager.mutableProject().appDefinition = app;
    m_manager.markDirty();
    emit projectChanged();
    emitDirtyIfChanged();
}

Core::CanvasSettings ProjectDocument::canvas() const
{
    return m_manager.project().canvas;
}

bool ProjectDocument::setCanvas(const Core::CanvasSettings &canvas)
{
    if (!canvas.isValid())
        return false;
    const Core::CanvasSettings current = m_manager.project().canvas;
    if (current.preset == canvas.preset && current.customWidth == canvas.customWidth
        && current.customHeight == canvas.customHeight)
        return true;
    m_manager.mutableProject().canvas = canvas;
    m_manager.markDirty();
    emit projectChanged();
    emit canvasChanged(canvas);
    emitDirtyIfChanged();
    return true;
}

const Core::UiDesign &ProjectDocument::uiDesign() const
{
    return m_manager.project().uiDesign;
}

void ProjectDocument::setUiDesign(const Core::UiDesign &design)
{
    m_manager.mutableProject().uiDesign = design;
    m_manager.markDirty();
    emit projectChanged();
    emitDirtyIfChanged();
}

void ProjectDocument::setSelection(SelectionType type, const QString &id,
                                   const QString &parentId)
{
    if (m_selection.type == type && m_selection.id == id && m_selection.parentId == parentId)
        return;
    m_selection.type = type;
    m_selection.id = id;
    m_selection.parentId = parentId;
    emit selectionChanged();
}

void ProjectDocument::clearSelection()
{
    if (m_selection.type == SelectionType::None)
        return;
    m_selection.type = SelectionType::None;
    m_selection.id.clear();
    m_selection.parentId.clear();
    emit selectionChanged();
}

void ProjectDocument::emitDirtyIfChanged()
{
    const bool dirty = m_manager.isDirty();
    if (dirty != m_dirtySignalled) {
        m_dirtySignalled = dirty;
        emit dirtyChanged(dirty);
    }
}

void ProjectDocument::emitFilePathIfChanged(const QString &previous)
{
    if (m_manager.filePath() != previous)
        emit filePathChanged(m_manager.filePath());
}

void ProjectDocument::clearSelectionIfMatches(SelectionType type, const QString &id)
{
    if (m_selection.type == type && m_selection.id == id)
        clearSelection();
}

void ProjectDocument::clearNestedSelections(const QString &parentId)
{
    if (!m_selection.parentId.isEmpty() && m_selection.parentId == parentId)
        clearSelection();
}
