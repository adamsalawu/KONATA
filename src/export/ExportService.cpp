#include "export/ExportService.h"

#include <QFileInfo>
#include <QDir>
#include <QImage>
#include <QPainter>

#include "rendering/CanvasChatPainter.h"

using namespace Konata;

ExportService::Result ExportService::exportPng(const Serialization::KonataProject &project,
                                               const QString &conversationId,
                                               const QString &filePath)
{
    const QSize logical = project.canvas.logicalSize();
    if (!project.canvas.isValid()) {
        return {false, QStringLiteral("Invalid canvas dimensions"), logical, filePath};
    }
    QFileInfo info(filePath);
    if (info.fileName().isEmpty())
        return {false, QStringLiteral("Invalid output path"), logical, filePath};
    QDir dir = info.dir();
    if (!dir.exists() && !dir.mkpath(QStringLiteral(".")))
        return {false, QStringLiteral("Cannot create output directory"), logical, filePath};

    QImage image = renderToImage(project, conversationId, logical);
    if (image.isNull())
        return {false, QStringLiteral("Failed to render image"), logical, filePath};
    if (!image.save(filePath, "PNG"))
        return {false, QStringLiteral("Failed to write PNG"), logical, filePath};
    return {true, QString(), image.size(), filePath};
}

QImage ExportService::renderToImage(const Serialization::KonataProject &project,
                                    const QString &conversationId,
                                    const QSize &logicalSize)
{
    QSize size = logicalSize;
    if (!size.isValid() || size.isEmpty())
        size = project.canvas.logicalSize();
    // If caller requested a specific size different from project's canvas, honor it via a temporary Custom canvas
    if (size != project.canvas.logicalSize()) {
        Konata::Core::CanvasSettings tmp = project.canvas;
        tmp.preset = Konata::Core::CanvasSettings::Preset::Custom;
        tmp.customWidth = size.width();
        tmp.customHeight = size.height();
        if (!tmp.isValid()) tmp = project.canvas;
        return CanvasChatPainter::renderToImage(project.story, project.appDefinition, tmp, conversationId);
    }
    return CanvasChatPainter::renderToImage(project.story, project.appDefinition, project.canvas, conversationId);
}
