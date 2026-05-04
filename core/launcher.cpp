#include "launcher.h"
#include "launcher_p.h"

#include "common.h"

#include "utils/platform.h"

#include "log/formatters/fmt.h"
#include "log/components/colorful_console.h"
#include "log/components/rotating_file.h"

#include "config/app_config.h"
#include "config/service_config.h"

#include "highlighters/markdown_highlighter.h"

#include "hotkey_manager.h"
#include "config_manager.h"
#include "network/api_controller.h"
#include "screenshot_helper.h"

#include "widgets/markdown_viewer.h"

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QStandardPaths>


Launcher::Launcher(QObject *parent)
    : QObject{ parent }
    , d_ptr(new LauncherPrivate(this))
{
    Q_D(Launcher);

    d->m_apiController.reset(new HttpApiController);
    d->m_apiController->initialize();

    initializeAppConfiguration();
    initializeLoggingSystem();

    d->m_overlay = std::make_unique< MarkdownViewer >(nullptr);
    d->m_overlay->setMoveStep({ 25, 25 });

    initializeFinalConfiguration();
    initializeHotkeys();
    createConnections();

    d->m_overlay->show();

    d->m_overlay->setExcludeFromCapture(ConfigManager::instance()->appConfig()->excludeFromCapture);
}


Launcher::~Launcher()
{
    Q_D(Launcher);

    while (d->undoLastAction()); // stop all the operations

    qDebug() << "Before Hotkey manager shutdown";

    HotkeyManager::instance()->clearAll();

    qDebug() << "Before log manager shutdown";

    LogManager::instance()->shutdown();
}


void Launcher::initializeAppConfiguration()
{
    Q_D(Launcher);

    d->m_appConfigPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/config/settings.toml";
    d->m_serviceConfigPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/config/services.yaml";

    auto* configManager = ConfigManager::instance();

    connect(configManager, &ConfigManager::appConfigLoaded,     d, qOverload< AppConfig::ConstConfigPtr >(&LauncherPrivate::handleLoadedConfig));
    connect(configManager, &ConfigManager::serviceConfigLoaded, d, qOverload< ServiceConfig::ConstConfigPtr >(&LauncherPrivate::handleLoadedConfig));

    if (!configManager->loadAppConfigFromFile(d->m_appConfigPath, AppConfig::LoadMode_PartialOverload)) {
        LOG_ERROR() << "Failed to load app config at " << d->m_appConfigPath;
    }

    qInfo() << "Initialized app configuration at " << d->m_appConfigPath << ".";
}


void Launcher::initializeLoggingSystem()
{
    Q_D(Launcher);

    auto appConfig = ConfigManager::instance()->appConfig();

    LogInitParams initParams
    {
        .asyncMode = appConfig->asyncMode,
        .maxQueueSize = appConfig->maxQueueSize,
        .minLevel = appConfig->minLevel,
        .redirectQtLogging = true
    };

    auto logFormatter = FmtLogFormatter::create();

    if (utils::IsStdoutValid()) {
        logFormatter->getFormat().pattern = "[{timestamp}] [{level}] [{function}] [{line}]: {message}";
        d->m_consoleLogComponent = ColorfulConsoleLogComponent::create("ColorfulConsoleLog", logFormatter);

        if (!LogManager::instance()->addComponent(d->m_consoleLogComponent.get())) {
            LOG_WARNING() << "Failed to add console log component";
        } else {
            LOG_DEBUG() << "Initialized console log component";
            logFormatter = FmtLogFormatter::create();
        }
    }
    else {
        LOG_DEBUG() << "No console attached";
    }

    auto logsLocation = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/logs/app.log";
    logFormatter->getFormat().pattern = "[{timestamp}] [{level}] [{file}] [{function}] [{line}] {message}";

    d->m_rotatingFileLogComponent = RotatingFileLogComponent::create("FileLog", logsLocation, logFormatter);

    if (!LogManager::instance()->addComponent(d->m_rotatingFileLogComponent.get())) {
        LOG_ERROR() << "Failed to add rotating file log component";
    } else {
        LOG_DEBUG() << "Initialized file log component, path: " << logsLocation;
    }

    if (!LogManager::instance()->initialize(initParams)) {
        LOG_FATAL() << "Failed to initialize logging system";
    }

    LOG_DEBUG() << "Initialized logging system";
}


void Launcher::initializeFinalConfiguration()
{
    Q_D(Launcher);

    if (!QFile::exists(d->m_serviceConfigPath)) {

        qWarning() << "Service config at path " << d->m_serviceConfigPath << " does not exist";
        d->m_saveServiceConfig = true;
        ConfigManager::instance()->setServiceConfig(ServiceConfig::CreateDefaultConfig(d->m_serviceConfigPath));
    }
    else if (!ConfigManager::instance()->loadServiceConfigFromFileAsync(d->m_serviceConfigPath, ServiceConfig::CreateYamlParser(), ServiceConfig::CreateInjaRenderer())) {
        qError() << "Failed to load service config at " << d->m_serviceConfigPath;
    }
}


void Launcher::initializeHotkeys()
{
    HotkeyManager::instance()->override(ConfigManager::instance()->appConfig()->hotkeys);
    qDebug() << "Initialized hotkeys";
}


void Launcher::createConnections()
{
    Q_D(Launcher);

    connect(d->m_apiController.get(),   &HttpApiController::replyReady,             d,  &LauncherPrivate::handleHttpServiceReply);
    connect(d->m_apiController.get(),   &HttpApiController::errorOccurred,          d,  &LauncherPrivate::handleHttpServiceError);
    connect(d->m_apiController.get(),   &HttpApiController::proxySucceeded,         d,  &LauncherPrivate::handleProxySucceeded);
    connect(d->m_apiController.get(),   &HttpApiController::proxyFailed,            d,  &LauncherPrivate::handleProxyFailed);

    connect(d->m_overlay.get(),         &MarkdownViewer::filesChosen,               d,  &LauncherPrivate::handleFilesChosen);
    connect(d->m_overlay.get(),         &MarkdownViewer::messageSent,               d,  &LauncherPrivate::handleMessageSent);
    connect(d->m_overlay.get(),         &MarkdownViewer::messageRemoved,            d,  &LauncherPrivate::handleMessageRemoved);
    connect(d->m_overlay.get(),         &MarkdownViewer::screenshotPreviewClicked,  d,  &LauncherPrivate::handleScreenshotPreviewClicked);
    connect(d->m_overlay.get(),         &MarkdownViewer::screenshotReady,           d,  &LauncherPrivate::handleScreenshotReady);
    connect(d->m_overlay.get(),         &MarkdownViewer::fileMenuItemClicked,       d,  &LauncherPrivate::handleFileMenuItemClicked);
    connect(d->m_overlay.get(),         &MarkdownViewer::settingsOkClicked,         d,  &LauncherPrivate::handleSettingsOk);
    connect(d->m_overlay.get(),         &MarkdownViewer::settingsApplyClicked,      d,  &LauncherPrivate::handleSettingsApply);
    connect(d->m_overlay.get(),         &MarkdownViewer::settingsSaveClicked,       d,  &LauncherPrivate::handleSettingsSave);
    connect(d->m_overlay.get(),         &MarkdownViewer::settingsCancelClicked,     d,  &LauncherPrivate::handleSettingsCancel);

    connect(HotkeyManager::instance(),  &HotkeyManager::actionTriggered,            d,  &LauncherPrivate::handleAction);

    qDebug() << "Created connections";
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
//////// LauncherPrivate

void LauncherPrivate::handleAction(ActionType action)
{
    if (m_overlay->handleAction(action)) {
        return;
    }

    switch (action)
    {
        case ActionType::Quit:
            qApp->quit();
            break;

        case ActionType::TakeScreenshot:
            takeScreenshot();
            break;

        case ActionType::UndoLastAction:
            undoLastAction();
            break;

        case ActionType::SendCurrentRequest:
            sendCurrentRequest(SingleRequest_Mode);
            break;

        case ActionType::ReloadConfig:
            reloadConfig();
            break;

        case ActionType::Test1:
            qDebug() << "Test1 pressed";
            break;

        default:
            qDebug() << QString("No handler for %1 action").arg((int)action);
            break;
    }
}


void LauncherPrivate::handleHttpServiceReply(const HttpServiceReply& httpReply)
{
    int requestId = httpReply.Id;

    auto config = ConfigManager::instance()->serviceConfig();
    if (!config) { // must not happen at all
        qFatal() << "Cannot handle service reply with an empty config";
    }

    const auto& httpRequest = m_history[requestId].HttpRequest;

    auto ctx = config->createRenderingContext();
    ctx->Variables["ServiceRequest"] = ServiceConfig::GlobalVar::fromValue("ServiceRequest", ::ToQVariant(httpRequest));
    ctx->Variables["ServiceReply"]   = ServiceConfig::GlobalVar::fromValue("ServiceReply", ::ToQVariant(httpReply));
    ctx->Variables["Files"]          = ServiceConfig::GlobalVar::fromValue("Files", ::ToQVariant(m_history[requestId].Files));
    ctx->Variables["Messages"]       = ServiceConfig::GlobalVar::fromValue("Messages", collectEntries());

    if (m_history[requestId].CustomPrompt)
        ctx->Variables["CustomPrompt"] = ServiceConfig::GlobalVar::fromValue("CustomPrompt", *m_history[requestId].CustomPrompt);

    // Render to ServiceReply
    auto serviceReply = render(httpReply, *ctx);
    if (!serviceReply) {
        qError() << "Failed to create service reply";
        m_history[requestId].IsFailed = true;
        return;
    }

    m_history[requestId].HttpReply = httpReply;
    m_history[requestId].ServiceReply = *serviceReply;

    ChatMessage messageReply = createReplyMessage(*serviceReply);

    m_history[requestId].ReplyMessageId = messageReply.Id;

    m_overlay->setMarkdownText(serviceReply->MarkdownText);
    m_overlay->setLastOperationStatus(OperationStatus::Succeeded);
}


void LauncherPrivate::handleHttpServiceError(const HttpServiceError& serviceError)
{
    qDebug() << QString("Service error (%1) - %2").arg(serviceError.Code)
                                                  .arg(serviceError.ErrorDescription);

    m_history[serviceError.Id].IsFailed = true;

    if (auto userMessage = m_overlay->findChatMessage(m_history[serviceError.Id].RequestMessageId))
    {
        qTrace() << "Changing user message message color. Request message id: " << userMessage->get().Id;
        userMessage->get().MessageColor = QColor(Qt::darkRed);
        m_overlay->update(); // trigger repaint
    } else
    {
        qWarning() << "Failed to find user message with id " << m_history[serviceError.Id].RequestMessageId;
    }

    // not clearing overlay as it may be important to show the last obtained information
    m_overlay->setLastOperationStatus(OperationStatus::Failed);
}


void LauncherPrivate::handleProxySucceeded(const QString& proxyName)
{
    LOG_DEBUG("Proxy {} succeeded", proxyName);
    m_overlay->incProxySuccessCounter(proxyName, 1);
}


void LauncherPrivate::handleProxyFailed(const QString& proxyName)
{
    LOG_DEBUG("Proxy {} failed", proxyName);
    m_overlay->incProxyFailedCounter(proxyName, 1);
}


void LauncherPrivate::handleLoadedConfig(AppConfig::ConstConfigPtr config)
{
    Q_ASSERT(config);

    // general

    // at the beginning overlay may not be initialized yet
    if (m_overlay) {
        m_overlay->resetSettings(SettingsView::create(config));
        m_overlay->setExcludeFromCapture(config->excludeFromCapture);

#ifdef Q_OS_WIN
        utils::ScopedCallWndProcHook::setNoCapture(config->excludeFromCapture);
#endif
    }

    // logging
    LogManager::instance()->setAsyncMode(config->asyncMode);
    LogManager::instance()->setMaxQueueSize(config->maxQueueSize);
    LogManager::instance()->setMinLogLevel(config->minLevel);

    // proxies
    m_apiController->setProxies(config->proxies | views::filter([](const auto& proxy) { return proxy.Enabled; }) | ranges::to< ::ProxyList >());

    // hotkeys
    HotkeyManager::instance()->override(config->hotkeys);

    if (!QFileInfo::exists(config->path)) {
        m_saveAppConfig = true;
    }

    if (m_saveAppConfig) {
        ConfigManager::instance()->saveAppConfigToFile(m_appConfigPath);
        m_saveAppConfig = false;
    }

    qDebug() << "App config loaded succesfully";

    if (m_overlay)
        m_overlay->setLastOperationStatus(OperationStatus::Succeeded);
}


void LauncherPrivate::handleLoadedConfig(ServiceConfig::ConstConfigPtr config)
{    
    Q_ASSERT(config);

    m_overlay->setRequestsData(config->requestsData());
    m_overlay->resetSettings(SettingsView::create(nullptr, config));

    if (m_saveServiceConfig) {
        ConfigManager::instance()->saveServiceConfigToFile(m_serviceConfigPath);
        m_saveServiceConfig = false;
    }

    qDebug() << "Service config loaded succesfully";
    m_overlay->setLastOperationStatus(OperationStatus::Succeeded);
}


void LauncherPrivate::handleFilesChosen(const QStringList& filePaths)
{
    MimeDataList processedFiles;
    processedFiles.reserve(filePaths.size());

    for (const QString& path : filePaths)
    {
        if (auto mimeData = MimeData::fromFile(path))
        {
            qInfo() << "Loaded mime data from file at " << path;
            addFileMimeData(std::move(*mimeData));
        } else
        {
            qError() << "Failed to load mime data from file at " << path;
        }
    }
}


void LauncherPrivate::handleMessageSent(const QString& prompt)
{
    qTrace() << "Message prompt in handler: " << prompt;
    sendCurrentRequest(Chat_Mode, prompt);
}


void LauncherPrivate::handleMessageRemoved(const ChatMessage& message)
{
    for (auto& entry : m_history)
    {
        if (entry.RequestMessageId == message.Id) {
            entry.RequestMessageId = -1;
            qDebug() << QString("Changing entry %1 request message id %2 to -1").arg(entry.Id).arg(message.Id);
            return;
        }

        if (entry.ReplyMessageId == message.Id) {
            entry.ReplyMessageId = -1;
            qDebug() << QString("Changing entry %1 reply message id %2 to -1").arg(entry.Id).arg(message.Id);
            return;
        }
    }

    qWarning() << "Failed to find entry with request or reply message ids - " << message.Id;
}


void LauncherPrivate::handleScreenshotPreviewClicked(int index)
{
    if (!deleteMimeDataAt(index))
    {
        qFatal() << "Failed to delete mime data at index " << index
                 << ". Something's seriously wrong. Current mime data list size " << m_currentFiles.size()
                 << ". Screenshot preview size: " << m_overlay->screenshotPreviewsSize();
    }
}


void LauncherPrivate::handleScreenshotReady(const QPixmap& screenshot)
{
    takeScreenshot(screenshot);
}


void LauncherPrivate::handleFileMenuItemClicked(const QString& fileName)
{
    qInfo() << "File menu item clicked: " << fileName;

    auto it = std::find_if(m_currentFiles.begin(), m_currentFiles.end(), [&fileName](const auto& data) { return fileName == data.SourceName; });
    auto index = std::distance(m_currentFiles.begin(), it);

    if (!deleteMimeDataAt(index))
    {
        qFatal() << "Failed to delete mime data at index " << index
                 << ". Something's seriously wrong. Current mime data list size " << m_currentFiles.size()
                 << ". Screenshot preview size: " << m_overlay->screenshotPreviewsSize()
                 << ". File menu list size: " << m_overlay->fileMenuListSize();
    }
}


void LauncherPrivate::handleSettingsOk(const SettingsView& data)
{
    handleSettingsApply(data);
}


void LauncherPrivate::handleSettingsApply(const SettingsView& data)
{
    auto* manager = ConfigManager::instance();

    // apply app config
    manager->setAppConfig(data.AppSettings.config);

    qDebug() << "Before checking new service config";
    LOG_DEBUG("Is loaded config nullptr: {}, is manager->serviceConfig() nullptr: {}",
              data.ServicesSettings.loadedConfig == nullptr, manager->serviceConfig() == nullptr);
    if (data.ServicesSettings.loadedConfig &&
        data.ServicesSettings.loadedConfig->originalText() != manager->serviceConfig()->originalText())
    {
        // config was loaded and it's different
        qDebug() << "Set service config from loaded config";
        manager->setServiceConfig(data.ServicesSettings.loadedConfig);
    }
    else if (data.ServicesSettings.rawConfig &&
             manager->serviceConfig()->originalText() != *data.ServicesSettings.rawConfig)
    {
        qDebug() << "Set service config from raw config";
        // different raw config
        if (!manager->loadServiceConfigFromDataAsync(*data.ServicesSettings.rawConfig,
                                                     ServiceConfig::CreateYamlParser(),
                                                     ServiceConfig::CreateInjaRenderer()))
        {
            qError() << "Failed to start service config loading from data";
        }
    }
}


void LauncherPrivate::handleSettingsSave(const SettingsView& data)
{
    qDebug() << "Settings saved";

    m_saveAppConfig = true;
    m_saveServiceConfig = true;

    handleSettingsApply(data);
}


void LauncherPrivate::handleSettingsCancel()
{
    qDebug() << "Settings cancelled";
    m_overlay->resetSettings();
}


void LauncherPrivate::reloadConfig()
{
    if (m_apiController->currentRequestStatus() == HttpApiController::Status_Loading) {
        qError() << "Cannot reload config during network request processing... Cancel the request or wait for finishing";
        m_overlay->setLastOperationStatus(OperationStatus::Failed);
        return;
    }

    auto* configManager = ConfigManager::instance();

    if (!configManager->loadAppConfigFromFileAsync(m_appConfigPath, configManager->appConfig()->mode)) {
        qError() << "Failed to reaload app config, maybe already loading.";
    }

    auto yamlParser = ServiceConfig::CreateYamlParser();
    auto injaRenderer = ServiceConfig::CreateInjaRenderer();

    if (!ConfigManager::instance()->loadServiceConfigFromFileAsync(m_serviceConfigPath, std::move(yamlParser), std::move(injaRenderer))) {
        qError() << "Config already loading... Wait for finishing or cancel it.";
    }
}


void LauncherPrivate::takeScreenshot()
{
    QPixmap screenshot = ScreenshotHelper::grabScreenshot(m_overlay.get());
    takeScreenshot(screenshot);
}


void LauncherPrivate::takeScreenshot(const QPixmap& screenshot)
{
    constexpr QLatin1StringView screenshotFormat { "png" };

    if (!screenshot) {
        qDebug() << "Failed to take screenshot";
        m_overlay->setLastOperationStatus(OperationStatus::Failed);
        return;
    }

    QString path = ScreenshotHelper::getDefaultScreenshotPath(screenshotFormat);
    qDebug() << "Screenshot path: " << path;
    if (ConfigManager::instance()->appConfig()->saveScreenshots &&
        !ScreenshotHelper::saveScreenshot(screenshot, path, screenshotFormat)) {
        qError() << "Failed to save a screenshot at " << path;
    }

    auto mimeData = MimeData::fromImage(screenshot.toImage(), path);
    if (!mimeData) {
        qError() << "Failed to populate mime data from screenshot saved at " << path;
        return;
    }

    addFileMimeData(std::move(*mimeData));
    m_overlay->setLastOperationStatus(OperationStatus::Succeeded);
}


bool LauncherPrivate::undoLastAction()
{
    Optional< OperationStatus > status = OperationStatus::Canceled;;

    if (!cancelConfigLoading()) {
        if (!cancelLastRequest()) {
            if (!deleteMimeDataAt()) {
                if (!clearText()) {
                    qInfo() << "Nothing to do";
                    status = std::nullopt;
                } else {
                    qInfo() << "Cleared text";
                }
            } else {
                qInfo() << "Deleted last screenshot";
            }
        } else {
            qInfo() << "Cancelled current request";
        }
    } else {
        qInfo() << "Cancelled config loading";
    }

    if (status)
        m_overlay->setLastOperationStatus(*status);

    return status == OperationStatus::Canceled;
}


void LauncherPrivate::sendCurrentRequest(ESendMode mode, Optional< QString > prompt)
{
    qTrace() << QString("Send current request in %1 mode, with prompt param - %2").arg(static_cast< int >(mode))
                                                                                  .arg(prompt ? *prompt : "[no prompt]");

    // render http request with external data
    Optional< HttpServiceRequest > request = createHttpRequest(m_currentFiles, prompt);
    if (!request) {
        qError() << "Failed to create http request. Change configuration";
        return;
    }

    if (mode == SingleRequest_Mode) {
        m_overlay->clearChatMessages();
        m_history.clear();
    }

    ChatMessage userMessage = createUserMessage(request->Display.Request.Selector);
    qTrace() << "Create user message with id " << userMessage.Id;

    request->Id = ++m_lastRequestId;
    m_apiController->sendHttpRequest(*request);

    qDebug() << QString("Sending request %1 (id - %2)").arg(request->Name)
                                                       .arg(m_lastRequestId);
    qDebug() << "Headers: " << request->Headers;
    // qDebug() << "Request body: " << QString::fromUtf8(request->Body);

    HistoryEntry entry;
    entry.Id                = m_lastRequestId;
    entry.RequestMessageId  = userMessage.Id;
    entry.IsFailed          = false;
    entry.HttpRequest       = *request;
    entry.CustomPrompt      = prompt;
    entry.Files             = m_currentFiles;

    qTrace().noquote() << QString(R"(
        Request Id: %1;
        Request Message Id: %2;
        Custom Prompt: %3;
        Files Count: %4;
        Messages Count: %5;
        Sending Mode: %6;
        Method: %7;
    )").arg(entry.Id)
       .arg(entry.RequestMessageId)
       .arg(entry.CustomPrompt.value_or("[None]"))
       .arg(entry.Files.size())
       .arg(collectEntries().size())
       .arg(static_cast< int >(mode))
       .arg(entry.HttpRequest.Method);

    m_history[m_lastRequestId] = entry;
    m_overlay->setLastOperationStatus(OperationStatus::Processing);
}


Optional< HttpServiceRequest > LauncherPrivate::createHttpRequest(const MimeDataList& files, Optional< QString > prompt) const
{
    auto config = ConfigManager::instance()->serviceConfig();
    if (!config) {
        qError() << "Cannot create request with an empty config";
        m_overlay->setLastOperationStatus(OperationStatus::Failed);
        return std::nullopt;
    }

    auto requestInfo = m_overlay->requestInfo();
    auto httpRequest = config->findHttpRequest(requestInfo);

    if (!httpRequest) {
        qError() << "Failed to find http request. Try reloading config (Ctr+R)";
        return std::nullopt;
    }

    auto renderingContext = config->createRenderingContext();

    renderingContext->Variables["Files"] = ServiceConfig::GlobalVar::fromValue("Files", ::ToQVariant(files));
    renderingContext->Variables["Messages"] = ServiceConfig::GlobalVar::fromValue("Messages", collectEntries());

    if (prompt)
        renderingContext->Variables["CustomPrompt"] = ServiceConfig::GlobalVar::fromValue("CustomPrompt", *prompt);

    return ServiceConfig::Render(*httpRequest, *renderingContext);
}


namespace
{
    const QColor USER_BUBBLE_COLOR = QColor(0, 120, 215, int(0.10 * 255));
    const QColor OTHER_BUBBLE_COLOR = QColor(50, 50, 50, 10);

    QList< ChatMessage::Attachment > CreateAttachments(const MimeDataList& mimeData)
    {
        QList< ChatMessage::Attachment > result;
        for (const auto& data : mimeData) {
            if (data.Icon)
                result.append({ *data.Icon, data.SourceName, false });
        }

        return result;
    }
}


ChatMessage LauncherPrivate::createUserMessage(const QString& text) const
{
    ChatMessage result;

    result.Text         = text;
    result.Alignment    = ChatMessage::AlignRight;
    result.MessageColor = USER_BUBBLE_COLOR;
    result.Attachments  = CreateAttachments(m_currentFiles);
    result.Timestamp    = QDateTime::currentDateTime();
    result.Id           = m_overlay->appendChatMessage(result);

    return result;
}


ChatMessage LauncherPrivate::createReplyMessage(const ServiceReply& reply) const
{
    ChatMessage result;

    result.Text         = reply.MarkdownText;
    result.Alignment    = ChatMessage::AlignNone;
    result.MessageColor = OTHER_BUBBLE_COLOR;
    result.Attachments  = {};
    result.Timestamp    = QDateTime::currentDateTime();
    result.Id           = m_overlay->appendChatMessage(result);

    return result;
}


bool LauncherPrivate::cancelLastRequest()
{
    if (!m_apiController->cancelCurrentRequest()) {
        return false;
    }

    m_history[m_lastRequestId].IsFailed = true;
    if (auto userMessage = m_overlay->findChatMessage(m_history[m_lastRequestId].RequestMessageId))
    {
        userMessage->get().MessageColor = QColor(Qt::darkRed);
        m_overlay->update(); // trigger repaint
    } else
    {
        qWarning() << "Failed to find user message with id " << m_history[m_lastRequestId].RequestMessageId;
    }

    return true;
}


bool LauncherPrivate::deleteMimeDataAt(int index)
{
    if (m_currentFiles.isEmpty()) return false;

    MimeData data;
    if (index == -1) {
        data = m_currentFiles.takeLast();
    } else if (index >= 0 && index < m_currentFiles.size()) {
        data = m_currentFiles.takeAt(index);
    } else {
        qError() << "Failed to delete screenshot at " << index;
        return false;
    }

    if (!m_overlay->deleteScreenshotPreview(index)) {
        qFatal() << "Failed to delete screenshot in overlay at index " << index
                 << "Something's seriously wrong. Screenshot preview size: " << m_overlay->screenshotPreviewsSize();
    }

    if (!m_overlay->deleteFileMenuItem(data.SourceName)) {
        qFatal() << "Failed to delete file menu item in overlay with the name: " << data.SourceName
                 << "Something's seriously wrong. File menu list size: " << m_overlay->fileMenuListSize();
    }

    m_overlay->setCounter(m_currentFiles.size());

    return true;
}


void LauncherPrivate::addFileMimeData(MimeData data)
{
    if (data.Icon) {
        m_overlay->addScreenshotPreview(*data.Icon);
    } else {
        m_overlay->addScreenshotPreview(QPixmap{});
    }

    qInfo() << QString("Added mime data (%1) from %2 of type %3 %4").arg(data.SourceName)
                                                                    .arg(data.SourcePath)
                                                                    .arg(data.Type.name())
                                                                    .arg(data.IsBase64Encoded ? "base64 encoded" : "not encoded");

    m_overlay->addFileMenuItem(data.SourceName);
    m_currentFiles.append(std::move(data));

    m_overlay->setCounter(m_currentFiles.size());
}


bool LauncherPrivate::clearText()
{
    if (m_overlay->isEmpty()) return false;
    m_overlay->clearText();
    return true;
}


bool LauncherPrivate::cancelConfigLoading()
{
    bool appConfigCanceled = ConfigManager::instance()->cancelAppConfigLoading();
    bool serviceConfigCanceled = ConfigManager::instance()->cancelServiceConfigLoading();
    return appConfigCanceled || serviceConfigCanceled;
}


Optional< ServiceReply > LauncherPrivate::render(const HttpServiceReply& httpReply, const ServiceConfig::RenderingContext& ctx) const
{
    auto config = ConfigManager::instance()->serviceConfig();
    if (!config) {
        qError() << "Cannot create service reply with an empty config";
        m_overlay->setLastOperationStatus(OperationStatus::Failed);
        return std::nullopt;
    }

    auto requestInfo = m_overlay->requestInfo();
    auto httpRequest = config->findHttpRequest(requestInfo);

    if (!httpRequest) {
        qError() << "Failed to find http request. Try reloading config (Ctr+R)";
        return std::nullopt;
    }

    auto renderedText = ServiceConfig::RenderRaw(httpRequest->Display.Reply.Selector, ctx);

    if (!renderedText) {
        qError() << "Failed to render http reply. Change the configuration";
        m_overlay->setLastOperationStatus(OperationStatus::Failed);
        return std::nullopt;
    }

    ServiceReply result;

    result.Code = httpReply.StatusCode;
    result.MarkdownText = *renderedText;

    return result;
}


QVariant LauncherPrivate::ToQVariant(const HistoryEntry& entry) const
{
    QVariantMap result;

    result["Id"]             = entry.Id;
    result["Files"]          = ::ToQVariant(entry.Files);
    result["ServiceRequest"] = ::ToQVariant(entry.HttpRequest);
    result["ServiceReply"]   = ::ToQVariant(entry.HttpReply);

    if (entry.RequestMessageId > 0) result["RequestMessageId"] = entry.RequestMessageId;
    if (entry.ReplyMessageId > 0)   result["ReplyMessageId"]   = entry.ReplyMessageId;
    if (entry.CustomPrompt)         result["CustomPrompt"]     = *entry.CustomPrompt;

    return result;
}


QVariantList LauncherPrivate::collectEntries() const
{
    QVariantList result;

    for (const auto& [id, entry] : m_history.asKeyValueRange()) {
        if (!entry.IsFailed)
            result.push_back(ToQVariant(entry));
    }

    return result;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////
