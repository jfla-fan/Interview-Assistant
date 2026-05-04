#include "proxy_checker.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QSslConfiguration>
#include <QDebug>


ProxyChecker::ProxyChecker(const ServiceProxy& proxy, QObject *parent)
    : QObject(parent)
    , m_status(OperationStatus::Idle)
    , m_manager(new QNetworkAccessManager)
    , m_currentReply(nullptr)
{
    setProxy(proxy);
}


ProxyChecker::~ProxyChecker()
{
    cancelCheck();
}


void ProxyChecker::setProxy(const ServiceProxy& proxy)
{
    cancelCheck();
    m_proxy = proxy;
    m_manager->setProxy(m_proxy.NetworkProxy);
    setStatus(OperationStatus::Idle);
}


void ProxyChecker::startCheck()
{
    cancelCheck();
    setStatus(OperationStatus::Processing, "Sending request...");

    QNetworkRequest request(QUrl("https://api.ipify.org?format=json"));

    if (m_proxy.IgnoreSslErrors) {
        QSslConfiguration sslConfig = request.sslConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        request.setSslConfiguration(sslConfig);
    }

    request.setTransferTimeout(15000); // 15 seconds
    m_currentReply = m_manager->get(request);

    connect(m_currentReply, &QNetworkReply::finished, this, &ProxyChecker::onReplyFinished);

    if (!m_proxy.IgnoreSslErrors) {
        connect(m_currentReply, &QNetworkReply::sslErrors, this, &ProxyChecker::onSslErrors);
    }
    else {
        m_currentReply->ignoreSslErrors();
    }
}


void ProxyChecker::cancelCheck()
{
    if (m_currentReply) {
        disconnect(m_currentReply, nullptr, this, nullptr);

        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;

        if (m_status == OperationStatus::Processing) {
            setStatus(OperationStatus::Idle, "Check cancelled by user.");
        }
    }
}


void ProxyChecker::onReplyFinished()
{
    if (!m_currentReply) {
        return;
    }

    QScopedPointer< QNetworkReply, QScopedPointerDeleteLater > reply(m_currentReply);
    m_currentReply = nullptr;

    if (reply->error() != QNetworkReply::NoError) {
        setStatus(OperationStatus::Failed, QString("Network Error: %1").arg(reply->errorString()));
        return;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        setStatus(OperationStatus::Failed, QString("JSON Parse Error: %1").arg(parseError.errorString()));
        return;
    }
    if (!doc.isObject()) {
        setStatus(OperationStatus::Failed, "Response is not a valid JSON object.");
        return;
    }

    QString responseIp = doc.object().value("ip").toString();

    if (responseIp.isEmpty()) {
        setStatus(OperationStatus::Failed, "JSON response did not contain an 'ip' field.");
        return;
    }

    setStatus(OperationStatus::Succeeded, qformat("Answer: {}", responseIp));
}


void ProxyChecker::onSslErrors(const QList<QSslError>& errors)
{
    QString errorString;
    for (const auto& error : errors) {
        errorString += error.errorString() + "\n";
    }

    setStatus(OperationStatus::Failed, "SSL Error(s):\n" + errorString.trimmed());
}


void ProxyChecker::setStatus(OperationStatus newStatus, const QString& details)
{
    OperationStatus oldStatus = m_status;
    m_status = newStatus;
    m_lastError = details;

    if (oldStatus != newStatus) {
        emit statusChanged(newStatus, details);
    }
}
