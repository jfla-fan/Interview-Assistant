#pragma once

#include <QObject>
#include <QFutureWatcher>

#include "../common.h"

#include "service_config_fwd.h"


namespace ServiceConfig
{

    class ConfigLoader : public QObject
    {
        Q_OBJECT

    public:
        explicit ConfigLoader(QObject* parent = nullptr);
        ~ConfigLoader() override;

        using Status = ::OperationStatus;

        void loadFromData(const QString& text, ServiceConfig::ParserPtr parser, ServiceConfig::RendererPtr renderer);
        void reset(ServiceConfig::ConstConfigPtr config);
        void cancel();

        Status status() const { return m_status; }
        bool isLoading() const { return m_status == OperationStatus::Processing; }
        ServiceConfig::ConstConfigPtr lastValidConfig() const { return m_lastResult; }

    signals:
        void statusChanged(Status newStatus);

    private slots:
        void onLoadingFinished();

    private:
        void setStatus(Status newStatus);

        QFutureWatcher< ServiceConfig::ConfigPtr > m_watcher;
        ServiceConfig::ConstConfigPtr m_lastResult;
        Status m_status;
    };

}
