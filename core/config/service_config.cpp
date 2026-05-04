#include "service_config_p.h"

#include "log/log.h"

#include "utils/files.h"

#include "yaml_parser.h"
#include "inja_renderer.h"

#include <QDebug>
#include <QFile>
#include <QtConcurrent>

using namespace ServiceConfig;


Config::Config()
    : d_ptr(new ConfigPrivate)
{ }


Config::~Config() = default;


Config& Config::setOriginalText(QString text)
{
    Q_D(Config);
    d->m_originalText = std::move(text);
    return *this;
}


Config& Config::setParser(ParserPtr parser) {
    Q_D(Config);
    d->m_parser = std::move(parser);
    return *this;
}


Config& Config::setRenderer(RendererPtr renderer) {
    Q_D(Config);
    d->m_renderer = std::move(renderer);
    return *this;
}


Config& Config::setPath(const QString& path)
{
    Q_D(Config);
    d->m_path = path;
    return *this;
}


const QString& Config::originalText() const
{
    Q_D(const Config);
    return d->m_originalText;
}


const QString& Config::preprocessedConfig() const {
    Q_D(const Config);
    return d->m_parser->getConfig();
}


Optional< Root > Config::root() const
{
    Q_D(const Config);
    return d->m_root;
}


QString Config::path() const
{
    Q_D(const Config);
    return d->m_path;
}


ParserPtr Config::parser() const {
    Q_D(const Config);
    return d->m_parser;
}


RendererPtr Config::renderer() const {
    Q_D(const Config);
    return d->m_renderer;
}


QList< ServiceRequestLookUpData > Config::requestsData() const
{
    Q_D(const Config);
    return d->m_requestLookUpData;
}


QList< ServiceProxy > Config::renderedProxies() const
{
    Q_D(const Config);
    return d->m_renderedProxies;
}


Optional< HttpRequest > Config::findHttpRequest(const ServiceRequestLookUpData& requestInfo) const
{
    auto root = this->root();
    if (!root) {
        qCritical() << "Cannot find request as there's no root (config is not properly parsed)";
        return std::nullopt;
    }

    auto serviceProviderIt = root->ServiceProviders.find(requestInfo.ServiceProviderName);
    if (serviceProviderIt == root->ServiceProviders.cend()) {
        qCritical() << "Failed to find service provider " << requestInfo.ServiceProviderName;
        return std::nullopt;
    }

    auto serviceIt = serviceProviderIt.value().Services.find(requestInfo.ServiceName);
    if (serviceIt == serviceProviderIt.value().Services.cend()) {
        qCritical() << "Failed to find service " << requestInfo.ServiceName;
        return std::nullopt;
    }

    auto requestIt = serviceIt.value().Requests.find(requestInfo.RequestName);
    if (requestIt == serviceIt.value().Requests.cend()) {
        qCritical() << "Failed to find request " << requestInfo.RequestName;
        return std::nullopt;
    }

    return requestIt.value();
}


bool Config::initialize()
{
    Q_D(Config);

    if (!d->m_parser) {
        qDebug() << "Have no parser in config";
        return false;
    }

    if (!d->m_renderer) {
        qDebug() << "Have no renderer in config";
        return false;
    }

    if (!d->m_renderer->initialize()) {
        qDebug() << "Renderer failed to initialize";
        return false;
    }

    return true;
}


bool Config::load(QString input) { // parse as yaml
    Q_D(Config);

    if (!d->m_parser) {
        qDebug() << "Have no parser in config";
        return false;
    }

    if (!d->m_renderer) {
        qDebug() << "Have no renderer in config";
        return false;
    }

    if (!d->m_parser->load(input)) {
        qDebug() << "Parser failed to load";
        return false;
    }

    d->m_originalText = std::move(input);

    return true;
}


bool Config::preprocess() { // expand
    Q_D(Config);

    if (!d->m_parser) {
        qDebug() << "Have no parser in config";
        return false;
    }

    if (!d->m_renderer) {
        qDebug() << "Have no renderer in config";
        return false;
    }

    if (!d->m_parser->preprocess()) {
        qDebug() << "Parser failed to preprocess";
        return false;
    }

    return true;
}


bool Config::resolve() { // resolve structure
    Q_D(Config);

    if (!d->m_parser) {
        qDebug() << "Have no parser in config";
        return false;
    }

    if (!d->m_renderer) {
        qDebug() << "Have no renderer in config";
        return false;
    }

    if (auto root = d->m_parser->dump()) {
        d->m_root = std::move(root);
    } else {
        qDebug() << "Parser failed to extract raw data";
        return false;
    }

    return true;
}


bool Config::renderGlobalVars() { // render global vars
    Q_D(Config);

    qDebug() << "Started rendering global variables";
    if (!d->m_root) {
        qDebug() << "Cannot render global variables before resolving config";
        return false;
    }

    auto renderingContext = createRenderingContext();
    if (!renderingContext ) {
        qDebug() << "Failed to create rendering context";
        return false;
    }

    for (const auto& [name, var] : d->m_root->GlobalVariables) {
        auto maybeRendered = Render(var, *renderingContext);
        if (!maybeRendered) {
            qDebug() << "Failed to render " << var.Name << " variables with value " << var.Value;
            return false;
        } else {
            qDebug() << "Rendered " << var.Name << " variable with value " << var.Value;
        }

        renderingContext->Variables[maybeRendered->Name] = *maybeRendered;
    }

    d->m_root->GlobalVariables = std::move(renderingContext->Variables);

    return true;
}


bool Config::renderProxies()
{
    Q_D(Config);

    if (!d->m_root) {
        qDebug() << "Cannot render proxies before resolving config";
        return false;
    }

    auto renderingContext = createRenderingContext();
    if (!renderingContext ) {
        qDebug() << "Failed to create rendering context";
        return false;
    }

    QList< ServiceProxy > proxies;
    for (const auto& [name, proxy] : d->m_root->Proxies) {
        auto renderedProxy = ServiceConfig::Render(proxy, *renderingContext);
        if (!renderedProxy) {
            qDebug() << "Failed to render proxy " << name;
            return false;
        }

        proxies.append(*renderedProxy);
    }

    d->m_renderedProxies = proxies;
    return true;
}


bool Config::collectRequestsLookUpData()
{
    Q_D(Config);

    if (!d->m_root) {
        qDebug() << "Cannot collect requests look up data before resolving config";
        return false;
    }

    for (const auto& [spName, serviceProvider] : d->m_root->ServiceProviders) {
        for (const auto& [svcName, service] : serviceProvider.Services) {
            for (const auto& [reqName, request] : service.Requests) {
                d->m_requestLookUpData.push_back({
                    spName,
                    svcName,
                    reqName
                });
            }
        }
    }

    return true;
}


std::shared_ptr< RenderingContext > Config::createRenderingContext() const
{
    const Q_D(Config);

    if (!d->m_root || !d->m_parser || !d->m_renderer) {
        return nullptr;
    }

    return std::make_shared< RenderingContext >(d->m_renderer, d->m_parser, d->m_root->GlobalVariables);
}


ConfigPtr Config::clone() const
{
    Q_D(const Config);

    auto newConfig = std::make_shared< Config >();

    newConfig->d_ptr->m_path = d->m_path;
    newConfig->d_ptr->m_originalText = d->m_originalText;
    newConfig->d_ptr->m_root = d->m_root;
    newConfig->d_ptr->m_renderedProxies = d->m_renderedProxies;
    newConfig->d_ptr->m_requestLookUpData = d->m_requestLookUpData;

    if (d->m_parser) {
        newConfig->d_ptr->m_parser = d->m_parser->clone();
    }

    if (d->m_renderer) {
        newConfig->d_ptr->m_renderer = d->m_renderer->clone();
    }

    return newConfig;
}


Optional< CString > ServiceConfig::RenderRaw(const CString& source, const RenderingContext& context)
{
    return context.Renderer->renderWithGlobalVars(source, context.Variables);
}


Optional< CString > ServiceConfig::RenderFromJson(const CString& source, const QString& jsonData, const RenderingContext& context)
{
    return context.Renderer->renderWithJsonVars(source, jsonData, context.Variables);
}


QVariant ServiceConfig::Render(const CString& source, const RenderingContext& context)
{
    auto rendered = context.Renderer->renderWithGlobalVars(source, context.Variables);

    if (!rendered) {
        qCritical() << "Failed to render: " << source;
        return {};
    }

    auto parsed = context.Renderer->parse(*rendered);

    return parsed;
}


Optional< GlobalVar > ServiceConfig::Render(const GlobalVar& source, const RenderingContext& context)
{
    // only strings may be rendered
    if (source.Value.canConvert< QString >()) {
        auto rendered = Render(source.Value.toString(), context);
        if (!rendered.isNull()) return GlobalVar::fromValue(source.Name, rendered);
        else return std::nullopt;
    }

    return source;
}


static Optional< QNetworkProxy > ParseProxyEndpoint(const QString& endpoint)    // TODO@ REMOVE
{
    QStringList parts = endpoint.split(':', Qt::SkipEmptyParts);
    if (parts.size() != 2) {
        qWarning() << "Invalid proxy endpoint format (expected 'host:port'):" << endpoint;
        return {};
    }

    bool ok;
    quint16 port = parts[1].toUShort(&ok);
    if (!ok || port == 0) {
        qWarning() << "Invalid port in proxy endpoint:" << endpoint;
        return {};
    }

    QHostAddress address(parts[0]);
    if (address.isNull()) {
        qWarning() << "Invalid host address in proxy endpoint:" << endpoint;
        return {};
    }

    return QNetworkProxy(QNetworkProxy::HttpProxy, address.toString(), port);
}


Optional< ServiceProxy > ServiceConfig::Render(const Proxy& source, const RenderingContext& context)
{
    ServiceProxy result;

    result.Name = source.Name;
    if (auto renderedEndpoint = RenderRaw(source.Endpoint, context)) {
        if (auto parsedNetworkProxy = ParseProxyEndpoint(*renderedEndpoint)) {
            result.NetworkProxy = *parsedNetworkProxy;
        }
    } else {
        return std::nullopt;
    }

    return result;
}


Optional< HttpServiceRequest > ServiceConfig::Render(const HttpRequest& source, const RenderingContext& context)
{
    /*
        1. Url
        2. Method
        3. Headers
        4. Body
        5. Ignore Ssl Erros
        6. Use Proxies
    */

    HttpServiceRequest result;

    result.Name = source.Name;
    result.Display.Reply.Selector = source.Display.Reply.Selector;

    if (auto renderedUrl = RenderRaw(source.Url, context)) {
        result.Url = QUrl(*renderedUrl);
    }
    else {
        return std::nullopt;
    }

    if (auto renderedMethod = RenderRaw(source.Method, context)) {
        result.Method = *renderedMethod;
    } else {
        return std::nullopt;
    }

    if (auto renderedHeaders = RenderRaw(source.Headers, context)) {
        qTrace() << "Gonna parse rendered headers: " << *renderedHeaders;
        if (auto parsedHeaders = QVariantToOptional< QHash< QString, QString > >(context.Parser->parse(*renderedHeaders))) {
            result.Headers = *parsedHeaders;
        } else {
            return std::nullopt;
        }
    } else {
        return std::nullopt;
    }

    if (auto renderedBody = RenderRaw(source.Body, context)) {
        result.Body = renderedBody->toUtf8();
    } else {
        return std::nullopt;
    }

    if (auto renderedIgnoreSslErros = RenderRaw(source.IgnoreSslErrors, context)) {
        if (auto parsedIgnoreSslErrors = QVariantToOptional< bool >(context.Parser->parse(*renderedIgnoreSslErros))) {
            result.IgnoreSslErrors = *parsedIgnoreSslErrors;
        } else {
            return std::nullopt;
        }
    } else {
        return std::nullopt;
    }

    if (auto renderedUseProxies = RenderRaw(source.UseProxies, context)) {
        if (auto parsedUseProxies = QVariantToOptional< bool >(context.Parser->parse(*renderedUseProxies))) {
            result.UseProxies = *parsedUseProxies;
        } else {
            return std::nullopt;
        }
    } else {
        return std::nullopt;
    }

    if (auto renderedDisplayRequest = RenderRaw(source.Display.Request.Selector, context)) {
        result.Display.Request.Selector = *renderedDisplayRequest;
    } else {
        return std::nullopt;
    }

    return result;
}


ParserPtr ServiceConfig::CreateYamlParser()
{
    return std::make_shared< YamlParser >();
}


RendererPtr ServiceConfig::CreateInjaRenderer()
{
    return std::make_shared< InjaRenderer >();
}


static ConfigPtr LoadConfigFromDataImpl(const QString& content,
                                        ParserPtr parser,
                                        RendererPtr renderer,
                                        Optional< QString > path = std::nullopt,
                                        QPromise< ConfigPtr >* promise = nullptr)
{
    int step = 0;
    auto updateProgress = [&](const QString& message) {
        qDebug() << "Config progress updated: " << message << ", step - " << ++step;
        if (promise) {
            promise->setProgressValueAndText(step, message);
        }
    };

    auto checkCancellation = [&]() -> bool {
        return promise && promise->isCanceled();
    };

    if (promise) {
        promise->setProgressRange(0, 8);
        updateProgress("Started");
    }

    // Step 1: Create config object
    auto config = CreateEmptyConfig(std::move(parser), std::move(renderer), path);
    if (!config) {
        updateProgress("Failed to create config");
        return nullptr;
    }

    if (checkCancellation()) {
        updateProgress("Cancelled");
        return nullptr;
    }

    // Step 2: Initialize config
    updateProgress("Initializing config...");
    if (!config->initialize()) {
        updateProgress("Failed to initialize config");
        return nullptr;
    }

    if (checkCancellation()) {
        updateProgress("Cancelled");
        return nullptr;
    }


    // Step 3: Parse config
    updateProgress("Parsing config...");
    if (!config->load(content)) {
        updateProgress("Failed to parse config");
        return nullptr;
    }

    if (checkCancellation()) {
        updateProgress("Cancelled");
        return nullptr;
    }

    // Step 4: Preprocess
    updateProgress("Preprocessing...");
    if (!config->preprocess()) {
        updateProgress("Failed to preprocess config");
        return nullptr;
    }

    // QThread::sleep(std::chrono::seconds{ 5 });

    if (checkCancellation()) {
        updateProgress("Cancelled");
        return nullptr;
    }

    // Step 5: Resolve
    updateProgress("Resolving...");
    if (!config->resolve()) {
        updateProgress("Failed to resolve config");
        return nullptr;
    }

    if (checkCancellation()) {
        updateProgress("Cancelled");
        return nullptr;
    }

    // Step 6: Render global variables
    updateProgress("Rendering global variables...");
    if (!config->renderGlobalVars()) {
        updateProgress("Failed to render global variables");
        return nullptr;
    }

    if (checkCancellation()) {
        updateProgress("Cancelled");
        return nullptr;
    }

    // Step 7: Render proxies
    updateProgress("Rendering proxies...");
    if (!config->renderProxies()) {
        updateProgress("Failed to render proxies");
        return nullptr;
    }

    if (checkCancellation()) {
        updateProgress("Cancelled");
        return nullptr;
    }

    // Step 8: Collect request metadata
    updateProgress("Collecting request info...");
    if (!config->collectRequestsLookUpData()) {
        updateProgress("Failed to collect request metadata");
        return nullptr;
    }

    updateProgress("Done");

    return config;
}


ConfigPtr ServiceConfig::CreateEmptyConfig()
{
    return std::make_shared< Config >();
}


ConfigPtr ServiceConfig::CreateEmptyConfig(ParserPtr parser, RendererPtr renderer, Optional< QString > path)
{
    auto result = CreateEmptyConfig();

    result->setParser(std::move(parser));
    result->setRenderer(std::move(renderer));

    if (path)
        result->setPath(*path);

    return result;
}


ConfigPtr ServiceConfig::CreateDefaultConfig(Optional< QString > path)
{
    static const ConfigPtr defaultConfig = ([] () {
        auto configContent = utils::ReadFile(":/others/service-config-default.txt");
        Q_ASSERT(configContent);
        return LoadConfigFromDataImpl(*configContent, CreateYamlParser(), CreateInjaRenderer());
    })();

    auto result = defaultConfig->clone();

    if (path)
        result->setPath(*path);

    return result;
}


ConfigPtr ServiceConfig::LoadConfigFromData(const QString& data, ParserPtr parser, RendererPtr renderer)
{
    return LoadConfigFromDataImpl(data, std::move(parser), std::move(renderer));
}


QFuture< ConfigPtr > ServiceConfig::LoadConfigFromDataAsync(const QString& data, ParserPtr parser, RendererPtr renderer)
{
    return QtConcurrent::run([=](QPromise< ConfigPtr >& promise) {
        auto config = LoadConfigFromDataImpl(data, std::move(parser), std::move(renderer), std::nullopt, &promise);
        promise.addResult(std::move(config));
    });
}


ConfigPtr ServiceConfig::LoadConfigFromFile(const QString& path, ParserPtr parser, RendererPtr renderer)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open config file:" << path;
        return nullptr;
    }

    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    return LoadConfigFromDataImpl(content, std::move(parser), std::move(renderer), path);
}


QFuture< ConfigPtr > ServiceConfig::LoadConfigFromFileAsync(const QString& path, ParserPtr parser, RendererPtr renderer)
{
    return QtConcurrent::run([=](QPromise< ConfigPtr >& promise) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "Failed to open config file:" << path;
            promise.setProgressValueAndText(0, "File open failed");
            promise.addResult(nullptr);
            return;
        }

        QTextStream in(&file);

        QString content = in.readAll();
        file.close();

        auto result = LoadConfigFromDataImpl(content, parser, renderer, path, &promise);

        promise.addResult(std::move(result));
    });
}


bool ServiceConfig::SaveConfigToFile(const QString& path, ConstConfigPtr config)
{
    if (!config) {
        qError() << "Service config is null, cannot save to " << path;
        return false;
    }

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

    // @todo WRONG, should save based on root value
    out << config->originalText();

    return true;
}
