#include "service_config_text_edit.h"

#include <QKeyEvent>


ServiceConfigTextEdit::ServiceConfigTextEdit(QWidget* parent)
    : QTextEdit(parent)
    , m_tabWidth(4)
{ }


void ServiceConfigTextEdit::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Tab && !(event->modifiers() & Qt::ControlModifier)) {
        QString spaces(m_tabWidth, ' ');
        textCursor().insertText(spaces);
        return;
    }

    QTextEdit::keyPressEvent(event);
}

// @TODO ADD MENU HANDLING
