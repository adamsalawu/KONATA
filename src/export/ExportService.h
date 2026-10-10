#pragma once

#include <QImage>
#include <QSize>
#include <QString>

#include "core/CanvasSettings.h"
#include "serialization/KonataProject.h"

// Export pipeline: Story + AppDefinition + Canvas → PNG.
// Read-only, deterministic, no Core mutation, no widget chrome.
class ExportService
{
public:
    struct Result {
        bool ok = false;
        QString error;
        QSize size;
        QString path;
    };

    // Renders the given conversation (by ID) to a PNG at the canvas's
    // logical size. The image contains only the chat surface (header +
    // messages + composer), centered, with background filling the canvas.
    // No ProjectPanel/Inspector/Timeline/window chrome.
    static Result exportPng(const Konata::Serialization::KonataProject &project,
                            const QString &conversationId,
                            const QString &filePath);

    // In-memory variant for tests: returns the image that would be written.
    static QImage renderToImage(const Konata::Serialization::KonataProject &project,
                                const QString &conversationId,
                                const QSize &logicalSize);

private:
    ExportService() = delete;
};
