#pragma once

#include <QObject>
#include <QString>
#include <QHash>

QT_FORWARD_DECLARE_CLASS(HttpApiControllerPrivate)
QT_FORWARD_DECLARE_STRUCT(HttpServiceRequest)
QT_FORWARD_DECLARE_STRUCT(HttpServiceReply)
QT_FORWARD_DECLARE_STRUCT(ServiceProxy)
QT_FORWARD_DECLARE_STRUCT(HttpServiceError)


class HttpApiController : public QObject
{
    Q_OBJECT
public:
    enum RequestStatus {
        Status_Loading,
        Status_Finished,
        Status_SslError,
        Status_Error,
        Status_Cancelled
    };

public:
    HttpApiController(QObject* parent = nullptr);
    ~HttpApiController();

    void initialize();
    void sendHttpRequest(const HttpServiceRequest& request);
    void setProxies(QList< ServiceProxy > proxies);
    bool cancelCurrentRequest();

    RequestStatus currentRequestStatus() const;
signals:
    void replyReady(const HttpServiceReply& reply);
    void errorOccurred(const HttpServiceError& error);
    void statusChanged(RequestStatus oldStatus, RequestStatus currentStatus);
    void proxySucceeded(const QString& name);
    void proxyFailed(const QString& name);

private slots:
    void onReplyFinished(int requestId);

private:
    void setProxy(int index);
    void doSendHttpRequest();

    std::unique_ptr< HttpApiControllerPrivate > d_ptr;
    Q_DECLARE_PRIVATE(HttpApiController)
};
