#pragma once

#include <QString>

#include <optional>


namespace utils
{
    bool CreateDirectoryToFilePath(const QString& absolutePath);
    std::optional< QString > ReadFile(const QString& path);
}
