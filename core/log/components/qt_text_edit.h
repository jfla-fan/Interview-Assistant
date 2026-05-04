#pragma once

#include "../log_component.h"

QT_FORWARD_DECLARE_CLASS(QTextEdit)


class QtTextEditLogComponent : public QObject, public ILogComponent
{
    Q_OBJECT

public:
    QtTextEditLogComponent(QString name, QTextEdit* textEdit, LogFormatterPtr formatter);

    bool initialize() override;
    bool log(const LogMessage& msg) override;
    bool shutdown() override;

    const ColorScheme& colorScheme() const { return m_colors; }

    void setFormat(const LogFormat& format) override;
    void setColorScheme(const ColorScheme& colors) { m_colors = colors; }
    void setMaxLines(int maxLines) { m_maxLines = maxLines; }

    static std::unique_ptr< QtTextEditLogComponent > create(QString name,
                                                            QTextEdit* textEdit,
                                                            LogFormatterPtr formatter);

private slots:
    void appendLogToWidget(const LogMessage& msg);
    void appendLogToWidget(const QString& formattedMessage, LogLevel lvl);

private:
    QTextEdit* m_textEdit;
    ColorScheme m_colors;
    int m_maxLines = 1000;
    int m_currentLines = 0;
    LogFormat m_originalFormat;
};
