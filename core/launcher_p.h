#pragma once

#include "common.h"
#include "action.h"

#include "config/service_config_fwd.h"


QT_FORWARD_DECLARE_CLASS(MarkdownViewer)
QT_FORWARD_DECLARE_CLASS(HttpApiController)
QT_FORWARD_DECLARE_STRUCT(ServiceReply)
QT_FORWARD_DECLARE_CLASS(ILogComponent)
QT_FORWARD_DECLARE_CLASS(RotatingFileLogComponent)


class LauncherPrivate : public QObject
{
public:
    LauncherPrivate(QObject* parent = nullptr) : QObject(parent) { }

    std::unique_ptr< MarkdownViewer > m_overlay;
    std::unique_ptr< HttpApiController > m_apiController;
    std::unique_ptr< ILogComponent > m_consoleLogComponent;
    std::unique_ptr< RotatingFileLogComponent > m_rotatingFileLogComponent;

    struct HistoryEntry
    {
        int                 Id;
        int                 RequestMessageId = -1; // -1 if not defined
        int                 ReplyMessageId   = -1; // -1 if not defined
        bool                IsFailed;
        MimeDataList        Files;
        Optional< QString > CustomPrompt;
        HttpServiceRequest  HttpRequest;
        HttpServiceReply    HttpReply;
        ServiceReply        ServiceReply;
    };

    QMap< int, HistoryEntry > m_history;
    int m_lastRequestId = 0;
    bool m_saveAppConfig = false; // do we need to save after loading
    bool m_saveServiceConfig = false;
    QString m_serviceConfigPath;
    QString m_appConfigPath;

    MimeDataList m_currentFiles;

public slots:
    void handleAction(ActionType action);
    void handleHttpServiceReply(const HttpServiceReply& httpReply);
    void handleHttpServiceError(const HttpServiceError& serviceError);
    void handleProxySucceeded(const QString& proxyName);
    void handleProxyFailed(const QString& proxyName);
    void handleLoadedConfig(AppConfig::ConstConfigPtr config);
    void handleLoadedConfig(ServiceConfig::ConstConfigPtr config);
    void handleFilesChosen(const QStringList& filePaths);
    void handleMessageSent(const QString& prompt);
    void handleMessageRemoved(const ChatMessage& message);
    void handleScreenshotPreviewClicked(int index);
    void handleScreenshotReady(const QPixmap& screenshot);
    void handleFileMenuItemClicked(const QString& fileName);
    void handleSettingsOk(const SettingsView& data);
    void handleSettingsApply(const SettingsView& data);
    void handleSettingsSave(const SettingsView& data);
    void handleSettingsCancel();

    void takeScreenshot(); // take full desktop screenshot
    void takeScreenshot(const QPixmap& screenshot);
    bool undoLastAction();
    enum ESendMode { SingleRequest_Mode, Chat_Mode };
    void sendCurrentRequest(ESendMode mode, Optional< QString > prompt = {});
    void reloadConfig();

private:
    // utilities
    Optional< HttpServiceRequest > createHttpRequest(const MimeDataList& files, Optional< QString > prompt) const;
    ChatMessage createUserMessage(const QString& text) const;
    ChatMessage createReplyMessage(const ServiceReply& reply) const;
    bool cancelLastRequest();
    bool deleteMimeDataAt(int index = -1);
    void addFileMimeData(MimeData data);
    bool clearText();
    bool cancelConfigLoading();

    Optional< ServiceReply > render(const HttpServiceReply& httpReply, const ServiceConfig::RenderingContext& ctx) const;
    QVariant ToQVariant(const HistoryEntry& entry) const;
    QVariantList collectEntries() const;
};


