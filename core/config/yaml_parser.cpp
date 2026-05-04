#include "yaml_parser_p.h"

using namespace ServiceConfig;


namespace {

template< typename StructType >
CStorage< StructType > extractMapSection(const YAML::Node& parent, const std::string& sectionKey, BaseNode* env = nullptr);

template< typename StructType >
CStorage< StructType > extractSequenceSection(const YAML::Node& parent, const std::string& sectionKey, BaseNode* env = nullptr);

template< typename T >
struct ParserTraits;

// Helper to extract GlobalVar
template<>
struct ParserTraits< GlobalVar > {
    static Optional< GlobalVar > parse(const YAML::Node& node, const QString& name, BaseNode* env = nullptr) {
        if (!node) return {};

        GlobalVar result;
        result.Name = name;
        // in config we treat variables as strings only for now
        result.Value = QString::fromStdString(node.as< std::string >());

        return result;
    }
};


// Helper to extract Proxy
template<>
struct ParserTraits< Proxy > {
    static Optional< Proxy > parse(const YAML::Node& node, const QString& name, BaseNode* env = nullptr) {
        if (!node || !node.IsMap()) return {};

        Proxy result;
        result.Name = name;

        if (const YAML::Node& endpoint = node["endpoint"]) {
            result.Endpoint = QString::fromStdString(endpoint.as< std::string >());
        } else {
            qCritical() << "Failed to parse proxy endpoint, name - " << name;
            return std::nullopt;
        }

        return result;
    }
};


// Helper to extract Reply
template<>
struct ParserTraits< Request > {
    static Optional< Request > parse(const YAML::Node& node, BaseNode* env = nullptr) {
        if (!node || !node.IsMap()) return {};

        Request request;
        request.Name = "Request";

        if (const YAML::Node& selector = node["selector"])
            request.Selector = QString::fromStdString(selector.as< std::string >());
        else {
            qCritical() << "Failed to parse display request reply selector";
            return std::nullopt;
        }

        return request;
    }
};


// Helper to extract Reply
template<>
struct ParserTraits< Reply > {
    static Optional< Reply > parse(const YAML::Node& node, BaseNode* env = nullptr) {
        if (!node || !node.IsMap()) return {};

        Reply reply;
        reply.Name = "Reply";

        if (const YAML::Node& selector = node["selector"])
            reply.Selector = QString::fromStdString(selector.as< std::string >());
        else {
            qCritical() << "Failed to parse http request reply selector";
            return std::nullopt;
        }

        return reply;
    }
};


// Helper to extract Reply
template<>
struct ParserTraits< Display > {
    static Optional< Display > parse(const YAML::Node& node, BaseNode* env = nullptr) {
        if (!node || !node.IsMap()) return {};

        Display display;
        display.Name = "Display";

        if (const YAML::Node& request = node["request"]) {
            if (auto parsedRequest = ParserTraits< Request >::parse(request, env))
                display.Request = *parsedRequest;
            else
                return std::nullopt;
        }
        else {
            qCritical() << "Failed to parse display request selector";
            return std::nullopt;
        }

        if (const YAML::Node& reply = node["reply"]) {
            if (auto parsedReply = ParserTraits< Reply >::parse(reply, env))
                display.Reply = *parsedReply;
            else
                return std::nullopt;
        }
        else {
            qCritical() << "Failed to parse display reply selector";
            return std::nullopt;
        }

        return display;
    }
};


// Helper to extract HttpRequest
template<>
struct ParserTraits< HttpRequest > {
    static Optional< HttpRequest > parse(const YAML::Node& node, const QString& name, BaseNode* env = nullptr) {
        if (!node || !node.IsMap()) return {};

        HttpRequest result;
        result.Name = name;

        if (const YAML::Node& headers = node["headers"])
            result.Headers = QString::fromStdString(headers.as< std::string >());

        if (const YAML::Node& body = node["body"])
            result.Body = QString::fromStdString(body.as< std::string >());
        else {
            qCritical() << "Failed to parse http request body, name - " << name;
            return std::nullopt;
        }

        if (const YAML::Node& enabled = node["enabled"])
            result.Enabled = QString::fromStdString(enabled.as< std::string >());
        else
            result.Enabled = "true";

        if (const YAML::Node& method = node["method"])
            result.Method = QString::fromStdString(method.as< std::string >());
        else {
            qCritical() << "Failed to parse http request method, name - " << name;
            return std::nullopt;
        }

        if (const YAML::Node& useProxies = node["useProxies"])
            result.UseProxies = QString::fromStdString(useProxies.as< std::string >());
        else {
            qCritical() << "Failed to parse http request use proxies, name - " << name;
            return std::nullopt;
        }

        if (const YAML::Node& ignoreSslErrors = node["ignoreSslErrors"])
            result.IgnoreSslErrors = QString::fromStdString(ignoreSslErrors.as< std::string >());
        else {
            qCritical() << "Failed to parse http request ignore ssl errors, name - " << name;
            return std::nullopt;
        }

        if (const YAML::Node& url = node["url"])
            result.Url = QString::fromStdString(url.as< std::string >());
        else if (auto* service = dynamic_cast< Service* >(env)){
            result.Url = service->BaseUrl;
        } else {
            qCritical() << "Failed to parse http request url, name - " << name;
            return std::nullopt;
        }

        if (const YAML::Node& display = node["display"]) {
            auto parsedDisplay = ParserTraits< Display >::parse(display);
            if (parsedDisplay)
                result.Display = *parsedDisplay;
        }

        // we set default name to the reply if it is parsed correctly
        if (result.Display.Reply.Name.isEmpty()) {
            qCritical() << "Failed to parse http request reply, name - " << name;
            return std::nullopt;
        }

        return result;
    }
};


// Helper to extract Service
template<>
struct ParserTraits< Service > {
    static Optional< Service > parse(const YAML::Node& node, const QString& name, BaseNode* env = nullptr) {
        if (!node || !node.IsMap()) return {};

        Service result;
        result.Name = name;

        if (const YAML::Node& urlTemplate = node["baseUrl"]) {
            result.BaseUrl = QString::fromStdString(urlTemplate.as< std::string >());
        }

        result.Requests = extractSequenceSection< HttpRequest >(node, "requests", &result);

        return result;
    }
};


// Helper to extract ServiceProvider
template<>
struct ParserTraits< ServiceProvider > {
    static Optional< ServiceProvider > parse(const YAML::Node& node, const QString& name, BaseNode* env = nullptr) {
        if (!node || !node.IsMap()) return {};

        ServiceProvider result;
        result.Name = name;
        result.Services = extractSequenceSection< Service >(node, "services", &result);

        return result;
    }
};


// Generic extractor for CStorage<T>
template< typename StructType >
CStorage< StructType > extractMapSection(const YAML::Node& parent, const std::string& sectionKey, BaseNode* env) {
    CStorage< StructType > result;
    if (!parent[sectionKey]) return result;

    YAML::Node section = parent[sectionKey];
    if (!section.IsMap()) return result;

    for (const auto& pair : section) {
        if (!pair.first || !pair.second) continue;

        QString name = QString::fromStdString(pair.first.as< std::string >());
        if (auto val = ParserTraits< StructType >::parse(pair.second, name)) {
            result.insert({ val->Name, *val });
            // result.insert(val->Name, *val);
        }
    }

    return result;
}


template< typename StructType >
CStorage< StructType > extractSequenceSection(const YAML::Node& parent, const std::string& sectionKey, BaseNode* env) {
    CStorage< StructType > result;
    if (!parent[sectionKey]) return result;

    YAML::Node section = parent[sectionKey];
    if (!section.IsSequence()) return result;

    for (const auto& item : section) {
        if (!item.IsMap()) continue;

        QString name;
        if (const YAML::Node& n = item["name"]) {
            name = QString::fromStdString(n.as<std::string>());
        } else {
            continue; // skip unnamed items
        }

        if (auto val = ParserTraits< StructType >::parse(item, name, env)) {
            result.insert({ val->Name, *val });
            // result.insert(val->Name, *val);
        }
    }

    return result;
}

} // anonymous namespace


YamlParser::YamlParser()
    : d_ptr(new YamlParserPrivate)
{ }


YamlParser::~YamlParser() = default;


bool YamlParser::load(QString input) {
    Q_D(YamlParser);

    try {
        d->m_parsedConfig = YAML::Load(input.toStdString());
        d->m_strConfig = std::move(input);
    } catch (const YAML::ParserException& ex) {
        qDebug() << "YAML Parser Exception: " << ex.what();
        return false;
    }

    return true;
}


bool YamlParser::preprocess() {
    Q_D(YamlParser);

    auto templates = d->parseTemplates();
    auto expandedNode = d->expandNode(d->m_parsedConfig, templates);

    if (!expandedNode) {
        qDebug() << "Failed to preprocess";
        return false;
    }

    d->m_parsedConfig = std::move(expandedNode);
    d->m_strConfig = d->nodeToString(d->m_parsedConfig);

    return true;
}


Optional< Root > YamlParser::dump() {
    Q_D(YamlParser);

    Root root;

    root.GlobalVariables = extractMapSection< GlobalVar >(d->m_parsedConfig, "globals");
    root.Proxies = extractSequenceSection< Proxy >(d->m_parsedConfig, "proxies");
    root.ServiceProviders = extractSequenceSection< ServiceProvider >(d->m_parsedConfig, "serviceProviders");

    return root;
}


const QString& YamlParser::getConfig() const {
    const Q_D(YamlParser);
    return d->m_strConfig;
}


QVariant YamlParser::parse(const QString& input) const
{
    try {
        YAML::Node node = YAML::Load(input.toStdString());

        // Helper lambda to recursively convert YAML::Node to QVariant
        std::function< QVariant(const YAML::Node&) > convertNode = [&](const YAML::Node& n) -> QVariant {
            switch (n.Type()) {
                case YAML::NodeType::Scalar: {
                    QString value = QString::fromStdString(n.as<std::string>());

                    // Try parsing bool
                    if (value.compare("true", Qt::CaseInsensitive) == 0)
                        return QVariant(true);
                    if (value.compare("false", Qt::CaseInsensitive) == 0)
                        return QVariant(false);

                    // Try parsing integer
                    bool ok;
                    qint64 iVal = value.toLongLong(&ok);
                    if (ok) return QVariant(iVal);

                    // Try parsing double
                    double dVal = value.toDouble(&ok);
                    if (ok) return QVariant(dVal);

                    // Default: string
                    return QVariant(value);
                }

                case YAML::NodeType::Sequence: {
                    QVariantList list;
                    for (const auto& item : n) {
                        list.append(convertNode(item));
                    }
                    return QVariant(list);
                }

                case YAML::NodeType::Map: {
                    QVariantHash hash;
                    for (const auto& pair : n) {
                        QString key = QString::fromStdString(pair.first.as< std::string >());
                        hash.insert(key, convertNode(pair.second));
                    }
                    return QVariant(hash);
                }

                case YAML::NodeType::Null:
                    return QVariant();

                case YAML::NodeType::Undefined:
                    return QVariant();
            }

            return QVariant();
        };

        return convertNode(node);
    } catch (const YAML::Exception& e) {
        qWarning() << "Failed to parse YAML:" << e.what();
        return QVariant();
    }
}


void YamlParser::printParsedConfigDebug(const QVariant& value, int indent)
{
    QString indentStr(indent, ' ');

    // Helper to print key-value pairs inside maps
    auto printKeyVal = [&](const QString& key, const QVariant& val) {
        qDebug() << qPrintable(indentStr + key + ":");
        printParsedConfigDebug(val, indent + 2);
    };

    if (value.typeId() == QMetaType::QVariantHash) {
        QVariantHash map = value.toHash();
        if (map.isEmpty()) {
            qDebug() << qPrintable(indentStr + "{}");
            return;
        }
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            printKeyVal(it.key(), it.value());
        }
    }
    else if (value.typeId() == QMetaType::QVariantList) {
        QVariantList list = value.toList();
        if (list.isEmpty()) {
            qDebug() << qPrintable(indentStr + "[]");
            return;
        }
        for (const auto& item : list) {
            qDebug() << qPrintable(indentStr + "- ");
            printParsedConfigDebug(item, indent + 2);
        }
    }
    else if (value.typeId() == QMetaType::QString) {
        qDebug() << qPrintable(indentStr + value.toString());
    }
    else if (value.typeId() == QMetaType::Int ||
             value.typeId() == QMetaType::LongLong) {
        qDebug() << qPrintable(indentStr + QString::number(value.toLongLong()));
    }
    else if (value.typeId() == QMetaType::Double) {
        qDebug() << qPrintable(indentStr + QString::number(value.toDouble()));
    }
    else if (value.typeId() == QMetaType::Bool) {
        qDebug() << qPrintable(indentStr + (value.toBool() ? "true" : "false"));
    }
    else if (value.isNull()) {
        qDebug() << qPrintable(indentStr + "null");
    }
    else {
        qDebug() << qPrintable(indentStr + "[Unsupported type: " +
                               QString(value.typeName()) + "]");
    }
}

YAML::Node YamlParser::getConfigDebug() const {
    const Q_D(YamlParser);
    return d->m_parsedConfig;
}

QString YamlParser::nodeToStringDebug(const YAML::Node& node) const
{
    const Q_D(YamlParser);
    return d->nodeToString(node);
}

ParserPtr YamlParser::clone() const
{
    Q_D(const YamlParser);

    auto newParser = std::make_shared< YamlParser >();

    newParser->d_ptr->m_strConfig = d->m_strConfig;
    newParser->d_ptr->m_parsedConfig = d->m_parsedConfig;

    return newParser;
}


///////////////////////////////////////////////////////////////////

CStorage< YAML::Node > YamlParserPrivate::parseTemplates() {
    CStorage< YAML::Node > result;

    if (m_parsedConfig["templates"]) {
        for (const auto& item : m_parsedConfig["templates"]) {
            result.emplace(QString::fromStdString(item.first.Scalar()), item.second);
        }
    }

    return result;
}


YAML::Node YamlParserPrivate::expandNode(const YAML::Node& node, const CStorage< YAML::Node >& templates) const {
    if (!node.IsMap()) return node;

    // Check if this node has an 'extends' reference
    if (node["extends"]) {
        QString templateName = QString::fromStdString(node["extends"].as< std::string >());

        // Get base node from registry or use as-is if not found
        YAML::Node base = templates.contains(templateName) ? templates.at(templateName) : YAML::Node();

        // Recursively expand base node first
        YAML::Node expanded_base = expandNode(base, templates);

        // Merge and return
        return mergeNodes(expanded_base, node, templates);
    }

    // Recursively expand all child nodes
    YAML::Node result = YAML::Clone(node);
    for (const auto& kv : node) {
        const std::string key = kv.first.as<std::string>();
        const YAML::Node& value = kv.second;

        if (value.IsMap()) {
            result[key] = expandNode(value, templates);
        } else if (value.IsSequence()) {
            YAML::Node seq = YAML::Clone(value);
            for (size_t i = 0; i < seq.size(); ++i) {
                seq[i] = expandNode(seq[i], templates);
            }
            result[key] = seq;
        }
    }

    return result;
}


YAML::Node YamlParserPrivate::mergeNodes(const YAML::Node& base, const YAML::Node& overrides, const CStorage< YAML::Node >& templates) const
{
    using namespace YAML;

    if (!overrides) return base;

    if (base.Type() != NodeType::Map || overrides.Type() != NodeType::Map) {
        // Handle scalar values
        if (base.IsScalar() && overrides.IsScalar()) {
            std::string base_str = base.as<std::string>();
            std::string override_str = overrides.as<std::string>();

            // Check for append marker
            if (override_str.rfind("++append++", 0) == 0) {
                return Node(base_str + override_str.substr(10));
            }
            // Optionally add prepend logic
            else if (override_str.rfind("++prepend++", 0) == 0) {
                return Node(override_str.substr(11) + base_str);
            }
        }
        return overrides;
    }

    Node result = Clone(base);

    for (const auto& override_pair : overrides) {
        std::string key = override_pair.first.as<std::string>();
        const Node& override_value = override_pair.second;

        if (!base[key]) {
            result[key] = override_value;
            continue;
        }

        const Node& base_value = base[key];

        if (base_value.Type() == NodeType::Map && override_value.Type() == NodeType::Map) {
            result[key] = mergeNodes(base_value, override_value, templates);
        }
        else if (base_value.Type() == NodeType::Sequence && override_value.Type() == NodeType::Sequence) {
            Node merged_seq = Clone(base_value);
            for (size_t i = 0; i < override_value.size(); ++i) {
                const Node& override_item = override_value[i];
                bool found = false;
                for (size_t j = 0; j < merged_seq.size(); ++j) {
                    const Node& base_item = merged_seq[j];
                    if (override_item["name"] && base_item["name"] &&
                        override_item["name"].as<std::string>() == base_item["name"].as<std::string>()) {
                        merged_seq[j] = mergeNodes(base_item, expandNode(override_item, templates), templates);
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    merged_seq.push_back(expandNode(override_item, templates));
                }
            }
            result[key] = merged_seq;
        }
        else {
            // Apply string merge strategies
            if (base_value.IsScalar() && override_value.IsScalar()) {
                std::string base_str = base_value.as<std::string>();
                std::string override_str = override_value.as<std::string>();

                if (override_str.rfind("++append++", 0) == 0) {
                    result[key] = base_str + override_str.substr(10);
                } else if (override_str.rfind("++prepend++", 0) == 0) {
                    result[key] = override_str.substr(11) + base_str;
                } else {
                    result[key] = override_str;
                }
            } else {
                result[key] = override_value;
            }
        }
    }

    return result;
}


QString YamlParserPrivate::nodeToString(const YAML::Node& node) const {
    YAML::Emitter emitter;
    emitter << node;

    if (!emitter.good()) {
        qDebug() << "Emitter failed state";
    }

    return emitter.c_str();
}
