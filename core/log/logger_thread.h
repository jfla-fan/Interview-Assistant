#pragma once

#include "log.h"

#include <QThread>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>


class LoggerThread : public QThread
{
    Q_OBJECT

public:
    LoggerThread(QObject* parent = nullptr);

    void setMaxQueueSize(int maxQueueSize);
    void enqueueMessage(const LogMessage& message);
    void shutdown();

protected:
    void run() override;

private:
    QQueue< LogMessage > m_messageQueue;
    QMutex m_mutex;
    QWaitCondition m_condition;
    bool m_shutdown = false;
    int m_maxQueueSize = 10000;
};
