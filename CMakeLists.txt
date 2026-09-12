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

find_package(Qt6 6.8 REQUIRED COMPONENTS Widgets)

set(KONATA_SOURCES
    src/main.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/ProjectPanel.h
    src/ui/ProjectPanel.cpp
    src/ui/PreviewArea.h
    src/ui/PreviewArea.cpp
    src/ui/InspectorPanel.h
    src/ui/InspectorPanel.cpp
    src/ui/TimelinePanel.h
    src/ui/TimelinePanel.cpp
    resources/resources.qrc
)

if(COMMAND qt_add_executable)
    qt_add_executable(KONATA MANUAL_FINALIZATION ${KONATA_SOURCES})
else()
    add_executable(KONATA WIN32 ${KONATA_SOURCES})
endif()

target_include_directories(KONATA PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(KONATA PRIVATE Qt6::Widgets)

set_target_properties(KONATA PROPERTIES
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME "KONATA"
)

if(COMMAND qt_finalize_executable)
    qt_finalize_executable(KONATA)
endif()
