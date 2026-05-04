#pragma once

#include "../common.h"

#include <QString>
#include <QDateTime>
#include <QColor>
#include <QMutex>
#include <QtClassHelperMacros>
#include <QReadWriteLock>


class ILogComponent;
class LogManager;
class LoggerThread;


enum LogLevel : int
{
    LogLevel_Trace,
    LogLevel_Debug,
    LogLevel_Info,
    LogLevel_Warning,
    LogLevel_Error,
    LogLevel_Fatal,

    LogLevel_Max // meta
};


struct LogMessage
{
    QString   message;
    LogLevel  level;
    QDateTime timestamp;
    QString   category;
    QString   file;
    QString   function;
    int       line;
    QThread*  thread;
};


struct LogFormat
{
    QString pattern = QStringLiteral("[{timestamp}] [{level}] {message}");
    QString timestampFormat = QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz");
};


struct ColorScheme
{
    QColor trace    = QColor(128, 128, 128);   // Gray
    QColor debug    = QColor(0, 255, 255);     // Cyan
    QColor info     = QColor(255, 255, 255);   // White
    QColor warning  = QColor(255, 255, 0);     // Yellow
    QColor error    = QColor(255, 0, 0);       // Red
    QColor fatal    = QColor(255, 0, 255);     // Magenta

    QColor getColor(LogLevel lvl) const;
};


struct LogInitParams
{
    bool     asyncMode = true;
    int      maxQueueSize = 10000;
    LogLevel minLevel = LogLevel_Trace;
    bool     redirectQtLogging = true;
};


class LogManager : public QObject
{
    Q_OBJECT

public:
    Q_DISABLE_COPY_MOVE(LogManager)

    static LogManager* instance();

    const LogInitParams& initParams() const { return m_params; }
    LogInitParams& initParams() { return m_params; }

    bool asyncMode() const;
    int maxQueueSize() const;
    LogLevel minLogLevel() const;
    bool redirectQtLogging() const;

    LogManager& setAsyncMode(bool value);
    LogManager& setMaxQueueSize(int value);
    LogManager& setMinLogLevel(LogLevel value);
    LogManager& setRedirectQtLogging(bool value);

    bool initialize(const LogInitParams& params = LogInitParams());
    bool addComponent(ILogComponent* logComponent);
    ILogComponent* findComponent(QAnyStringView name);
    bool removeComponent(QAnyStringView name);
    bool broadcast(const LogMessage& message);
    template< ranges::range TRange > requires(std::is_convertible_v< ranges::range_value_t< TRange >, QAnyStringView >)
    bool broadcastTo(const LogMessage& message, TRange&& range); // @todo implement later
    bool shutdown();

    template< class TComponent >
    std::add_pointer_t< TComponent > findComponent(QAnyStringView name) {
        return dynamic_cast< std::add_pointer_t< TComponent > >(findComponent(name));
    }

private:
    friend class LoggerThread;

    LogManager();    
    static void qtLogMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);
    void broadcastSync(const LogMessage& message);
    ILogComponent* findComponentNoLock(QAnyStringView name);

    QMutex m_componentsMutex;
    QList< ILogComponent* > m_components;

    QReadWriteLock m_paramsLock;
    LogInitParams m_params;
    std::unique_ptr< LoggerThread > m_loggerThread;
    bool m_initialized = false;
    // keeps messages before log manager is initialized
    QList< LogMessage > m_pendingMessages;
};


// @todo implement later
template< ranges::range TRange > requires(std::is_convertible_v< ranges::range_value_t< TRange >, QAnyStringView >)
bool LogManager::broadcastTo(const LogMessage& message, TRange&& range)
{
    for (QAnyStringView componentName : range) {

    }

    return false;
}

// Utilities

const QString& LogLevelToString(LogLevel lvl);
LogLevel QtMsgTypeToLogLevel(QtMsgType type);
LogMessage CreateLogMessage(LogLevel lvl, const QString& msg, const char* file, int line, const char* function);
QString CreateUniqueLogFileName();

template< typename... Args >
QString FmtToQString(fmt::string_view format_str, Args&&... args)
{
    std::string formatted = fmt::vformat(format_str, fmt::make_format_args(args...));
    return QString::fromStdString(formatted);
}

inline QString FmtToQString(const QString& str)
{
    return str;
}

template< typename... Args >
inline QString FmtToQString(const QString& str, Args&&... args)
{
    return FmtToQString(str.toStdString(), std::forward< Args >(args)...);
}

template< typename... Args >
inline QString FmtToQString(const char* str, Args&&... args)
{
    return FmtToQString(fmt::string_view{ str }, std::forward< Args >(args)...);
}

template<>
struct fmt::formatter< LogLevel > : fmt::formatter< QString >
{
    template< typename FormatContext >
    auto format(LogLevel lvl, FormatContext& ctx) const {
        return fmt::formatter< QString >::format(LogLevelToString(lvl), ctx);
    }
};

class DynamicLogEntry
{
public:
    Q_DISABLE_COPY_MOVE(DynamicLogEntry)

    DynamicLogEntry(LogMessage msg)
        : m_message(std::move(msg))
    {
        m_stream.setString(&m_message.message);
    }

    ~DynamicLogEntry()
    {
        LogManager::instance()->broadcast(m_message);
    }

    template< class... TArgs >
    DynamicLogEntry& operator () (TArgs&&... args)
    {
        m_stream << FmtToQString(std::forward< TArgs >(args)...);
        return *this;
    }

    DynamicLogEntry& operator () () { return *this; }

    template< class T >
    DynamicLogEntry& operator << (const T& value)
    {
        m_stream << qformat("{}", value);
        return *this;
    }

private:
    LogMessage m_message;
    QTextStream m_stream;
};


/* Logging macros for convenience.
 *
 * Typical usage:
 *
 * - LOG_INFO("Text with data - {}, {}, ...", data1, data2)
 * - LOG_INFO() << "Text..." << data;
 * - LOG_INFO() << qformat("Text with data - {}", data);
 */

#define LOG_IMPL(lvl) DynamicLogEntry{ CreateLogMessage(lvl, "", __FILE__, __LINE__, Q_FUNC_INFO) }

#define LOG_TRACE   LOG_IMPL(LogLevel_Trace)
#define LOG_DEBUG   LOG_IMPL(LogLevel_Debug)
#define LOG_INFO    LOG_IMPL(LogLevel_Info)
#define LOG_WARNING LOG_IMPL(LogLevel_Warning)
#define LOG_ERROR   LOG_IMPL(LogLevel_Error)
#define LOG_FATAL   LOG_IMPL(LogLevel_Fatal)

#include "qt_log_hook.inl"
