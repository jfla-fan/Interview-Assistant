#include "overlay_widget.h"


OverlayWidget::OverlayWidget(QWidget *parent, bool excludeFromCapture)
    : OverlayWidget("OverlayWidget", parent, excludeFromCapture)
{ }


OverlayWidget::OverlayWidget(QString objectName, QWidget *parent, bool excludeFromCapture)
    : BaseWidget(objectName, parent, excludeFromCapture)
{
    setWindowFlags(Qt::Tool |
                   Qt::FramelessWindowHint |
                   Qt::WindowStaysOnTopHint |
                   Qt::CustomizeWindowHint);

    setAttribute(Qt::WA_TranslucentBackground);
}


void OverlayWidget::toggleVisibility(bool recursive)
{
    if (isHidden() && m_wasVisible) {
        m_wasVisible = !m_wasVisible; // show event will change it back
        show();
    }
    else if (isVisible()) {
        m_wasVisible = !m_wasVisible; // hide event will change it back
        hide();
    }

    if (recursive) {
        for (auto* overlayChild : findChildren< OverlayWidget* >(Qt::FindDirectChildrenOnly)) {
            overlayChild->toggleVisibility();
        }
    }
}


void OverlayWidget::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    m_wasVisible = !m_wasVisible;
}


void OverlayWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    m_wasVisible = !m_wasVisible;
}
