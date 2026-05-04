#pragma once

#include "../common.h"

#include "api_controller.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>


class HttpApiControllerPrivate
{
public:
    struct {
        QNetworkRequest networkRequest;
        QNetworkReply* networkReply = nullptr;
        int proxyIndex = 0;
        HttpApiController::RequestStatus status = HttpApiController::Status_Finished;
        HttpServiceRequest serviceRequest;
        QList< ServiceProxy > proxies;
    } m_currentRequest;

    std::unique_ptr< QNetworkAccessManager > m_manager = std::make_unique< QNetworkAccessManager >();
    QList< ServiceProxy > m_proxies;
};
