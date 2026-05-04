#include "config_manager.h"

#include "log/log.h"

#include "config/app_config.h"
#include "config/service_config.h"

#include <QFutureWatcher>


ConfigManager* ConfigManager::instance()
{
    static ConfigManager s_manager;
    return &s_manager;
}


bool ConfigManager::loadAppConfigFromData(const QString& data,
                                          AppConfig::ConfigLoadMode mode,
                                          bool forceReload)
{
    return loadConfigImpl(m_appConfig,
                          AppConfig::LoadConfigFromData,
                          &ConfigManager::appConfigLoaded,
                          &ConfigManager::appConfigFailed,
                          "",
                          forceReload,
                          data,
                          mode);
}

bool ConfigManager::loadAppConfigFromFile(const QString& path,
                                          AppConfig::ConfigLoadMode mode,
                                          bool forceReload)
{
    return loadConfigImpl(m_appConfig,
                          AppConfig::LoadConfigFromFile,
                          &ConfigManager::appConfigLoaded,
                          &ConfigManager::appConfigFailed,
                          path,
                          forceReload,
                          path,
                          mode);
}


bool ConfigManager::loadAppConfigFromDataAsync(const QString& data,
                                               AppConfig::ConfigLoadMode mode,
                                               bool forceReload)
{
    return loadConfigAsyncImpl(m_appConfig,
                               AppConfig::LoadConfigFromDataAsync,
                               &ConfigManager::appConfigLoaded,
                               &ConfigManager::appConfigFailed,
                               "",
                               forceReload,
                               data,
                               mode);
}


bool ConfigManager::loadAppConfigFromFileAsync(const QString& path,
                                               AppConfig::ConfigLoadMode mode,
                                               bool forceReload)
{
    return loadConfigAsyncImpl(m_appConfig,
                               AppConfig::LoadConfigFromFileAsync,
                               &ConfigManager::appConfigLoaded,
                               &ConfigManager::appConfigFailed,
                               path,
                               forceReload,
                               path,
                               mode);
}


bool ConfigManager::cancelAppConfigLoading()
{
    return cancelConfigLoadingImpl(m_appConfig);
}


bool ConfigManager::saveAppConfigToFile(const QString& path)
{
    return saveConfigImpl(m_appConfig,
                          AppConfig::SaveConfigToFile,
                          &ConfigManager::appConfigSaved,
                          &ConfigManager::appConfigFailed,
                          path);
}


bool ConfigManager::loadServiceConfigFromData(const QString& data,
                                              ServiceConfig::ParserPtr parser,
                                              ServiceConfig::RendererPtr renderer,
                                              bool forceReload)
{
    return loadConfigImpl(m_serviceConfig,
                          ServiceConfig::LoadConfigFromData,
                          &ConfigManager::serviceConfigLoaded,
                          &ConfigManager::serviceConfigFailed,
                          "",
                          forceReload,
                          data,
                          std::move(parser),
                          std::move(renderer));
}


bool ConfigManager::loadServiceConfigFromFile(const QString& path,
                                              ServiceConfig::ParserPtr parser,
                                              ServiceConfig::RendererPtr renderer,
                                              bool forceReload)
{
    return loadConfigImpl(m_serviceConfig,
                          ServiceConfig::LoadConfigFromFile,
                          &ConfigManager::serviceConfigLoaded,
                          &ConfigManager::serviceConfigFailed,
                          path,
                          forceReload,
                          path,
                          std::move(parser),
                          std::move(renderer));
}


bool ConfigManager::loadServiceConfigFromDataAsync(const QString& data,
                                                   ServiceConfig::ParserPtr parser,
                                                   ServiceConfig::RendererPtr renderer,
                                                   bool forceReload)
{
    return loadConfigAsyncImpl(m_serviceConfig,
                               ServiceConfig::LoadConfigFromDataAsync,
                               &ConfigManager::serviceConfigLoaded,
                               &ConfigManager::serviceConfigFailed,
                               "",
                               forceReload,
                               data,
                               std::move(parser),
                               std::move(renderer));
}


bool ConfigManager::loadServiceConfigFromFileAsync(const QString& path,
                                                   ServiceConfig::ParserPtr parser,
                                                   ServiceConfig::RendererPtr renderer,
                                                   bool forceReload)
{
    return loadConfigAsyncImpl(m_serviceConfig,
                               ServiceConfig::LoadConfigFromFileAsync,
                               &ConfigManager::serviceConfigLoaded,
                               &ConfigManager::serviceConfigFailed,
                               path,
                               forceReload,
                               path,
                               std::move(parser),
                               std::move(renderer));
}


bool ConfigManager::cancelServiceConfigLoading()
{
    return cancelConfigLoadingImpl(m_serviceConfig);
}


bool ConfigManager::saveServiceConfigToFile(const QString& path)
{
    return saveConfigImpl(m_serviceConfig,
                          ServiceConfig::SaveConfigToFile,
                          &ConfigManager::serviceConfigSaved,
                          &ConfigManager::serviceConfigFailed,
                          path);
}


AppConfig::ConstConfigPtr ConfigManager::appConfig() const noexcept
{
    return getConfigImpl(m_appConfig);
}


ServiceConfig::ConstConfigPtr ConfigManager::serviceConfig() const noexcept
{
    return getConfigImpl(m_serviceConfig);
}


ConfigManager& ConfigManager::setAppConfig(AppConfig::ConfigPtr config)
{
    setConfigImpl(m_appConfig, std::move(config), &ConfigManager::appConfigLoaded);
    return *this;
}


ConfigManager& ConfigManager::setAppConfig(AppConfig::ConstConfigPtr config)
{
    return setAppConfig(config->clone());
}


ConfigManager& ConfigManager::setServiceConfig(ServiceConfig::ConfigPtr config)
{
    setConfigImpl(m_serviceConfig, std::move(config), &ConfigManager::serviceConfigLoaded);
    return *this;
}


ConfigManager& ConfigManager::setServiceConfig(ServiceConfig::ConstConfigPtr config)
{
    return setServiceConfig(config->clone());
}


ConfigManager::ConfigManager(QObject *parent)
    : QObject{ parent }
    , m_appConfig("App Config")
    , m_serviceConfig("Service Config")
{ }


template< class TState >
typename TState::ConstConfigType ConfigManager::getConfigImpl(const TState& state) const noexcept
{
    QReadLocker locker { &state.lock };
    return state.config;
}


template< class TState, class TReadySignal >
void ConfigManager::setConfigImpl(TState& state,
                                  typename TState::ConfigType config,
                                  TReadySignal readySignal)
{
    {
        QWriteLocker locker { &state.lock };
        state.config = std::move(config);
    }

    emit (this->*readySignal)(state.config);
}


template< class TState >
void ConfigManager::setAsyncLoadResultNoLock(TState& state,
                                             const AsyncLoadResult< typename TState::ConfigType >& result)
{
    state.config = result.config;
    state.path = result.path;
}


template< class TState >
void ConfigManager::setAsyncLoadResult(TState& state,
                                       const AsyncLoadResult< typename TState::ConfigType >& result)
{
    QWriteLocker locker { &state.lock };
    state.config = result.config;
    state.path = result.path;
}


template< class TState, class TReadySignal >
void ConfigManager::setConfigNoLockImpl(TState& state,
                                        typename TState::ConfigType config,
                                        TReadySignal readySignal)
{
    state.config = std::move(config);
    emit (this->*readySignal)(state.config);
}


template<class TState, class TLoadFunc, class TReadySignal, class TFailedSignal, class... TArgs>
bool ConfigManager::loadConfigImpl(TState& state,
                                   TLoadFunc&& loadFunc,
                                   TReadySignal readySignal,
                                   TFailedSignal failedSignal,
                                   const QString& path,
                                   bool forceReload,
                                   TArgs&&... args)
{
    {
        QWriteLocker locker { &state.lock };

        if (state.loadingFuture.isRunning()) {

            if (!forceReload) {
                locker.unlock();
                LOG_WARNING("Ignoring {} loading request, already in progress.", state.debugName);
                emit (this->*failedSignal)(path, Reason_AlreadyLoading);
                return false;
            }

            state.loadingFuture.cancel();
        }
    }

    try {
        if (auto loadedConfig = std::invoke(std::forward< TLoadFunc >(loadFunc), std::forward< TArgs >(args)...)) {
            setAsyncLoadResult(state, { loadedConfig, path });
            emit (this->*readySignal)(loadedConfig);
            return true;
        }
    } catch (const std::exception& ex) {
        LOG_ERROR("Exception raised during {} loading from {}: {}", state.debugName, path, ex.what());
        emit (this->*failedSignal)(path, Reason_ExceptionRaised);
        return false;
    }

    LOG_ERROR("Failed to load {} at {}", state.debugName, path);
    emit (this->*failedSignal)(path, Reason_Other);

    return false;
}


template< class TState, class TLoadFunc, class TReadySignal, class TFailedSignal, class... TArgs>
bool ConfigManager::loadConfigAsyncImpl(TState& state,
                                        TLoadFunc&& loadFunc,
                                        TReadySignal readySignal,
                                        TFailedSignal failedSignal,
                                        const QString& path,
                                        bool forceReload,
                                        TArgs&&... args)
{
    {
        QWriteLocker locker { &state.lock };

        if (state.loadingFuture.isRunning()) {
            if (!forceReload) {
                locker.unlock();
                LOG_WARNING("Ignoring {} loading request from {}, already in progress", state.debugName, path);
                emit (this->*failedSignal)(path, Reason_AlreadyLoading);
                return false;
            }

            state.loadingFuture.cancel();
        }

        try {
            state.loadingFuture = std::invoke(std::forward< TLoadFunc >(loadFunc), std::forward< TArgs >(args)...);

            state.loadingFuture.onCanceled([this, failedSignal, path, &state] {
                LOG_WARNING("Future of config {} has been canceled", state.debugName);
                emit (this->*failedSignal)(path, Reason_Canceled);
                return nullptr;
            }).onFailed([this, failedSignal, path, &state] (const std::exception& ex) {
                LOG_ERROR("Future of config {} has exception raised: {}", state.debugName, ex.what());
                emit (this->*failedSignal)(path, Reason_ExceptionRaised);
                return nullptr;
            }).then([this, failedSignal, readySignal, path, &state](typename TState::ConfigType config) {
                if (state.loadingFuture.isCanceled()) {
                    return;
                }

                if (!config) {
                    LOG_ERROR("Config ({}) loading failed at path <{}>.", state.debugName, path);
                    emit (this->*failedSignal)(path, Reason_Other);
                    return;
                }

                setAsyncLoadResult(state, { config, path });
                emit (this->*readySignal)(config);
            });

        } catch (const std::exception& ex) {
            locker.unlock();
            LOG_ERROR("Exception raised during {} loading from {}: {}", state.debugName, path, ex.what());
            emit (this->*failedSignal)(path, Reason_ExceptionRaised);
            return false;
        }
    }

    return true;
}


template< class TState >
bool ConfigManager::cancelConfigLoadingImpl(TState& state)
{
    QWriteLocker locker{ &state.lock };

    // LOG_DEBUG("isStarted: {}, isRunning: {}, isCanceled: {}, isFinished: {}, isSuspended: {}, isSuspending: {}",
    //           state.loadingFuture.isStarted(), state.loadingFuture.isRunning(), state.loadingFuture.isCanceled(),
    //           state.loadingFuture.isFinished(), state.loadingFuture.isSuspended(), state.loadingFuture.isSuspending());

    if (!state.loadingFuture.isCanceled() && state.loadingFuture.isRunning()) {
        state.loadingFuture.cancel();
        LOG_DEBUG("Config {} canceled", state.debugName);
        return true;
    }

    LOG_DEBUG("Config {} not running or already canceled", state.debugName);
    return false;
}


template< class TState, class TSaveFunc, class TReadySignal, class TFailedSignal, class... TArgs >
bool ConfigManager::saveConfigImpl(TState& state,
                                   TSaveFunc&& func,
                                   TReadySignal readySignal,
                                   TFailedSignal failedSignal,
                                   const QString& path,
                                   TArgs&&... args)
{
    auto config = getConfigImpl(state);

    if (!config) {
        LOG_ERROR("Failed to save {} at {}", state.debugName, path);
        emit (this->*failedSignal)(path, Reason_ConfigNotSet);
        return false;
    }

    if (!std::invoke(std::forward< TSaveFunc >(func), path, config, std::forward< TArgs >(args)...)) {
        LOG_ERROR("Failed to save {} at {}", state.debugName, path);
        emit (this->*failedSignal)(path, Reason_Other);
        return false;
    }

    LOG_INFO("Successfully saved {} at {}", state.debugName, path);
    emit (this->*readySignal)(config);

    return true;
}
