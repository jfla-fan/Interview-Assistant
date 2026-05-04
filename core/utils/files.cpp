#include "files.h"

#include <QFileInfo>
#include <QDir>


bool utils::CreateDirectoryToFilePath(const QString& absolutePath)
{
    QFileInfo fileInfo(absolutePath);
    QDir dir = fileInfo.dir();

    return dir.mkpath(".");
}


std::optional< QString > utils::ReadFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qCritical() << "Failed to read file at " << path;
        return std::nullopt;
    }

    return QString::fromUtf8(file.readAll());
}
