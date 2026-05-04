#pragma once

#include <QPixmap>

QT_FORWARD_DECLARE_CLASS(QWidget)


class ScreenshotHelper
{
public:
    static QPixmap    grabScreenshot(QWidget *widgetToHide = nullptr);
    static QPixmap    grabScreenshot(const QList< QWidget* >& widgetsToHide);
    static bool       saveScreenshot(const QPixmap& screenshot, const QString& path, QLatin1StringView format);
    static QByteArray encodeScreenshotToBase64(const QPixmap& screenshot, QLatin1StringView format);
    static QString    getDefaultScreenshotPath(QLatin1StringView format);
};
