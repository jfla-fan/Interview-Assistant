#pragma once

#include "../log_formatter.h"
#include <fmt/args.h>


class FmtLogFormatter : public LogFormatterDecorator
{
public:
    FmtLogFormatter(LogFormatterPtr source = std::make_shared< LogFormatterBase >())
        : LogFormatterDecorator(std::move(source))
    { }

    QString format(const LogMessage& msg) const override;
    static std::shared_ptr< FmtLogFormatter > create(LogFormatterPtr source = std::make_shared< LogFormatterBase >());

protected:
    using args_t = fmt::dynamic_format_arg_store< fmt::format_context >;
    virtual args_t getArgs(const LogMessage& msg, args_t args = args_t()) const;
};


class FmtCustomThreadLogFormatter : public FmtLogFormatter
{
public:
    FmtCustomThreadLogFormatter(QObject* object, LogFormatterPtr source = std::make_shared< LogFormatterBase >())
        : FmtLogFormatter(std::move(source))
        , m_object(object)
    { }

    static std::shared_ptr< FmtCustomThreadLogFormatter >
    create(QObject* object, LogFormatterPtr source = std::make_shared< LogFormatterBase >());

protected:
    // using FmtLogFormatter::args_t;
    args_t getArgs(const LogMessage& msg, args_t args = args_t()) const override;

    QObject* m_object;
};
