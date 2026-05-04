#pragma once

#include "config/app_config_fwd.h"
#include "config/service_config_fwd.h"

#include <QObject>
#include <QFutureWatcher>
#include <QReadWriteLock>


/**
 * Singleton that loads config files and allows accessing current configurations from any part of the project.
 * To update a config clone or create one and use "set*Config" function. All functions are thread-safe.
 * @note Set "forceReload" argument to true In "load*ConfigFrom*" and "load*ConfigFrom*Async" functions to cancel current loadings.
*/
class ConfigManager : public QObject
{
    Q_OBJECT
public:
    enum FailureReason
    {
        Reason_WrongArgument,
        Reason_ConfigNotSet,
        Reason_AlreadyLoading,
        Reason_Canceled,
        Reason_ExceptionRaised,
        Reason_Other,

        Reason_Max
    };
    Q_ENUM(FailureReason)

public:
    static ConfigManager* instance();

    bool loadAppConfigFromData(const QString& data,
                               AppConfig::ConfigLoadMode mode,
                               bool forceReload = false);

    bool loadAppConfigFromFile(const QString& path,
                               AppConfig::ConfigLoadMode mode,
                               bool forceReload = false);

    bool loadAppConfigFromDataAsync(const QString& data,
                                    AppConfig::ConfigLoadMode mode,
                                    bool forceReload = false);

    bool loadAppConfigFromFileAsync(const QString& path,
                                    AppConfig::ConfigLoadMode mode,
                                    bool forceReload = false);

    bool cancelAppConfigLoading();

    bool saveAppConfigToFile(const QString& path);

    bool loadServiceConfigFromData(const QString& data,
                                   ServiceConfig::ParserPtr parser,
                                   ServiceConfig::RendererPtr renderer,
                                   bool forceReload = false);

    bool loadServiceConfigFromFile(const QString& path,
                                   ServiceConfig::ParserPtr parser,
                                   ServiceConfig::RendererPtr renderer,
                                   bool forceReload = false);

    bool loadServiceConfigFromDataAsync(const QString& data,
                                        ServiceConfig::ParserPtr parser,
                                        ServiceConfig::RendererPtr renderer,
                                        bool forceReload = false);

    bool loadServiceConfigFromFileAsync(const QString& path,
                                        ServiceConfig::ParserPtr parser,
                                        ServiceConfig::RendererPtr renderer,
                                        bool forceReload = false);

    bool cancelServiceConfigLoading();

    bool saveServiceConfigToFile(const QString& path);

    AppConfig::ConstConfigPtr appConfig() const noexcept;
    ServiceConfig::ConstConfigPtr serviceConfig() const noexcept;

    ConfigManager& setAppConfig(AppConfig::ConfigPtr config);
    ConfigManager& setAppConfig(AppConfig::ConstConfigPtr config);

    ConfigManager& setServiceConfig(ServiceConfig::ConfigPtr config);
    ConfigManager& setServiceConfig(ServiceConfig::ConstConfigPtr config);

signals:
    void appConfigLoaded(AppConfig::ConstConfigPtr config);
    void appConfigSaved(AppConfig::ConstConfigPtr config);
    void appConfigFailed(const QString& path, FailureReason reason);

    void serviceConfigLoaded(ServiceConfig::ConstConfigPtr config);
    void serviceConfigSaved(ServiceConfig::ConstConfigPtr config);
    void serviceConfigFailed(const QString& path, FailureReason reason);

private:
    ConfigManager(QObject *parent = nullptr);

    template< class TConfig >
    struct AsyncLoadResult
    {
        TConfig config;
        QString path;
    };

    template< class TConfig, class TConstConfig  >
    struct ConfigState
    {
        using ConfigType        = TConfig;
        using ConstConfigType   = TConstConfig;
        using LoadingFutureType = QFuture< TConfig >;

        const QString debugName;
        QString path;
        TConfig config;
        QString configName;
        mutable QReadWriteLock lock;
        LoadingFutureType loadingFuture;

        ConfigState(QString dbgName) : debugName(std::move(dbgName)) { }
    };

    template< class TState >
    typename TState::ConstConfigType getConfigImpl(const TState& state) const noexcept;

    template< class TState >
    void setAsyncLoadResultNoLock(TState& state,
                                  const AsyncLoadResult< typename TState::ConfigType >& result);

    template< class TState >
    void setAsyncLoadResult(TState& state,
                            const AsyncLoadResult< typename TState::ConfigType >& result);

    template< class TState, class TReadySignal >
    void setConfigNoLockImpl(TState& state,
                             typename TState::ConfigType config,
                             TReadySignal readySignal);

    template< class TState, class TReadySignal >
    void setConfigImpl(TState& state,
                       typename TState::ConfigType config,
                       TReadySignal readySignal);

    template< class TState, class TLoadFunc, class TReadySignal, class TFailedSignal, class... TArgs>
    bool loadConfigImpl(TState& state,
                        TLoadFunc&& loadFunc,
                        TReadySignal readySignal,
                        TFailedSignal failedSignal,
                        const QString& path,
                        bool forceReload,
                        TArgs&&... args);

    template< class TState, class TLoadFunc, class TReadySignal, class TFailedSignal, class... TArgs>
    bool loadConfigAsyncImpl(TState& state,
                             TLoadFunc&& loadFunc,
                             TReadySignal readySignal,
                             TFailedSignal failedSignal,
                             const QString& path,
                             bool forceReload,
                             TArgs&&... args);

    template< class TState >
    bool cancelConfigLoadingImpl(TState& state);

    template< class TState, class TSaveFunc, class TReadySignal, class TFailedSignal, class... TArgs >
    bool saveConfigImpl(TState& state,
                        TSaveFunc&& func,
                        TReadySignal readySignal,
                        TFailedSignal failedSignal,
                        const QString& path,
                        TArgs&&... args);

    using AppConfigState = ConfigState< AppConfig::ConfigPtr, AppConfig::ConstConfigPtr >;
    using ServiceConfigState = ConfigState< ServiceConfig::ConfigPtr, ServiceConfig::ConstConfigPtr >;

    AppConfigState m_appConfig;
    ServiceConfigState m_serviceConfig;
};
