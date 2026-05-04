#include "interactive_label.h"

#include <QMouseEvent>


InteractiveLabel::InteractiveLabel(QWidget* parent)
    : QLabel(parent)
{ }


InteractiveLabel::InteractiveLabel(const QString& text, QWidget* parent)
    : QLabel(text, parent)
{ }


void InteractiveLabel::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked();
    }

    QLabel::mousePressEvent(event);
}
