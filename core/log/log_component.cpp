#include "log_component.h"

#include <fmt/format.h>
#include <fmt/args.h>

#include <QThread>


ILogComponent::ILogComponent(QString name, LogFormatterPtr formatter)
    : m_name(std::move(name))
    , m_formatter(std::move(formatter))
{ }


ILogComponent::~ILogComponent()
{
    // if not auto unregister, make sure to remove component before destruction!
    if (m_autoUnregister) {
        LogManager::instance()->removeComponent(name());
    }
}


QString ILogComponent::formatMessage(const LogMessage& message) const
{
    return m_formatter->format(message);
}
