#pragma once

#include <QString>

QT_FORWARD_DECLARE_CLASS(QWidget)


namespace utils
{
    QString LoadStyle(const QString& path);
    void SetWidgetStyleSheet(QWidget* widget, const QString& stylePath);
}
