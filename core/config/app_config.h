#pragma once

#include "../common.h"

#include <QKeySequence>
#include <QFuture>


// log/log.h
enum LogLevel : int;

// log/log.h
enum class ActionType;


namespace AppConfig
{

enum ConfigLoadMode : int
{
    LoadMode_Default,           // nullptr if content is incorrect
    LoadMode_Overload,          // default config if content is incorrect
    LoadMode_PartialOverload,   // substitute incorrect parts with default values

    LoadMode_Max                // meta
};

struct Config;

using ConfigPtr = std::shared_ptr< Config >;
using ConstConfigPtr = std::shared_ptr< const Config >;

struct Config
{
    // general
    bool     excludeFromCapture;
    bool     saveScreenshots;

    // logging
    bool     asyncMode;
    int      maxQueueSize;
    LogLevel minLevel;

    // proxies
    using ProxyList = ::ProxyList;
    ProxyList proxies;

    // hotkeys
    using HotkeyList = ::HotkeyList;
    HotkeyList hotkeys;

    // other
    ConfigLoadMode mode;
    QString path;

    // routines
    ConfigPtr clone() const;
    QString   toStringDebug() const;
};


ConstConfigPtr       GetDefaultConfig();
ConfigPtr            CreateDefaultConfig();
ConfigPtr            CreateDefaultConfig(const QString& path, ConfigLoadMode mode);

ConfigPtr            LoadConfigFromData(const QString& data, ConfigLoadMode mode = LoadMode_Default);
QFuture< ConfigPtr > LoadConfigFromDataAsync(const QString& data, ConfigLoadMode mode = LoadMode_Default);
ConfigPtr            LoadConfigFromFile(const QString& path, ConfigLoadMode mode = LoadMode_Default);
QFuture< ConfigPtr > LoadConfigFromFileAsync(const QString& path, ConfigLoadMode mode = LoadMode_Default);

bool                 SaveConfigToFile(const QString& path, ConstConfigPtr config);

}
