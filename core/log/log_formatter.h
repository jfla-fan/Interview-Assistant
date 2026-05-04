#pragma once

#include "log.h"

class ILogFormatter;
using LogFormatterPtr = std::shared_ptr< ILogFormatter >;

class ILogFormatter
{
public:
    ILogFormatter() = default;
    virtual ~ILogFormatter() = default;

    virtual QString format(const LogMessage& msg) const = 0;
    virtual const LogFormat& getFormat() const = 0;
    virtual LogFormat& getFormat() = 0;
    virtual void setFormat(LogFormat format) = 0;
};

class LogFormatterBase : public ILogFormatter
{
public:
    QString format(const LogMessage& msg) const { return m_format.pattern; };
    const LogFormat& getFormat() const { return m_format; };
    LogFormat& getFormat() { return m_format; }
    void setFormat(LogFormat format) { m_format = std::move(format); };

protected:
    LogFormat m_format;
};

class LogFormatterDecorator : public ILogFormatter
{
public:
    LogFormatterDecorator(LogFormatterPtr source = std::make_shared< LogFormatterBase >())
        : m_wrappee(std::move(source))
    { }

    QString format(const LogMessage& msg) const override { return m_wrappee->format(msg); }
    const LogFormat& getFormat() const override { return m_wrappee->getFormat(); };
    LogFormat& getFormat() override { return m_wrappee->getFormat(); }
    void setFormat(LogFormat format) override { m_wrappee->setFormat(std::move(format)); };

protected:
    LogFormatterPtr m_wrappee;
};
