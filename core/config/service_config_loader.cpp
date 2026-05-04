#include "service_config_loader.h"
#include "service_config.h"

#include <QDebug>


namespace ServiceConfig
{

ConfigLoader::ConfigLoader(QObject* parent)
    : QObject(parent),
      m_status(Status::Idle)
{
    connect(&m_watcher, &QFutureWatcherBase::finished, this, &ConfigLoader::onLoadingFinished);
}


ConfigLoader::~ConfigLoader()
{
    cancel();
}


void ConfigLoader::loadFromData(const QString& text, ServiceConfig::ParserPtr parser, ServiceConfig::RendererPtr renderer)
{
    if (isLoading()) {
        qWarning() << "ConfigLoader::loadFromText called while already loading. Cancelling previous operation.";
        m_watcher.cancel();
    }

    setStatus(Status::Processing);

    auto future = LoadConfigFromDataAsync(text, std::move(parser), std::move(renderer));
    m_watcher.setFuture(std::move(future));
}


void ConfigLoader::reset(ServiceConfig::ConstConfigPtr config)
{
    cancel();
    m_lastResult = config;
}


void ConfigLoader::cancel()
{
    if (isLoading()) {
        qDebug() << "Is loading, has been canceled";
        m_watcher.cancel();
    }
}


void ConfigLoader::onLoadingFinished()
{
    if (m_watcher.isCanceled()) {
        setStatus(Status::Idle);
        return;
    }

    ConfigPtr resultConfig = m_watcher.result();

    if (resultConfig) {
        m_lastResult = resultConfig;
        setStatus(Status::Succeeded);
    } else {
        setStatus(Status::Failed);
    }
}


void ConfigLoader::setStatus(Status newStatus)
{
    if (m_status != newStatus) {
        m_status = newStatus;
        emit statusChanged(newStatus);
    }
}

}
