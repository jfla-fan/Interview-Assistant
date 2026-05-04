#include "inja_renderer.h"

#include "log/log.h"

#include <nlohmann/json.hpp>

#include <inja/inja.hpp>
#include <inja/exceptions.hpp>

#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonArray>
#include <QJsonObject>


using namespace ServiceConfig;

namespace
{

using json = nlohmann::json;

Optional< LogLevel > ToLogLevel(std::string_view lvl)
{
    static_assert(LogLevel_Max == 6);

    static QHash< std::string_view, LogLevel > lvlMap =
    {
        { "trace",   LogLevel_Trace   },
        { "debug",   LogLevel_Debug   },
        { "info",    LogLevel_Info    },
        { "warning", LogLevel_Warning },
        { "error",   LogLevel_Error   },
        { "fatal",   LogLevel_Fatal   },
    };

    auto it = lvlMap.find(lvl);

    return it == lvlMap.cend() ? std::nullopt : std::make_optional(it.value());
}

inline bool IsNonQuoteString(const CString& data)
{
    auto firstChar = std::find_if(data.cbegin(), data.cend(), [](QChar c) {
        return !c.isSpace();
    });

    if (firstChar == data.cend()) {
        return false;
    }

    return !(*firstChar == '{' || *firstChar == '[' || *firstChar == '"');
}


json ToJson(const QVariant& value)
{
    if (!value.isValid())
        return nullptr;

    switch (value.typeId()) {
    case QMetaType::Int:
        return value.toInt();
    case QMetaType::Bool:
        return value.toBool();
    case QMetaType::Double:
        return value.toDouble();
    case QMetaType::QString:
        return value.toString().toStdString();
    }

    if (value.canConvert(QMetaType::QVariantList)) {
        auto list = value.toList();
        json arr = json::array();
        for (const auto& item : list)
            arr.push_back(ToJson(item));

        return arr;
    }

    if (value.canConvert(QMetaType::QVariantMap)) {
        auto map = value.toMap(); // or to Hash?
        json obj = json::object();
        for (auto it = map.begin(); it != map.end(); ++it) {
            obj[it.key().toStdString()] = ToJson(it.value());
        }

        return obj;
    }

    return json(nullptr);
}


json ToJson(const GlobalVars& vars)
{
    json j = json::object();

    for (auto it = vars.begin(); it != vars.end(); ++it) {
        const GlobalVar& var = it.value();
        j[it.key().toStdString()] = ToJson(var.Value);
    }

    return j;
}


QVariant FromJson(const json& value)
{
    try
    {
        if (value.is_number_integer())
        {
            return static_cast< qint64 >(value.get< int64_t >());
        } else if (value.is_number_float())
        {
            return value.get< double >();
        } else if (value.is_boolean())
        {
            return value.get< bool >();
        } else if (value.is_array())
        {
            QVariantList list;
            for (const auto& item : value) {
                list.append(FromJson(item));
            }

            return list;
        } else if (value.is_object())
        {
            QVariantMap map;
            for (auto it = value.begin(); it != value.end(); ++it) {
                map[QString::fromStdString(it.key())] = FromJson(it.value());
            }

            return map;
        } else
        {
            return QString::fromStdString(value.get< std::string >());
        }
    }
    catch (const std::exception& ex)
    {
        qCritical() << "Failed to parse from json, details: " << ex.what();
        return {};
    }
}

json GetJsonPath(const json& j, const std::string& path) {
    std::istringstream iss(path);
    std::string token;
    auto current = j;

    while (std::getline(iss, token, '.')) {
        if (token.empty()) continue;

        if (current.is_object() && current.contains(token)) {
            current = current[token];
        } else if (current.is_array()) {
            try {
                size_t index = std::stoul(token);
                if (index < current.size()) {
                    current = current[index];
                } else {
                    return nullptr; // out of bounds
                }
            } catch (...) {
                return nullptr; // invalid index
            }
        } else {
            return nullptr; // key not found
        }
    }

    // qWarning().noquote() << "Got json path: " << current.dump(4);

    return current;
}

}

class ServiceConfig::InjaRendererPrivate
{
public:
    mutable inja::Environment m_environment;
};


InjaRenderer::InjaRenderer()
    : d_ptr(std::make_unique< InjaRendererPrivate >())
{ }


InjaRenderer::~InjaRenderer() = default;


bool InjaRenderer::initialize()
{
    Q_D(InjaRenderer);

    d->m_environment.add_callback("concat", [](inja::Arguments& args) {
        json::string_t result;

        for (const auto* arg : args) {
            if (arg->is_string()) {
                result += arg->get_ref< const json::string_t& >();
            }
            else {
                result += arg->dump();
            }
        }

        return result;
    });

    d->m_environment.add_callback("json_parse", [](inja::Arguments& args) {
        return json::parse(args.at(0)->get_ref< const json::string_t& >());
    });

    d->m_environment.add_callback("get", 2, [](inja::Arguments& args) {
        const auto& obj = *args[0];
        const auto& path = args[1]->get_ref< const json::string_t& >();

        return GetJsonPath(obj, path);
    });

    d->m_environment.add_callback("json_extract", [](inja::Arguments& args) {
        const auto& src = args.at(0);
        const auto& path = args.at(1)->get_ref< const json::string_t& >();

        json parsed;

        // non-parsed
        if (src->is_string()) {
            try {
                parsed = json::parse(src->get_ref< const json::string_t& >());
            } catch (const json::parse_error& e) {
                qDebug() << "JSON parse error in json_extract:" << e.what();
                return json(nullptr);
            }
        } else {
            // Already a json object/array
            parsed = *src;
        }

        // qDebug().noquote() << "JsonExtract parsed: " << parsed.dump(4);

        return GetJsonPath(parsed, path);
    });

    d->m_environment.add_callback("json_escape", [](inja::Arguments& args) {
        // qDebug().noquote() << "JSON ESCAPE argument: " << args.front()->dump();
        return args.front()->dump();
    });

    d->m_environment.add_void_callback("log", 2, [](inja::Arguments& args) {
        const auto& lvl = args.front()->get_ref< const json::string_t& >();
        const auto mappedLvl = ToLogLevel(lvl);

        if (!mappedLvl) {
            INJA_THROW(inja::RenderError(fmt::format("log callback error: unrecognized log lvl - {}.", lvl), {}));
        }

        LOG_IMPL(*mappedLvl) << args.at(1)->get_ref< const json::string_t& >();
    });

    d->m_environment.add_void_callback("log_trace", 1, [](inja::Arguments& args) {
        LOG_TRACE() << args.front()->get_ref< const json::string_t& >();
    });

    d->m_environment.add_void_callback("log_debug", 1, [](inja::Arguments& args) {
        LOG_DEBUG() << args.front()->get_ref< const json::string_t& >();
    });

    d->m_environment.add_void_callback("log_info", 1, [](inja::Arguments& args) {
        LOG_INFO() << args.front()->get_ref< const json::string_t& >();
    });

    d->m_environment.add_void_callback("log_warning", 1, [](inja::Arguments& args) {
        LOG_WARNING() << args.front()->get_ref< const json::string_t& >();
    });

    d->m_environment.add_void_callback("log_error", 1, [](inja::Arguments& args) {
        LOG_ERROR() << args.front()->get_ref< const json::string_t& >();
    });

    d->m_environment.add_void_callback("log_fatal", 1, [](inja::Arguments& args) {
        LOG_FATAL() << args.front()->get_ref< const json::string_t& >();
    });

    return true;
}


Optional< CString > InjaRenderer::renderWithGlobalVars(const CString& document, const GlobalVars& vars) const
{
    const Q_D(InjaRenderer);

    std::string rendered;
    try {
        auto jsonVars = ToJson(vars);
        // qWarning().noquote() << "jsonVars: " << jsonVars.dump(4);
        rendered = d->m_environment.render(document.toStdString(), jsonVars);
    } catch (const std::exception& e) {
        qWarning() << "Inja rendering failed:" << e.what();
        return {};
    }

    return QString::fromStdString(rendered);
}


Optional< CString > InjaRenderer::renderWithJsonVars(const CString& document, const CString& jsonData, const GlobalVars& vars) const
{
    const Q_D(InjaRenderer);

    try {
        auto parsedJson = json::parse(jsonData.toStdString());
        auto parsedVars = ToJson(vars);

        parsedJson.merge_patch(parsedVars);
        std::string rendered = d->m_environment.render(document.toStdString(), parsedJson);

        return QString::fromStdString(rendered);
    }
    catch (const inja::InjaError& ex) {
        qCritical() << "Failed to render: " << ex.what();
        return {};
    }
    catch (const std::exception& ex) {
        qCritical() << "Failed to parse json, details: " << ex.what();
        return {};
    }
}


QVariant InjaRenderer::parse(const CString& document) const
{
    const Q_D(InjaRenderer);

    try {
        auto parsedJson = json::parse(document.toStdString());
        return FromJson(parsedJson);
    } catch (const std::exception& ex) {

        // nlohman::json treats strings without quotes as invalid
        // we are gonna treat them as regular strings
        if (IsNonQuoteString(document)) {
            return document;
        }

        qCritical() << "Failed to parse json, details: " << ex.what();
        return {};

    }
}


RendererPtr InjaRenderer::clone() const
{
    Q_D(const InjaRenderer);

    auto newRenderer = std::make_shared< InjaRenderer >();
    newRenderer->initialize();

    return newRenderer;
}


Optional< CString > InjaRenderer::renderWithJsonVars(const CString& document, const QJsonObject& variables) const
{
    const Q_D(InjaRenderer);

    try {
        auto jsonVars = ToJson(variables);
        return QString::fromStdString(d->m_environment.render(document.toStdString(), jsonVars));
    } catch (const std::exception& e) {
        qWarning() << "Inja rendering failed:" << e.what();
        return {};
    }
}
