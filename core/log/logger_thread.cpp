#include "logger_thread.h"


LoggerThread::LoggerThread(QObject* parent)
    : QThread(parent)
{ }


void LoggerThread::setMaxQueueSize(int maxQueueSize)
{
    if (maxQueueSize < 0) {
        LOG_ERROR("Max queue size must be >= 0, got - {}", maxQueueSize);
        return;
    }

    m_maxQueueSize = maxQueueSize;
}


void LoggerThread::enqueueMessage(const LogMessage& message)
{
    QMutexLocker locker(&m_mutex);
    if (m_messageQueue.size() < m_maxQueueSize) {
        m_messageQueue.enqueue(message);
        m_condition.wakeOne();
    }
}


void LoggerThread::shutdown()
{
    QMutexLocker locker(&m_mutex);
    m_shutdown = true;
    m_condition.wakeAll();
}


void LoggerThread::run()
{
    while (!m_shutdown) {
        QMutexLocker locker(&m_mutex);
        while (m_messageQueue.isEmpty() && !m_shutdown) {
            m_condition.wait(&m_mutex);
        }

        while (!m_messageQueue.isEmpty()) {
            LogMessage message = m_messageQueue.dequeue();
            locker.unlock();
            LogManager::instance()->broadcastSync(message);
            locker.relock();
        }
    }
}
