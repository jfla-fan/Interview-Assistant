#include "api_controller_p.h"

#include <QNetworkProxy>


namespace
{
    constexpr const char* PROXY_NAME_PROPERTY = "ProxyName";
}


HttpApiController::HttpApiController(QObject* parent)
    : QObject(parent)
    , d_ptr(new HttpApiControllerPrivate)
{ }

HttpApiController::~HttpApiController() = default;


void HttpApiController::initialize()
{
    Q_D(HttpApiController);
    d->m_manager->setTransferTimeout(std::chrono::minutes { 10 });
}


void HttpApiController::sendHttpRequest(const HttpServiceRequest& request)
{
    Q_D(HttpApiController);
    cancelCurrentRequest();

    d->m_currentRequest.serviceRequest = request;
    d->m_currentRequest.proxyIndex = 0;
    d->m_currentRequest.proxies = d->m_proxies;

    if (request.UseProxies && !d->m_currentRequest.proxies.isEmpty()) {
        setProxy(d->m_currentRequest.proxyIndex);
        qDebug() << "Using proxy: " << d->m_manager->proxy();
    } else {
        setProxy(-1);
        qDebug() << "No proxy is used";
    }

    doSendHttpRequest();
}


void HttpApiController::setProxies(QList< ServiceProxy > proxies)
{
    Q_D(HttpApiController);
    d->m_proxies = proxies;
}


bool HttpApiController::cancelCurrentRequest()
{
    Q_D(HttpApiController);

    if (d->m_currentRequest.status == Status_Loading && d->m_currentRequest.networkReply) {

        d->m_currentRequest.networkReply->abort();
        d->m_currentRequest.status = Status_Cancelled;
        qDebug() << "Request cancelled";

        return true;
    }

    return false;
}


HttpApiController::RequestStatus HttpApiController::currentRequestStatus() const
{
    const Q_D(HttpApiController);
    return d->m_currentRequest.status;
}


void HttpApiController::onReplyFinished(int requestId)
{
    Q_D(HttpApiController);

    qDebug() << "On reply finished, request id - " << requestId;

    QNetworkReply* reply = qobject_cast< QNetworkReply* >(sender());
    if (d->m_currentRequest.networkReply != reply )
    {
        qWarning() << "Already cancelled"; // unexpected
        return;
    }

    d->m_currentRequest.networkReply = nullptr;

    QVariant proxyName = d->m_manager->property(PROXY_NAME_PROPERTY);
    int httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (reply->error() == QNetworkReply::NoError && httpStatusCode >= 200 && httpStatusCode < 300) {
        HttpServiceReply serviceReply = CreateServiceReply(reply, requestId);

        // qDebug() << "Service reply (delete this log): " << serviceReply.Body;

        d->m_currentRequest.status = Status_Finished;
        emit replyReady(serviceReply);

        if (!proxyName.isNull()) {
            emit proxySucceeded(proxyName.toString());
        }

        reply->deleteLater();
        qDebug() << "Received QNetworkReply: " << reply->error();

        return;
    }

    qWarning() << "Network request failed for ID " << requestId
               << "Error: " << reply->error()
               << "Error String: " << reply->errorString()
               << "HTTP Status Code: " << httpStatusCode
               << "Response body: " << reply->readAll();

    if (!proxyName.isNull()) {
        emit proxyFailed(proxyName.toString());
    }

    qWarning() << "Received QNetworkReply: " << reply->error();

    // if proxies are loaded
    if (!d->m_currentRequest.proxies.isEmpty()) {
        int currProxyIndex = ++d->m_currentRequest.proxyIndex;

        // try next proxy
        if (currProxyIndex < d->m_currentRequest.proxies.size()) {
            const auto& proxy = d->m_currentRequest.proxies.at(currProxyIndex);

            qInfo() << "Gonna try next proxy: " << proxy.Name << " " << proxy.NetworkProxy;

            setProxy(currProxyIndex);
            doSendHttpRequest();

            return;
        }

        qWarning() << "All proxies failed. ";
    }

    HttpServiceError serviceError = CreateServiceError(reply, requestId);

    // qDebug() << QString("Reply error - %2, status - %1").arg(serviceError.Code)
    //                                                     .arg(serviceError.ErrorDescription);

    d->m_currentRequest.status = Status_Error;
    emit errorOccurred(serviceError);
    reply->deleteLater();
}


void HttpApiController::setProxy(int index)
{
    Q_D(HttpApiController);

    OptionalCRef< ServiceProxy > proxy{};
    if (index >= 0) proxy = d->m_proxies.at(index);

    if (!proxy) {
        d->m_manager->setProxy(QNetworkProxy::NoProxy);
        d->m_manager->setProperty(PROXY_NAME_PROPERTY, {});
        return;
    }

    d->m_manager->setProxy(proxy->get().NetworkProxy);
    d->m_manager->setProperty(PROXY_NAME_PROPERTY, proxy->get().Name);
}


void HttpApiController::doSendHttpRequest()
{
    Q_D(HttpApiController);

    auto& currentNetworkRequest = d->m_currentRequest.networkRequest;
    const auto& serviceRequest = d->m_currentRequest.serviceRequest;
    const QString method = d->m_currentRequest.serviceRequest.Method;

    QNetworkRequest networkRequest;
    networkRequest.setUrl(serviceRequest.Url);

    for (auto it = serviceRequest.Headers.constBegin(); it != serviceRequest.Headers.constEnd(); ++it) {
        networkRequest.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }

    currentNetworkRequest = std::move(networkRequest);

    qInfo() << QString("Method (%1), name (%2), url (%3), verb(%4)").arg(method).arg(serviceRequest.Name)
                                                                    .arg(serviceRequest.Url.toString()).arg(method.toLatin1());
    qDebug() << "Transferring body size: " << serviceRequest.Body.size();
    // qDebug().noquote() << "Body: " << serviceRequest.Body;

    // return; // delete later

    d->m_currentRequest.networkReply = d->m_manager->sendCustomRequest(currentNetworkRequest, method.toLatin1(), serviceRequest.Body);

    // if (request.IgnoreSslErrors)
    //     d->m_currentRequest.networkReply->ignoreSslErrors();

    if (serviceRequest.IgnoreSslErrors) {
        // Connect to the sslErrors signal and tell the reply to proceed anyway.
        connect(d->m_currentRequest.networkReply, &QNetworkReply::sslErrors, d->m_currentRequest.networkReply, [d](const QList<QSslError> &errors) {
            qWarning() << "SSL Errors encountered, but ignoring due to configuration:" << errors;
            d->m_currentRequest.networkReply->ignoreSslErrors(errors);
        });
    }

    connect(d->m_currentRequest.networkReply, &QNetworkReply::finished, this, [id=serviceRequest.Id, this]() { HttpApiController::onReplyFinished(id); });

    d->m_currentRequest.status = Status_Loading;
}
