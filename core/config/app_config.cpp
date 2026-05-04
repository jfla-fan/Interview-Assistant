#include "app_config.h"

#include "../log/log.h"
#include "../action.h"
#include "../utils/numeric.h"

#include <toml++/toml.h>
#include <magic_enum/magic_enum.hpp>
#include <magic_enum/magic_enum_utility.hpp>

#include <QSettings>
#include <QNetworkProxy>
#include <QtConcurrent>

using namespace AppConfig;


namespace
{

constexpr std::string_view EnumToConfigName(LogLevel level)
{
    constexpr std::string_view prefix { "LogLevel_" };
    return magic_enum::enum_name(level).substr(prefix.length());
}

template< class T >
Optional< T > ReadValue(const toml::table& tbl, std::string_view key, Optional< ConfigLoadMode > hint = std::nullopt)
{
    Q_UNUSED(hint)
    return tbl.at_path(key).value_exact< T >();
}

template<>
Optional< QString > ReadValue(const toml::table& tbl, std::string_view key, Optional< ConfigLoadMode > hint)
{
    auto value = ReadValue< std::string_view >(tbl, key, hint);
    if (value) return ToQString(*value);
    return {};
}

template< class T > requires(std::is_enum_v< T >)
Optional< T > ReadValue(const toml::table& tbl, std::string_view key, Optional< ConfigLoadMode > hint = std::nullopt)
{
    auto value = tbl.at_path(key).value_exact< std::string_view >();
    if (!value) return {};

    return magic_enum::enum_cast< T >(*value);
}

template<>
Optional< LogLevel > ReadValue(const toml::table& tbl, std::string_view key, Optional< ConfigLoadMode > hint)
{
    auto value = tbl.at_path(key).value_exact< std::string_view >();
    if (!value) return {};

    const auto& enumValues = magic_enum::enum_values< LogLevel >();
    auto enumValueIt = std::ranges::find_if(enumValues, [&value](const auto level) { return *value == EnumToConfigName(level); });

    return enumValueIt == enumValues.end() ? Optional< LogLevel >{} : *enumValueIt;
}

template<>
Optional< int > ReadValue(const toml::table& tbl, std::string_view key, Optional< ConfigLoadMode >)
{
    auto value = tbl.at_path(key).value_exact< int64_t >();
    if (!value) return {};

    auto convertedValue = utils::safe_cast< int >(*value);
    if (!convertedValue) {
        return {};
    }

    return *convertedValue;
}

template<>
Optional< Config::HotkeyList > ReadValue(const toml::table& tbl, std::string_view key, Optional< ConfigLoadMode > hint)
{
    auto array = tbl.at_path(key).as_array();
    if (!array) return {};

    Config::HotkeyList result;

    bool canContinue = hint.value_or(LoadMode_Default) == LoadMode_PartialOverload;

    // add enabled hotkeys with unique actions and unique shortcuts
    for (const auto& arrayItem : *array) {
        if (arrayItem.is_table()) {
            const auto& tbl = *arrayItem.as_table();

            auto action   = ReadValue< ActionType >(tbl, "action");
            auto shortcut = ReadValue< QString >(tbl, "shortcut");

            if (!(action && shortcut) && !canContinue) {
                LOG_WARNING("Failed to action and shortcut");
                return {};
            }

            if (!action) {
                LOG_WARNING() << "Failed to read action";
                continue;
            }

            if (!shortcut) {
                LOG_WARNING() << "Failed to read shortcut";
                continue;
            }

            auto enabled  = ReadValue< bool >(tbl, "enabled");

            if (!enabled) {
                LOG_TRACE << "No enabled. Defaulting to true";
                enabled = true;
            }
            // else if (*enabled == false) {
            //     // log
            //     continue;
            // }

            auto dublicatedActionIt = std::ranges::find_if(result, [&action](const auto& hotkey) { return hotkey.Action == *action; });
            bool isActionDublicated = dublicatedActionIt != result.end();

            auto dublicatedShortcutIt = std::ranges::find_if(result, [&shortcut](const auto& hotkey) { return hotkey.Shortcut == *shortcut; });
            bool isShortcutDublicated = dublicatedShortcutIt != result.end();

            if (!isActionDublicated && !isShortcutDublicated) {
                // log
                result.emplace_back(*action, *shortcut, *enabled);
                continue;
            }

            if (isActionDublicated && !isShortcutDublicated) {
                // log
                dublicatedActionIt->Shortcut = *shortcut;
            }
            else if (isShortcutDublicated && !isActionDublicated) {
                // log
                dublicatedShortcutIt->Action = *action;
            }

            // log
            if (!canContinue) return {};
        }
    }

    // add mandatory "Quit" action
    auto quitActionIt = std::ranges::find_if(result, [](const auto& hotkey) { return hotkey.Action == ActionType::Quit; });
    if (quitActionIt == result.end()) {
        auto quitKeySequence = QKeySequence::fromString("Ctrl+Q");
        auto quitShortcutIt = std::ranges::find_if(result, [&quitKeySequence](const auto& hotkey) { return hotkey.Shortcut == quitKeySequence; });

        if (quitShortcutIt != result.end()) {
            qWarning() << "Action \"Quit\" not found. Replacing hotkey (" << ToQString(quitShortcutIt->Action) << ", "
                       << quitShortcutIt->Shortcut << ") with (" << ToQString(ActionType::Quit) << quitShortcutIt->Shortcut << ")";
            quitShortcutIt->Action = ActionType::Quit;
        }
        else {
            qWarning() << "Action \"Quit\" not found. Adding hotkey (" << ToQString(ActionType::Quit) << ", " << quitKeySequence << ")";
            result.emplace_back(ActionType::Quit, std::move(quitKeySequence));
        }
    }

    return result;
}

template<>
Optional< Config::ProxyList > ReadValue(const toml::table& tbl, std::string_view key, Optional< ConfigLoadMode > hint)
{
    auto array = tbl.at_path(key).as_array();
    if (!array) return {};

    Config::ProxyList result;

    bool canContinue = hint.value_or(LoadMode_Default) == LoadMode_PartialOverload;

    // add enabled proxies with unique actions and unique shortcuts
    for (const auto& arrayItem : *array) {
        if (arrayItem.is_table()) {
            const auto& tbl = *arrayItem.as_table();

            auto name = ReadValue< QString >(tbl, "name");
            auto endpoint = ReadValue< QString >(tbl, "endpoint");

            if (!(name && endpoint) && !canContinue) {
                // log
                return {};
            }

            if (!name) {
                // std::cout << "No action\n";
                continue;
            }

            if (!endpoint) {
                // std::cout << "No shotrcut\n";
                continue;
            }

            auto networkProxy = ::ParseProxyFromEndpoint(*endpoint);
            if (!networkProxy) {
                if (!canContinue) {
                    // log
                    return {};
                }

                // log
                continue;
            }

            auto ignoreSslErrors = ReadValue< bool >(tbl, "ignoreSslErrors");

            if (!ignoreSslErrors) {
                // qDebug() << "No ignoreSslErrors flag, defaulting to false
            }

            auto enabled  = ReadValue< bool >(tbl, "enabled");

            if (!enabled) {
                // std::cout << "No enabled. Defaulting to true\n";
            }
            // else if (*enabled == false) {
            //     // log
            //     continue;
            // }

            ServiceProxy proxy
            {
                *name,
                ignoreSslErrors.value_or(false),
                enabled.value_or(true), // @todo incorrect, change later! Need to keep disabled proxies for ui
                *networkProxy
            };

            auto dublicatedNameIt = std::ranges::find_if(result, [&name](const auto& proxy) { return proxy.Name == name; });
            bool isNameDublicated = dublicatedNameIt != result.end();

            if (isNameDublicated) {
                // log
                *dublicatedNameIt = std::move(proxy);
            } else {
                result.push_back(std::move(proxy));
            }

            // log
        }
    }

    if (result.isEmpty()) {
        qDebug() << "No proxies have been added";
    }

    return result;
}

ConfigPtr CreateDefaultConfigNoHotkeys()
{
    static_assert(static_cast< int >(ActionType::Max) == 19, "Add default hotkey value");

    auto config = std::make_shared< Config >();

    // [General]
    config->excludeFromCapture = true;
    config->saveScreenshots    = true;

    // [Logging]
    config->asyncMode    = true;
    config->maxQueueSize = 8192;
    config->minLevel     = LogLevel_Trace;

    // [Proxies]
    config->proxies = { };

    // [Hotkeys]
    config->hotkeys = { };

    // other
    config->mode = LoadMode_Default;
    config->path = "";

    return config;
}

}


ConfigPtr AppConfig::Config::clone() const
{
    return std::make_shared< Config >(*this);
}


QString AppConfig::Config::toStringDebug() const
{
    static_assert(static_cast< int >(ActionType::Max) == 19, "Add hotkey string representation");

    QString result;

    QTextStream out(&result);

    out << "AppConfig (from: '" << path << "', mode: " << ToQString(mode) << ")\n";

    out << "  [General]\n";
    out << "    excludeFromCapture: " << (excludeFromCapture ? "true" : "false") << "\n";
    out << "    saveScreenshots: "    << (saveScreenshots    ? "true" : "false") << "\n";

    out << "  [Logging]\n";
    out << "    asyncMode: "    << (asyncMode ? "true" : "false") << "\n";
    out << "    maxQueueSize: " << maxQueueSize << "\n";
    out << "    minLevel: "     << ToQString(minLevel) << "\n";

    out << "  [Hotkeys]\n";

    if (hotkeys.isEmpty()) {
        out << "    (No hotkeys defined)\n";
    } else {
        for (const auto& hotkey : hotkeys) {
            out << "    "
                << ToQString(hotkey.Action).leftJustified(25) // Pad for alignment
                << ": "
                << hotkey.Shortcut.toString(QKeySequence::NativeText)
                << "\n";
        }
    }

    return result;
}


ConfigPtr AppConfig::CreateDefaultConfig()
{
    return CreateDefaultConfig("", ConfigLoadMode::LoadMode_Default);
}


ConfigPtr AppConfig::CreateDefaultConfig(const QString& path, ConfigLoadMode mode)
{
    static_assert(static_cast< int >(ActionType::Max) == 19, "Add default hotkey value");

    auto config = CreateDefaultConfigNoHotkeys();

    // [Hotkeys]
    config->hotkeys = {
       { ActionType::Quit,               QKeySequence("Ctrl+Q"),         true },
       { ActionType::ToggleVisibility,   QKeySequence("Ctrl+H"),         true },
       { ActionType::TogglePageMode,     QKeySequence("Ctrl+K"),         true },
       { ActionType::ToggleLogPanel,     QKeySequence("Ctrl+L"),         true },
       { ActionType::TakeScreenshot,     QKeySequence("Ctrl+S"),         true },
       { ActionType::Crop,               QKeySequence("Ctrl+O"),         true },
       { ActionType::MoveUp,             QKeySequence("Ctrl+Up"),        true },
       { ActionType::MoveDown,           QKeySequence("Ctrl+Down"),      true },
       { ActionType::MoveLeft,           QKeySequence("Ctrl+Left"),      true },
       { ActionType::MoveRight,          QKeySequence("Ctrl+Right"),     true },
       { ActionType::UndoLastAction,     QKeySequence("Ctrl+Backspace"), true },
       { ActionType::SendCurrentRequest, QKeySequence("Ctrl+Enter"),     true },
       { ActionType::ReloadConfig,       QKeySequence("Ctrl+R"),         true },
    };

    config->path = path;
    config->mode = mode;

    return config;
}


ConstConfigPtr AppConfig::GetDefaultConfig()
{
    static ConfigPtr s_config = CreateDefaultConfig();
    return s_config;
}


static ConfigPtr LoadConfigFromTable(const toml::table& tbl,
                                     ConfigLoadMode mode,
                                     Optional< QString > path = std::nullopt,
                                     QPromise< ConfigPtr >* promise = nullptr)
{
    static_assert(static_cast< int >(LoadMode_Max) == 3, "Add support for another load mode");

#define SET_VALUE(value, key)                                                           \
    if (promise && promise->isCanceled()) {                                             \
        qWarning() << "App config loading canceled";                                    \
        return nullptr;                                                                 \
    }                                                                                   \
    auto value = ReadValue< decltype(Config::value) >(tbl, key, mode);                  \
    if (!value) {                                                                       \
        switch (mode)                                                                   \
        {                                                                               \
            case LoadMode_Default:                                                      \
                return nullptr;                                                         \
            case LoadMode_Overload:                                                     \
                return CreateDefaultConfig(path.value_or(""), mode);                    \
            default:                                                                    \
                break;                                                                  \
        }                                                                               \
    }

    SET_VALUE(excludeFromCapture, "General.excludeFromCapture")
    SET_VALUE(saveScreenshots,    "General.saveScreenshots")

    SET_VALUE(asyncMode,          "Logging.asyncMode")
    SET_VALUE(maxQueueSize,       "Logging.maxQueueSize")
    SET_VALUE(minLevel,           "Logging.minLevel")

    SET_VALUE(proxies,            "Proxies")

    SET_VALUE(hotkeys,            "Hotkeys")

    auto config = CreateDefaultConfigNoHotkeys();

    config->excludeFromCapture = *excludeFromCapture;
    config->saveScreenshots    = *saveScreenshots;

    config->asyncMode          = *asyncMode;
    config->maxQueueSize       = *maxQueueSize;
    config->minLevel           = *minLevel;

    config->proxies            = *proxies;

    config->hotkeys            = *hotkeys;

    config->path               = path.value_or("");
    config->mode               = mode;

#undef SET_VALUE

    return config;
}


static ConfigPtr LoadConfigFromDataImpl(const QString& data, ConfigLoadMode mode, QPromise< ConfigPtr >* promise = nullptr)
{
    auto parsedConfig = toml::parse(data.toStdString(), std::string_view { "stdin" });

    if (!parsedConfig) {
        qCritical() << "Error with parsing app config from data. Description: " << parsedConfig.error().description() << ")";
        return (mode == LoadMode_Default) ? nullptr : CreateDefaultConfig();
    }

    return LoadConfigFromTable(parsedConfig.table(), mode, std::nullopt, promise);
}


static ConfigPtr LoadConfigFromFileImpl(const QString& path, ConfigLoadMode mode, QPromise< ConfigPtr >* promise = nullptr)
{
    if (!QFile::exists(path)) {
        qWarning() << "AppConfig file not found at:" << path;
        switch (mode) {
        case LoadMode_Default:
            qInfo() << "File doesn't exist, default mode is chosen, returning nullptr";
            return nullptr;
        case LoadMode_Overload:
        case LoadMode_PartialOverload:
            qInfo() << "Using default application configuration as app config file doesn't exist.";
            return CreateDefaultConfig();
        default:
            qCritical() << "Unrecognized ConfigLoadMode, software issue. Contact developers immediately.";
            return nullptr;
        }
    }

    auto parsedConfig = toml::parse_file(path.toStdString());

    if (!parsedConfig) {
        qCritical() << "Error with parsing app config file at " << path << "(description:" << parsedConfig.error().description() << ")";
        return (mode == LoadMode_Default) ? nullptr : CreateDefaultConfig();
    }

    return LoadConfigFromTable(parsedConfig.table(), mode, path, promise);
}


ConfigPtr AppConfig::LoadConfigFromData(const QString& data, ConfigLoadMode mode)
{
    return LoadConfigFromDataImpl(data, mode);
}


QFuture< ConfigPtr > AppConfig::LoadConfigFromDataAsync(const QString& data, ConfigLoadMode mode)
{
    return QtConcurrent::run([=](QPromise< ConfigPtr >& promise) { promise.addResult(LoadConfigFromDataImpl(data, mode, &promise)); });
}


ConfigPtr AppConfig::LoadConfigFromFile(const QString& path, ConfigLoadMode mode)
{
    return LoadConfigFromFileImpl(path, mode);
}


QFuture< ConfigPtr > AppConfig::LoadConfigFromFileAsync(const QString& path, ConfigLoadMode mode)
{
    return QtConcurrent::run([=](QPromise< ConfigPtr >& promise) { promise.addResult(LoadConfigFromFileImpl(path, mode, &promise)); });
}


bool AppConfig::SaveConfigToFile(const QString& path, ConstConfigPtr config)
{
    if (!config) {
        qWarning() << "SaveConfigToFile failed: provided config pointer is null.";
        return false;
    }

    toml::array hotkeysArray;
    for (const auto& hotkey : config->hotkeys) {
        hotkeysArray.push_back(toml::table {
            { "action",   ToQString(hotkey.Action).toStdString() },
            { "shortcut", hotkey.Shortcut.toString(QKeySequence::PortableText).toStdString() },
            { "enabled",  hotkey.Enabled }
        });
    }

    toml::array proxiesArray;
    for (const auto& proxy : config->proxies) {
        proxiesArray.push_back(toml::table {
            { "name",            proxy.Name.toStdString() },
            { "endpoint",        ::NetworkProxyToEndpoint< std::string >(proxy.NetworkProxy) },
            { "enabled",         proxy.Enabled },
            { "ignoreSslErrors", proxy.IgnoreSslErrors },
        });
    }

    auto root = toml::table {
        { "General", toml::table {
                        { "excludeFromCapture", config->excludeFromCapture },
                        { "saveScreenshots",    config->saveScreenshots }
                    }
        },
        { "Logging", toml::table {
                        { "asyncMode",    config->asyncMode },
                        { "maxQueueSize", config->maxQueueSize },
                        { "minLevel",     EnumToConfigName(config->minLevel) }
                    }
        },
        { "Proxies", proxiesArray },
        { "Hotkeys", hotkeysArray }
    };

    QFileInfo fileInfo(path);
    if (!QDir().mkpath(fileInfo.absolutePath())) {
        qError() << "Failed to create directory " << fileInfo.absolutePath();
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for writing:" << path;
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    std::stringstream ss;
    ss << root;

    out << QString::fromStdString(ss.str());

    return true;
}
