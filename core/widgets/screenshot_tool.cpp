#include "screenshot_tool.h"

#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainterPath>


ScreenshotTool::ScreenshotTool(QWidget* parent, bool excludeFromCapture)
    : OverlayWidget("ScreenshotTool", parent, excludeFromCapture)
    , m_isSelecting(false)
{
    setCursor(Qt::CrossCursor);
    hide();
}


void ScreenshotTool::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen) {
        m_desktopPixmap = screen->grabWindow(0);
        this->setGeometry(screen->geometry());
    }
    m_selectionRect = QRect();

    activateWindow();
}


void ScreenshotTool::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.drawPixmap(0, 0, m_desktopPixmap);

    QPainterPath overlayPath;
    overlayPath.setFillRule(Qt::OddEvenFill);
    overlayPath.addRect(rect());

    if (!m_selectionRect.isNull()) {
        overlayPath.addRect(m_selectionRect);
    }

    painter.fillPath(overlayPath, QColor(0, 0, 0, 120));

    if (!m_selectionRect.isNull()) {
        painter.setPen(QPen(Qt::white, 1, Qt::DashLine));
        painter.drawRect(m_selectionRect);
    }
}


void ScreenshotTool::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_isSelecting = true;
        m_startPoint = event->pos();
        m_selectionRect = QRect(m_startPoint, QSize());
        update();
    }
}


void ScreenshotTool::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isSelecting) {
        m_selectionRect = QRect(m_startPoint, event->pos()).normalized();
        update();
    }
}


void ScreenshotTool::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_isSelecting) {
        m_isSelecting = false;
        if (m_selectionRect.width() > 4 && m_selectionRect.height() > 4) {
            takeScreenshot();
        } else {
            hide();
            emit cancelled();
        }
    }
}


void ScreenshotTool::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        m_isSelecting = false;
        hide();
        emit cancelled();
    }
}


void ScreenshotTool::takeScreenshot()
{
    hide();
    QPixmap finalShot = m_desktopPixmap.copy(m_selectionRect);
    emit screenshotTaken(finalShot);
}
