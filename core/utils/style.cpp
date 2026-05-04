#include "style.h"

#include "files.h"

#include <QWidget>
#include <QDebug>


QString utils::LoadStyle(const QString& path)
{
    auto fileContent = ReadFile(path);
    if (!fileContent) {
        qCritical() << "Failed to load style at " << path << ". Contact developers.";
        return "";
    }

    return *fileContent;
}


void utils::SetWidgetStyleSheet(QWidget* widget, const QString& stylePath)
{
    Q_ASSERT(widget);
    widget->setStyleSheet(LoadStyle(stylePath));
}
