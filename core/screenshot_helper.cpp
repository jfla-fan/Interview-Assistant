#include "screenshot_helper.h"

#include "log/log.h"

#include "utils/files.h"

#include <QWidget>
#include <QScreen>
#include <QBuffer>
#include <QDateTime>
#include <QStandardPaths>
#include <QCoreApplication>


QPixmap ScreenshotHelper::grabScreenshot(QWidget *widgetToHide)
{
    // Hide the widget temporarily
    bool wasVisible = widgetToHide ? widgetToHide->isVisible() : false;
    if (widgetToHide) {
        widgetToHide->hide();
        QCoreApplication::processEvents(); // Ensure window is fully hidden
    }

    QScreen* screen = QGuiApplication::primaryScreen();
    QPixmap screenshot = screen ? screen->grabWindow() : QPixmap();

    // Restore widget visibility
    if (widgetToHide && wasVisible) {
        widgetToHide->show();
    }

    return screenshot;
}


QPixmap ScreenshotHelper::grabScreenshot(const QList< QWidget* >& widgetsToHide)
{
    QList< bool > visibilityMask;
    visibilityMask.resize(widgetsToHide.size());

    for (int i = 0; i < widgetsToHide.size(); ++i) {
        if (!widgetsToHide[i]) {
            qError() << QString("Widget at index %1 is null. Something's seriously wrong.").arg(i);
            continue;
        }

        visibilityMask[i] = widgetsToHide[i]->isVisible();
        widgetsToHide[i]->hide();
    }

    QCoreApplication::processEvents(); // Ensure windows are hidden

    QScreen* screen = QGuiApplication::primaryScreen();
    QPixmap screenshot = screen ? screen->grabWindow() : QPixmap();

    // restore visibility
    for (int i = 0; i < widgetsToHide.size(); ++i) {
        if (!widgetsToHide[i] || !visibilityMask[i]) continue;
        widgetsToHide[i]->show();
    }

    return screenshot;
}


bool ScreenshotHelper::saveScreenshot(const QPixmap& screenshot, const QString& path, QLatin1StringView format)
{
    utils::CreateDirectoryToFilePath(path);

    if (!screenshot.save(path, format.data())) {
        qDebug() << "Failed to save screenshot to " << path;
        return false;
    }

    qInfo() << "Saved screenshot to " << path;
    return true;
}


QByteArray ScreenshotHelper::encodeScreenshotToBase64(const QPixmap& screenshot, QLatin1StringView format)
{
    QByteArray byteArray;

    QBuffer buffer(&byteArray);
    if (buffer.open(QIODevice::WriteOnly) && screenshot.save(&buffer, format.data())) {
        return byteArray.toBase64();
    }

    qWarning() << QString("Failed to encode screenshot in %1 format").arg(format);
    return QByteArray();
}


QString ScreenshotHelper::getDefaultScreenshotPath(QLatin1StringView format)
{
    return QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/Screenshots/"
           + QString("screenshot_%1.%2").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hhmmss")).arg(format);
}
