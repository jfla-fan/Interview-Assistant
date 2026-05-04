#pragma once

#include "../log_component.h"


class ColorfulConsoleLogComponent : public ILogComponent
{
public:
    ColorfulConsoleLogComponent(QString name, LogFormatterPtr formatter);

    bool log(const LogMessage& message) override;

    static std::unique_ptr< ColorfulConsoleLogComponent > create(QString name,
                                                                 LogFormatterPtr formatter);
};
