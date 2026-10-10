cmake_minimum_required(VERSION 3.21)

project(KONATA VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Allow overriding from command line: -DCMAKE_PREFIX_PATH=...
list(APPEND CMAKE_PREFIX_PATH "C:/Aether/devtools/Qt/6.8.1/msvc2022_64")

find_package(Qt6 6.8 REQUIRED COMPONENTS Widgets Core)

# --- nlohmann/json 3.11.3: header-only, fetched at configure time, not vendored ---
include(FetchContent)
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")
FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(nlohmann_json)

# --- KonataCore: UI-independent data model (no Widgets) ---
set(KONATA_CORE_SOURCES
    src/core/Id.h
    src/core/Id.cpp
    src/core/Character.h
    src/core/Character.cpp
    src/core/Reaction.h
    src/core/Attachment.h
    src/core/Message.h
    src/core/Message.cpp
    src/core/Conversation.h
    src/core/Conversation.cpp
    src/core/SceneEvent.h
    src/core/Scene.h
    src/core/Scene.cpp
    src/core/Story.h
    src/core/Story.cpp
    src/core/AppUi.h
    src/core/AppDefinition.h
    src/core/AppDefinition.cpp
    src/core/CanvasSettings.h
    src/core/CanvasSettings.cpp
    src/core/UiDesign.h
    src/core/UiDesign.cpp
)

add_library(KonataCore STATIC ${KONATA_CORE_SOURCES})
target_include_directories(KonataCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(KonataCore PUBLIC Qt6::Core)

set(KONATA_SOURCES
    src/main.cpp
    src/document/ProjectDocument.h
    src/document/ProjectDocument.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/ProjectPanel.h
    src/ui/ProjectPanel.cpp
    src/ui/PreviewArea.h
    src/ui/PreviewArea.cpp
    src/ui/RoundedBubble.h
    src/ui/RoundedBubble.cpp
    src/ui/InspectorPanel.h
    src/ui/InspectorPanel.cpp
    src/ui/TimelinePanel.h
    src/ui/TimelinePanel.cpp
    src/ui/authoring/CharacterEditorWidget.h
    src/ui/authoring/CharacterEditorWidget.cpp
    src/ui/authoring/ConversationEditorWidget.h
    src/ui/authoring/ConversationEditorWidget.cpp
    src/ui/authoring/MessageEditorWidget.h
    src/ui/authoring/MessageEditorWidget.cpp
    src/ui/authoring/SceneEditorWidget.h
    src/ui/authoring/SceneEditorWidget.cpp
    src/ui/authoring/SceneEventEditorWidget.h
    src/ui/authoring/SceneEventEditorWidget.cpp
    src/ui/DeveloperMode.h
    src/ui/DeveloperMode.cpp
    src/ui/DeveloperCanvas.h
    src/ui/DeveloperCanvas.cpp
    src/ui/UiElementGraphicsItem.h
    src/ui/UiElementGraphicsItem.cpp
    resources/resources.qrc
)

if(COMMAND qt_add_executable)
    qt_add_executable(KONATA MANUAL_FINALIZATION ${KONATA_SOURCES})
else()
    add_executable(KONATA WIN32 ${KONATA_SOURCES})
endif()

target_include_directories(KONATA PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(KONATA PRIVATE Qt6::Widgets KonataCore KonataSerialization KonataRendering KonataExport)

set_target_properties(KONATA PROPERTIES
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME "KONATA"
)

if(COMMAND qt_finalize_executable)
    qt_finalize_executable(KONATA)
endif()

# --- Lightweight 0.1B verification, no test framework ---
add_executable(KonataCoreCheck tests/CoreSmokeCheck.cpp)
target_link_libraries(KonataCoreCheck PRIVATE KonataCore Qt6::Core)
set_target_properties(KonataCoreCheck PROPERTIES WIN32_EXECUTABLE FALSE)

# --- KonataRendering: read-only conversation renderer (icons need QtGui) ---
add_library(KonataRendering STATIC
    src/rendering/RenderModel.h
    src/rendering/KonataRenderer.h
    src/rendering/KonataRenderer.cpp
    src/rendering/CanvasLayout.h
    src/rendering/CanvasLayout.cpp
    src/rendering/CanvasChatPainter.h
    src/rendering/CanvasChatPainter.cpp
    src/rendering/skins/AppleMessagesSkin.h
    src/rendering/skins/AppleMessagesSkin.cpp
    src/rendering/skins/AppleGeometry.h
    src/rendering/skins/ApplePalette.h
    src/rendering/skins/AppleTypography.h
    src/rendering/skins/AppleIcons.h
    src/rendering/skins/AppleIcons.cpp)
target_include_directories(KonataRendering PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(KonataRendering PUBLIC KonataCore Qt6::Gui)

# --- KonataExport: PNG export via painter (no Widgets chrome) ---
add_library(KonataExport STATIC
    src/export/ExportService.h
    src/export/ExportService.cpp)
target_include_directories(KonataExport PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(KonataExport PUBLIC KonataRendering KonataSerialization Qt6::Gui Qt6::Widgets)

# --- KonataSerialization: .konata JSON persistence (no Widgets) ---
add_library(KonataSerialization STATIC
    src/serialization/KonataProject.h
    src/serialization/KonataProject.cpp
    src/serialization/ProjectSerializer.h
    src/serialization/ProjectSerializer.cpp
    src/serialization/ProjectManager.h
    src/serialization/ProjectManager.cpp)
target_link_libraries(KonataSerialization PUBLIC KonataCore nlohmann_json::nlohmann_json)

# --- Lightweight 0.1C verification, no test framework ---
add_executable(KonataSerializationCheck tests/SerializationRoundTrip.cpp)
target_link_libraries(KonataSerializationCheck PRIVATE KonataSerialization Qt6::Core)
set_target_properties(KonataSerializationCheck PROPERTIES WIN32_EXECUTABLE FALSE)

# --- Lightweight 0.1D verification: document flow + offscreen panel wiring ---
set(KONATA_AUTHORING_TEST_SOURCES
    tests/AuthoringFlowCheck.cpp
    src/document/ProjectDocument.h
    src/document/ProjectDocument.cpp
    src/ui/ProjectPanel.h
    src/ui/ProjectPanel.cpp
    src/ui/PreviewArea.h
    src/ui/PreviewArea.cpp
    src/ui/RoundedBubble.h
    src/ui/RoundedBubble.cpp
    src/ui/InspectorPanel.h
    src/ui/InspectorPanel.cpp
    src/ui/authoring/CharacterEditorWidget.h
    src/ui/authoring/CharacterEditorWidget.cpp
    src/ui/authoring/ConversationEditorWidget.h
    src/ui/authoring/ConversationEditorWidget.cpp
    src/ui/authoring/MessageEditorWidget.h
    src/ui/authoring/MessageEditorWidget.cpp
    src/ui/authoring/SceneEditorWidget.h
    src/ui/authoring/SceneEditorWidget.cpp
    src/ui/authoring/SceneEventEditorWidget.h
    src/ui/authoring/SceneEventEditorWidget.cpp)
add_executable(KonataAuthoringCheck ${KONATA_AUTHORING_TEST_SOURCES})
target_include_directories(KonataAuthoringCheck PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(KonataAuthoringCheck PRIVATE KonataSerialization KonataRendering Qt6::Widgets)
set_target_properties(KonataAuthoringCheck PROPERTIES WIN32_EXECUTABLE FALSE)

# --- Lightweight 0.1E verification: message/event authoring + nested wiring ---
set(KONATA_MESSAGE_EVENT_TEST_SOURCES
    tests/MessageEventCheck.cpp
    src/document/ProjectDocument.h
    src/document/ProjectDocument.cpp
    src/ui/ProjectPanel.h
    src/ui/ProjectPanel.cpp
    src/ui/PreviewArea.h
    src/ui/PreviewArea.cpp
    src/ui/RoundedBubble.h
    src/ui/RoundedBubble.cpp
    src/ui/InspectorPanel.h
    src/ui/InspectorPanel.cpp
    src/ui/authoring/CharacterEditorWidget.h
    src/ui/authoring/CharacterEditorWidget.cpp
    src/ui/authoring/ConversationEditorWidget.h
    src/ui/authoring/ConversationEditorWidget.cpp
    src/ui/authoring/MessageEditorWidget.h
    src/ui/authoring/MessageEditorWidget.cpp
    src/ui/authoring/SceneEditorWidget.h
    src/ui/authoring/SceneEditorWidget.cpp
    src/ui/authoring/SceneEventEditorWidget.h
    src/ui/authoring/SceneEventEditorWidget.cpp)
add_executable(KonataMessageEventCheck ${KONATA_MESSAGE_EVENT_TEST_SOURCES})
target_include_directories(KonataMessageEventCheck PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(KonataMessageEventCheck PRIVATE KonataSerialization KonataRendering Qt6::Widgets)
set_target_properties(KonataMessageEventCheck PROPERTIES WIN32_EXECUTABLE FALSE)

# --- Direct chat editor verification: selection, manipulation, add bar ---
set(KONATA_CHATEDITOR_TEST_SOURCES
    tests/ChatEditorCheck.cpp
    src/document/ProjectDocument.h
    src/document/ProjectDocument.cpp
    src/ui/PreviewArea.h
    src/ui/PreviewArea.cpp
    src/ui/RoundedBubble.h
    src/ui/RoundedBubble.cpp
    src/ui/InspectorPanel.h
    src/ui/InspectorPanel.cpp
    src/ui/authoring/CharacterEditorWidget.h
    src/ui/authoring/CharacterEditorWidget.cpp
    src/ui/authoring/ConversationEditorWidget.h
    src/ui/authoring/ConversationEditorWidget.cpp
    src/ui/authoring/MessageEditorWidget.h
    src/ui/authoring/MessageEditorWidget.cpp
    src/ui/authoring/SceneEditorWidget.h
    src/ui/authoring/SceneEditorWidget.cpp
    src/ui/authoring/SceneEventEditorWidget.h
    src/ui/authoring/SceneEventEditorWidget.cpp)
add_executable(KonataChatEditorCheck ${KONATA_CHATEDITOR_TEST_SOURCES})
target_include_directories(KonataChatEditorCheck PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(KonataChatEditorCheck PRIVATE KonataSerialization KonataRendering Qt6::Widgets)
set_target_properties(KonataChatEditorCheck PROPERTIES WIN32_EXECUTABLE FALSE)

# --- Lightweight 0.2A verification: renderer model + PreviewArea integration ---
set(KONATA_RENDERER_TEST_SOURCES
    tests/RendererCheck.cpp
    src/document/ProjectDocument.h
    src/document/ProjectDocument.cpp
    src/ui/PreviewArea.h
    src/ui/PreviewArea.cpp
    src/ui/RoundedBubble.h
    src/ui/RoundedBubble.cpp)
add_executable(KonataRendererCheck ${KONATA_RENDERER_TEST_SOURCES})
target_include_directories(KonataRendererCheck PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(KonataRendererCheck PRIVATE KonataRendering KonataSerialization Qt6::Widgets)
set_target_properties(KonataRendererCheck PROPERTIES WIN32_EXECUTABLE FALSE)

# --- Lightweight 0.2A.1 verification: avatars + image positioning ---
# Needs the message editor sources to verify the authoring controls exist.
set(KONATA_REFINEMENT_TEST_SOURCES
    tests/RendererRefinementCheck.cpp
    src/document/ProjectDocument.h
    src/document/ProjectDocument.cpp
    src/ui/PreviewArea.h
    src/ui/PreviewArea.cpp
    src/ui/RoundedBubble.h
    src/ui/RoundedBubble.cpp
    src/ui/InspectorPanel.h
    src/ui/InspectorPanel.cpp
    src/ui/authoring/CharacterEditorWidget.h
    src/ui/authoring/CharacterEditorWidget.cpp
    src/ui/authoring/ConversationEditorWidget.h
    src/ui/authoring/ConversationEditorWidget.cpp
    src/ui/authoring/MessageEditorWidget.h
    src/ui/authoring/MessageEditorWidget.cpp
    src/ui/authoring/SceneEditorWidget.h
    src/ui/authoring/SceneEditorWidget.cpp
    src/ui/authoring/SceneEventEditorWidget.h
    src/ui/authoring/SceneEventEditorWidget.cpp)
add_executable(KonataRendererRefinementCheck ${KONATA_REFINEMENT_TEST_SOURCES})
target_include_directories(KonataRendererRefinementCheck PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(KonataRendererRefinementCheck PRIVATE KonataRendering KonataSerialization Qt6::Widgets)
set_target_properties(KonataRendererRefinementCheck PROPERTIES WIN32_EXECUTABLE FALSE)

# --- Lightweight 0.2B verification: generic App UI architecture ---
add_executable(KonataAppUiCheck tests/AppUiCheck.cpp
    src/document/ProjectDocument.h
    src/document/ProjectDocument.cpp
    src/ui/PreviewArea.h
    src/ui/PreviewArea.cpp
    src/ui/RoundedBubble.h
    src/ui/RoundedBubble.cpp)
target_include_directories(KonataAppUiCheck PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(KonataAppUiCheck PRIVATE KonataRendering KonataSerialization Qt6::Widgets)
set_target_properties(KonataAppUiCheck PROPERTIES WIN32_EXECUTABLE FALSE)

# --- Lightweight 0.2C.1 verification: Apple Messages visual foundation ---
set(KONATA_APPLE_TEST_SOURCES
    tests/AppleMessagesCheck.cpp
    src/document/ProjectDocument.h
    src/document/ProjectDocument.cpp
    src/ui/PreviewArea.h
    src/ui/PreviewArea.cpp
    src/ui/RoundedBubble.h
    src/ui/RoundedBubble.cpp)
add_executable(KonataAppleMessagesCheck ${KONATA_APPLE_TEST_SOURCES})
target_include_directories(KonataAppleMessagesCheck PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(KonataAppleMessagesCheck PRIVATE KonataRendering KonataSerialization Qt6::Widgets)
set_target_properties(KonataAppleMessagesCheck PROPERTIES WIN32_EXECUTABLE FALSE)

# --- Demanding 0.2C.2 verification: exact Apple Messages reconstruction ---
set(KONATA_APPLE_EXACT_TEST_SOURCES
    tests/AppleMessagesExactCheck.cpp
    src/document/ProjectDocument.h
    src/document/ProjectDocument.cpp
    src/ui/PreviewArea.h
    src/ui/PreviewArea.cpp
    src/ui/RoundedBubble.h
    src/ui/RoundedBubble.cpp)
add_executable(KonataAppleMessagesExactCheck ${KONATA_APPLE_EXACT_TEST_SOURCES})
target_include_directories(KonataAppleMessagesExactCheck PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(KonataAppleMessagesExactCheck PRIVATE KonataRendering KonataSerialization Qt6::Widgets)
set_target_properties(KonataAppleMessagesExactCheck PROPERTIES WIN32_EXECUTABLE FALSE)

# --- Canvas aspect tests ---
add_executable(KonataCanvasCheck tests/CanvasCheck.cpp
    src/document/ProjectDocument.h
    src/document/ProjectDocument.cpp)
target_include_directories(KonataCanvasCheck PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(KonataCanvasCheck PRIVATE KonataCore KonataSerialization Qt6::Core)
set_target_properties(KonataCanvasCheck PROPERTIES WIN32_EXECUTABLE FALSE)

# --- Export PNG tests ---
add_executable(KonataExportCheck tests/ExportCheck.cpp
    src/document/ProjectDocument.h
    src/document/ProjectDocument.cpp)
target_include_directories(KonataExportCheck PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(KonataExportCheck PRIVATE KonataExport KonataRendering KonataSerialization Qt6::Widgets Qt6::Gui)
set_target_properties(KonataExportCheck PROPERTIES WIN32_EXECUTABLE FALSE)
