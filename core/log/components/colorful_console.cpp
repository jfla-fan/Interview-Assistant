#include "colorful_console.h"

#include <fmt/color.h>


static fmt::text_style getLogLevelStyle(LogLevel level)
{
    switch (level) {
    case LogLevel_Trace:    return fg(fmt::terminal_color::bright_black);   // Gray
    case LogLevel_Debug:    return fg(fmt::terminal_color::cyan);           // Cyan
    case LogLevel_Info:     return fg(fmt::terminal_color::bright_white);   // White
    case LogLevel_Warning:  return fg(fmt::terminal_color::bright_yellow);  // Yellow
    case LogLevel_Error:    return fg(fmt::terminal_color::bright_red);     // Red
    case LogLevel_Fatal:    return fg(fmt::terminal_color::bright_magenta); // Magenta
    default:                return {};
    }
}


ColorfulConsoleLogComponent::ColorfulConsoleLogComponent(QString name, LogFormatterPtr formatter)
    : ILogComponent(std::move(name), std::move(formatter))
{ }


bool ColorfulConsoleLogComponent::log(const LogMessage& message)
{
    if (message.level < m_minLevel) return true;

    QString formatted = formatMessage(message);
    fmt::print(getLogLevelStyle(message.level), "{}\n", formatted);

    return true;
}


std::unique_ptr< ColorfulConsoleLogComponent > ColorfulConsoleLogComponent::create(QString name,
                                                                                   LogFormatterPtr formatter)
{
    return std::make_unique< ColorfulConsoleLogComponent >(std::move(name), std::move(formatter));
}



