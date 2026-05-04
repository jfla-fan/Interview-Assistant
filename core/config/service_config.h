#pragma once

#include "../common.h"

#include <tsl/ordered_map.h>

#include <QVariant>
#include <QString>
#include <QFuture>
#include <QHostAddress>
#include <memory>


namespace ServiceConfig
{

using CString = QString;

template< class T >
using CStorage = tsl::ordered_map< CString, T >;

struct GlobalVar;
using GlobalVars = CStorage< GlobalVar >;

struct BaseNode {
    virtual ~BaseNode() = default;
    CString Name;
};

struct GlobalVar : BaseNode {
    using VarData = QVariant;
    VarData Value;

    static GlobalVar fromValue(const QString& name, VarData data) {
        GlobalVar r;
        r.Name = name;
        r.Value = std::move(data);
        return r;
    };
};

struct Proxy : BaseNode {
    CString Endpoint;
};

struct Request : BaseNode {
    CString Selector;
};

struct Reply : BaseNode {
    CString Selector;
};

struct Display : BaseNode {
    Request Request;
    Reply Reply;
};

struct HttpRequest : BaseNode {
    CString Url;
    CString Headers;
    CString Body;
    CString Method;
    CString Enabled;
    CString UseProxies;
    CString IgnoreSslErrors;
    Display Display;
};

struct Service : BaseNode {
    CString BaseUrl;
    CStorage< HttpRequest > Requests;
};

struct ServiceProvider : BaseNode {
    CStorage< Service > Services;
};

struct Root : BaseNode {
    GlobalVars GlobalVariables;
    CStorage< Proxy > Proxies;
    CStorage< ServiceProvider > ServiceProviders;
};


class IConfigParser;
using ParserPtr = std::shared_ptr< IConfigParser >;

class IConfigParser {
public:
    virtual ~IConfigParser() = default;
    virtual bool load(QString input) = 0;
    virtual bool preprocess() = 0;
    virtual Optional< Root > dump() = 0;
    virtual QVariant parse(const QString& input) const = 0;
    virtual const QString& getConfig() const = 0;
    virtual ParserPtr clone() const = 0;
};

class ITemplateEngine;
using RendererPtr = std::shared_ptr< ITemplateEngine >;

class ITemplateEngine {
public:
    virtual ~ITemplateEngine() = default;
    virtual bool initialize() = 0;
    virtual Optional< CString > renderWithGlobalVars(const CString& document, const GlobalVars& vars) const = 0;
    virtual Optional< CString > renderWithJsonVars(const CString& document, const CString& jsonData, const GlobalVars& vars) const = 0;
    virtual QVariant parse(const CString& document) const = 0;
    virtual RendererPtr clone() const = 0;
};

struct RenderingContext {
    RendererPtr Renderer;
    ParserPtr   Parser;
    GlobalVars  Variables;
};

class Config;
using ConfigPtr = std::shared_ptr< Config >;
using ConstConfigPtr = std::shared_ptr< const Config >;

class ConfigLoader;

class ConfigPrivate;

class Config {
public:
    Config();
    ~Config();

    Config& setParser(ParserPtr parser);
    Config& setRenderer(RendererPtr renderer);
    Config& setPath(const QString& path);
    Config& setOriginalText(QString text);

    const QString& originalText() const;
    const QString& preprocessedConfig() const;
    Optional< Root > root() const;
    QString path() const;
    ParserPtr parser() const;
    RendererPtr renderer() const;
    QList< ServiceRequestLookUpData > requestsData() const;
    QList< ServiceProxy > renderedProxies() const;
    Optional< HttpRequest > findHttpRequest(const ServiceRequestLookUpData& requestInfo) const;

    bool initialize();
    bool load(QString input);
    bool preprocess();
    bool resolve();
    bool renderGlobalVars();
    bool renderProxies();
    bool collectRequestsLookUpData();
    // bool validate()?

    std::shared_ptr< RenderingContext > createRenderingContext() const;
    ConfigPtr clone() const;

private:
    std::unique_ptr< ConfigPrivate > d_ptr;
    Q_DECLARE_PRIVATE(Config)
};


// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// API

Optional< CString >             RenderRaw(const CString& source, const RenderingContext& context);
Optional< CString >             RenderFromJson(const CString& source, const QString& jsonData, const RenderingContext& context);

QVariant                        Render(const CString& source, const RenderingContext& context);
Optional< GlobalVar >           Render(const GlobalVar& source, const RenderingContext& context);
Optional< ServiceProxy >        Render(const Proxy& source, const RenderingContext& context);
Optional< HttpServiceRequest >  Render(const HttpRequest& source, const RenderingContext& context);

ParserPtr                       CreateYamlParser();
RendererPtr                     CreateInjaRenderer();
ConfigPtr                       CreateEmptyConfig();
ConfigPtr                       CreateEmptyConfig(ParserPtr parser, RendererPtr renderer, Optional< QString > path = std::nullopt);
ConfigPtr                       CreateDefaultConfig(Optional< QString > path);

ConfigPtr                       LoadConfigFromData(const QString& data, ParserPtr parser, RendererPtr renderer);
QFuture< ConfigPtr >            LoadConfigFromDataAsync(const QString& data, ParserPtr parser, RendererPtr renderer);
ConfigPtr                       LoadConfigFromFile(const QString& path, ParserPtr parser, RendererPtr renderer);
QFuture< ConfigPtr >            LoadConfigFromFileAsync(const QString& path, ParserPtr parser, RendererPtr renderer);

bool                            SaveConfigToFile(const QString& path, ConstConfigPtr config);

// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

}
