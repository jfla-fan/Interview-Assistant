#include <QLoggingCategory>

#ifdef ENABLE_QT_LOG_HOOK

#define TRACE_LOGGING_CATEGORY_PREFIX "__trace__"

#define MAKE_TRACE_CATEGORY(category) \
    (QStringLiteral(QT_UNICODE_LITERAL(TRACE_LOGGING_CATEGORY_PREFIX) + QLatin1String(category).constData()))

#define QT_MESSAGE_LOGGER_COMMON_TRACE(category) \
for (QLoggingCategoryMacroHolder<QtDebugMsg> qt_category((category)()); qt_category; qt_category.control = false) \
    QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC, MAKE_TRACE_CATEGORY(qt_category.name()))


#define qTrace QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC, TRACE_LOGGING_CATEGORY_PREFIX).debug
#define qCTrace(category, ...) QT_MESSAGE_LOGGER_COMMON_TRACE(category).debug(__VA_ARGS__)

#define qError qCritical
#define qCError(category, ...) qCCritical(category __VA_OPT__(,) __VA_ARGS__)

#else

#define TRACE_LOGGING_CATEGORY_PREFIX ""
#define MAKE_TRACE_CATEGORY(category) ""
#define QT_MESSAGE_LOGGER_COMMON_TRACE QT_MESSAGE_LOGGER_COMMON

#define qTrace QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC).noDebug
#define qCTrace(category, ...) QT_MESSAGE_LOGGER_COMMON(category, QtDebugMsg).noDebug(__VA_ARGS__)

#define qError
#define qCError(category, ...)

#endif
