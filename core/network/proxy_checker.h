#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include "../common.h"


QT_FORWARD_DECLARE_CLASS(QNetworkReply)
QT_FORWARD_DECLARE_CLASS(QSslError)


class ProxyChecker : public QObject
{
    Q_OBJECT

public:
    explicit ProxyChecker(const ServiceProxy& proxy, QObject *parent = nullptr);
    ~ProxyChecker() override;

    void setProxy(const ServiceProxy& proxy);
    const ServiceProxy& proxy() const { return m_proxy; }

    void startCheck();
    void cancelCheck();
    OperationStatus status() const { return m_status; }
    const QString& lastError() const { return m_lastError; }

signals:
    void statusChanged(OperationStatus newStatus, const QString& details);

private slots:
    void onReplyFinished();
    void onSslErrors(const QList<QSslError>& errors);

private:
    void setStatus(OperationStatus newStatus, const QString& details = "");

    ServiceProxy           m_proxy;
    OperationStatus        m_status;
    QString                m_lastError;

    QNetworkAccessManager* m_manager;
    QNetworkReply*         m_currentReply;
};
