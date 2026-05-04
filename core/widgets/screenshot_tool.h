#pragma once

#include "overlay_widget.h"

#include <QWidget>
#include <QPixmap>
#include <QRect>


class ScreenshotTool : public OverlayWidget
{
    Q_OBJECT

public:
    explicit ScreenshotTool(QWidget* parent = nullptr, bool excludeFromCapture = false);

signals:
    void screenshotTaken(const QPixmap& pixmap);
    void cancelled();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void takeScreenshot();

    QPoint m_startPoint;
    QRect m_selectionRect;
    bool m_isSelecting;
    QPixmap m_desktopPixmap;
};
