#pragma once

#include <memory>

enum LogLevel: int; // log/log.h
enum class ActionType; // action.h

namespace AppConfig
{
    struct Config;

    enum ConfigLoadMode : int;

    using ConfigPtr = std::shared_ptr< Config >;
    using ConstConfigPtr = std::shared_ptr< const Config >;
}
