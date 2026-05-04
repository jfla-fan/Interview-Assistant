#pragma once

#include "../log_component.h"


class ConsoleLogComponent : public ILogComponent
{
public:
    ConsoleLogComponent(QString name, LogFormatterPtr&& formatter);

    bool log(const LogMessage& message) override;

    static std::unique_ptr< ConsoleLogComponent > create(QString name,
                                                         LogFormatterPtr formatter);
};
