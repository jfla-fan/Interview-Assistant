#include "fmt.h"
#include <QThread>


QString FmtLogFormatter::format(const LogMessage& msg) const {
    QString inner = LogFormatterDecorator::format(msg);
    auto args = getArgs(msg);
    std::string formatted = fmt::vformat(inner.toStdString(), args);
    return QString::fromStdString(formatted);
}

std::shared_ptr< FmtLogFormatter >
FmtLogFormatter::create(LogFormatterPtr source)
{
    return std::make_shared< FmtLogFormatter >(std::move(source));
}

FmtLogFormatter::args_t FmtLogFormatter::getArgs(const LogMessage& msg, args_t args) const {

    args.push_back(fmt::arg("timestamp", msg.timestamp.toString(getFormat().timestampFormat)));
    args.push_back(fmt::arg("level", msg.level));
    args.push_back(fmt::arg("category", msg.category));
    args.push_back(fmt::arg("message", msg.message));
    args.push_back(fmt::arg("file", msg.file));
    args.push_back(fmt::arg("function", msg.function));
    args.push_back(fmt::arg("line", msg.line));
    args.push_back(fmt::arg("thread", msg.thread ? msg.thread->objectName() : ""));
    args.push_back(fmt::arg("processingThread", QThread::currentThread()->objectName()));

    return args;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FmtCustomThreadLogFormatter

std::shared_ptr< FmtCustomThreadLogFormatter >
FmtCustomThreadLogFormatter::create(QObject* object, LogFormatterPtr source)
{
    return std::make_shared< FmtCustomThreadLogFormatter >(object, std::move(source));
}

FmtLogFormatter::args_t FmtCustomThreadLogFormatter::getArgs(const LogMessage& msg, args_t args) const {
    args.push_back(fmt::arg("processingThread", m_object->thread()->objectName()));
    return FmtLogFormatter::getArgs(msg, std::move(args));
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
