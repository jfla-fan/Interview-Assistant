#include "log_component.h"
#include "logger_thread.h"

#include <array>


LogManager::LogManager()
{ }


LogManager* LogManager::instance()
{
    static LogManager s_instance;
    return &s_instance;
}


bool LogManager::asyncMode() const
{
    return m_params.asyncMode;
}


int LogManager::maxQueueSize() const
{
    return m_params.maxQueueSize;
}


LogLevel LogManager::minLogLevel() const
{
    return m_params.minLevel;
}


bool LogManager::redirectQtLogging() const
{
    return m_params.redirectQtLogging;
}


LogManager& LogManager::setAsyncMode(bool value)
{
    m_params.asyncMode = value;
    return *this;
}


LogManager& LogManager::setMaxQueueSize(int value)
{
    QWriteLocker locker { &m_paramsLock };
    if (!m_loggerThread) {
        LOG_WARNING() << "Logger thread is not created, max queue size has no effect.";
        return *this;
    }

    if (value < 0) {
        LOG_ERROR() << "Negative (" << value << ") max queue size has no sense.";
        return *this;
    }

    m_params.maxQueueSize = value;
    m_loggerThread->setMaxQueueSize(value);

    return *this;
}


LogManager& LogManager::setMinLogLevel(LogLevel value)
{
    m_params.minLevel = value;
    return *this;
}


LogManager& LogManager::setRedirectQtLogging(bool value)
{
    if (value != m_params.redirectQtLogging) {
        QtMessageHandler handler = (value) ? qtLogMessageHandler : nullptr;
        qInstallMessageHandler(handler);
    }

    return *this;
}


bool LogManager::initialize(const LogInitParams& params)
{
    if (m_initialized) return true;

    QWriteLocker locker { &m_paramsLock };
    {
        m_params = params;

        if (m_params.asyncMode) {
            m_loggerThread = std::make_unique< LoggerThread >();
            m_loggerThread->setObjectName("LoggerThread");
            m_loggerThread->start();
        }

        if (m_params.redirectQtLogging) {
            qInstallMessageHandler(qtLogMessageHandler);
        }

        for (const auto& pendingMessage : m_pendingMessages) {
            broadcastSync(pendingMessage);
        }
    }

    m_initialized = true;

    return true;
}


bool LogManager::addComponent(ILogComponent* logComponent)
{
    if (!logComponent) return false;

    QMutexLocker locker(&m_componentsMutex);
    if (findComponentNoLock(logComponent->name())) {
        return false;
    }

    if (!logComponent->initialize()) {
        return false;
    }

    m_components.append(logComponent);

    return true;
}


ILogComponent* LogManager::findComponent(QAnyStringView name)
{
    QMutexLocker locker(&m_componentsMutex);
    return findComponentNoLock(name);
}


bool LogManager::removeComponent(QAnyStringView name)
{
    QMutexLocker locker(&m_componentsMutex);
    return m_components.removeIf([name](ILogComponent* item) { return name == item->name(); }) > 0;
}


bool LogManager::broadcast(const LogMessage& message)
{
    if (message.level < m_params.minLevel) return false;

    if (!m_initialized) {
        m_pendingMessages.push_back(message);
        return false;
    }

    if (m_params.asyncMode && m_loggerThread) {
        m_loggerThread->enqueueMessage(message);
    } else {
        broadcastSync(message);
    }

    return true;
}


void LogManager::broadcastSync(const LogMessage& message)
{
    QMutexLocker locker(&m_componentsMutex);
    for (auto* component : m_components) {
        component->log(message);
    }
}


ILogComponent* LogManager::findComponentNoLock(QAnyStringView name)
{
    auto it = std::find_if(m_components.cbegin(), m_components.cend(), [name](ILogComponent* item) { return name == item->name(); });
    return it == m_components.cend() ? nullptr : *it;
}


bool LogManager::shutdown()
{
    QMutexLocker locker(&m_componentsMutex);

    if (m_loggerThread) {
        m_loggerThread->shutdown();
        m_loggerThread->wait();
        m_loggerThread.reset();
    }

    for (auto* component : m_components) {
        component->shutdown();
    }

    m_components.clear();
    m_initialized = false;

    return true;
}


void LogManager::qtLogMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    LogMessage logMsg;
    logMsg.message      = msg;

    QLatin1StringView category(context.category),
                      tracePrefix(TRACE_LOGGING_CATEGORY_PREFIX);

#ifdef ENABLE_QT_LOG_HOOK
    // hack: special prefix to recognize trace log level
    if (category.startsWith(tracePrefix))
    {
        logMsg.level    = LogLevel_Trace;
        logMsg.category = category.sliced(tracePrefix.size());
    } else
#endif
    {
        logMsg.level    = QtMsgTypeToLogLevel(type);
        logMsg.category = category;
    }

    logMsg.timestamp    = QDateTime::currentDateTime();
    logMsg.file         = context.file ? context.file : "";
    logMsg.function     = context.function ? context.function : "";
    logMsg.line         = context.line;
    logMsg.thread       = QThread::currentThread();

    LogManager::instance()->broadcast(logMsg);
}


// Utility Functions
const QString& LogLevelToString(LogLevel lvl)
{
    static_assert(LogLevel_Max == 6);
    using namespace Qt::StringLiterals;

    static const std::array< QString, 6 > levelStrings = {
        "TRACE"_L1, "DEBUG"_L1, "INFO"_L1, "WARNING"_L1, "ERROR"_L1, "FATAL"_L1
    };

    static const QString unknown = "UNKNOWN";

    if (lvl >= 0 && lvl < LogLevel_Max) {
        return levelStrings[lvl];
    }

    return unknown;
}


LogLevel QtMsgTypeToLogLevel(QtMsgType type)
{
    switch (type) {
        case QtDebugMsg:    return LogLevel_Debug;
        case QtInfoMsg:     return LogLevel_Info;
        case QtWarningMsg:  return LogLevel_Warning;
        case QtCriticalMsg: return LogLevel_Error;
        case QtFatalMsg:    return LogLevel_Fatal;
        default:            return LogLevel_Debug;
    }
}


LogMessage CreateLogMessage(LogLevel lvl, const QString& msg, const char* file, int line, const char* function)
{
    return { msg, lvl, QDateTime::currentDateTime(), "", QString::fromUtf8(file), QString::fromUtf8(function), line, QThread::currentThread() };
}


QColor ColorScheme::getColor(LogLevel lvl) const
{
    static_assert(LogLevel_Max == 6);

    switch (lvl)
    {
        case LogLevel_Trace:   return trace;
        case LogLevel_Debug:   return debug;
        case LogLevel_Info:    return info;
        case LogLevel_Warning: return warning;
        case LogLevel_Error:   return error;
        case LogLevel_Fatal:   return fatal;
        default:               return trace;
    }
}
