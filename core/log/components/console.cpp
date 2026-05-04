#include "console.h"

#include <fmt/format.h>


ConsoleLogComponent::ConsoleLogComponent(QString name, LogFormatterPtr&& formatter)
    : ILogComponent(std::move(name), std::move(formatter))
{ }


bool ConsoleLogComponent::log(const LogMessage& message)
{
    if (message.level < m_minLevel) return true;

    QString formatted = formatMessage(message);
    fmt::println("{}", formatted);

    return true;
}


std::unique_ptr< ConsoleLogComponent > ConsoleLogComponent::create(QString name, LogFormatterPtr formatter)
{
    return std::make_unique< ConsoleLogComponent >(std::move(name), std::move(formatter));
}
