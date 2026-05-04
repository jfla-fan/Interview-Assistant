#pragma once

#include "log.h"
#include "log_formatter.h"


class ILogComponent
{
public:
    ILogComponent(QString name, LogFormatterPtr formatter);
    virtual ~ILogComponent();

    virtual bool initialize() { return true; };
    virtual bool log(const LogMessage& message) { return true; };
    virtual bool isHealthy() const { return true; };
    virtual bool shutdown() { return true; };

    const QString& name() const { return m_name; };
    bool autoUnregister() const { return m_autoUnregister; }
    LogFormat logFormat() const { return m_formatter->getFormat(); }
    LogLevel minLogLevel() const { return m_minLevel; }
    LogFormatterPtr formatter() const { return m_formatter; }

    virtual void setAutoUnregister(bool value) { m_autoUnregister = value; }
    virtual void setFormat(const LogFormat& format) { m_formatter->setFormat(format); }
    virtual void setMinLevel(LogLevel level) { m_minLevel = level; }
    virtual void setLogFormatter(LogFormatterPtr&& formatter) { m_formatter = std::move(formatter); };

protected:
    virtual QString formatMessage(const LogMessage& message) const;

    const QString m_name;
    bool m_autoUnregister = true;
    LogFormatterPtr m_formatter;
    LogLevel m_minLevel = LogLevel_Trace;
};
