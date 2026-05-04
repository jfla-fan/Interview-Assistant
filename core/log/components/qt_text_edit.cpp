#include "qt_text_edit.h"

#include <QEvent>
#include <QTextEdit>
#include <QThread>


QtTextEditLogComponent::QtTextEditLogComponent(QString name, QTextEdit* textEdit, LogFormatterPtr formatter)
    : ILogComponent(std::move(name), std::move(formatter))
    , m_textEdit(textEdit)
{ }


bool QtTextEditLogComponent::initialize()
{
    return m_textEdit != nullptr;
}


bool QtTextEditLogComponent::log(const LogMessage& msg)
{
    if (msg.level < m_minLevel) return true;

    QMetaObject::invokeMethod(this, "appendLogToWidget", Qt::QueuedConnection,
                              Q_ARG(LogMessage, msg));

    return true;
}


bool QtTextEditLogComponent::shutdown()
{
    return true;
}


void QtTextEditLogComponent::setFormat(const LogFormat& format)
{
    m_originalFormat = format;
}


void QtTextEditLogComponent::appendLogToWidget(const LogMessage& msg)
{
    QString formatted = formatMessage(msg);
    appendLogToWidget(formatted, msg.level);
}


void QtTextEditLogComponent::appendLogToWidget(const QString& formattedMessage, LogLevel lvl)
{
    if (!m_textEdit) return;

    QColor color = m_colors.getColor(lvl);
    m_textEdit->insertHtml(QString("<font color='%1'>%2</font><br>").arg(m_colors.getColor(lvl).name())
                                                                    .arg(formattedMessage));

    // Limit the number of lines
    m_currentLines++;
    if (m_currentLines > m_maxLines) {
        QTextCursor cursor = m_textEdit->textCursor();
        cursor.movePosition(QTextCursor::Start);
        cursor.select(QTextCursor::LineUnderCursor);
        cursor.removeSelectedText();
        cursor.deleteChar(); // Remove the newline
        --m_currentLines;
    }

    // Auto-scroll to bottom
    QTextCursor cursor = m_textEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_textEdit->setTextCursor(cursor);
}


std::unique_ptr< QtTextEditLogComponent > QtTextEditLogComponent::create(QString name,
                                                                         QTextEdit* textEdit,
                                                                         LogFormatterPtr formatter)
{
    return std::make_unique< QtTextEditLogComponent >(std::move(name), textEdit, std::move(formatter));
}
