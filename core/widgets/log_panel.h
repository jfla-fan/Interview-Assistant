#pragma once

#include "overlay_widget.h"
#include "../log/log.h"


QT_FORWARD_DECLARE_CLASS(QTextEdit)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QtTextEditLogComponent)


class LogPanel : public OverlayWidget
{
    Q_OBJECT
public:
    explicit LogPanel(QWidget* parent = nullptr);

public slots:
    void appendMessage(const QString& message, const QColor& color);
    void clear();
    void toggleLogging();
    void setLogLevel(LogLevel lvl);
    LogLevel getLogLevel() const;

signals:
    void loggingChanged(bool isStopped);

private:
    QTextEdit* m_logDisplay;
    QComboBox* m_levelFilter;
    QPushButton* m_pauseButton;
    QPushButton* m_clearButton;

    bool m_loggingStopped;
    LogLevel m_logLevel;
    std::unique_ptr< QtTextEditLogComponent > m_textEditLogComponent;

    void applyLevelFilter();
};
