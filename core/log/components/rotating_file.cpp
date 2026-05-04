#include "rotating_file.h"

#include <QFile>
#include <QDir>


RotatingFileLogComponent::RotatingFileLogComponent(QString name,
                                                   QString basePath,
                                                   LogFormatterPtr formatter,
                                                   qint64 maxSize,
                                                   int maxFiles)
    : ILogComponent(std::move(name), std::move(formatter))
    , m_basePath(std::move(basePath))
    , m_fileExtension("")
    , m_maxSize(maxSize)
    , m_maxFiles(maxFiles)
{ }


bool RotatingFileLogComponent::initialize()
{
    if (m_maxFiles < 1) {
        LOG_ERROR("Max file {} < 1", m_maxFiles);
        return false;
    }

    if (m_maxSize < 0) {
        LOG_ERROR("Max size {} < 0", m_maxSize);
        return false;
    }

    QFileInfo fileInfo(m_basePath);
    QDir dir = fileInfo.dir();

    m_fileExtension = fileInfo.suffix();
    m_logDirectory = dir.path();
    m_fileName = fileInfo.completeBaseName();

    if (!dir.exists()) {
        dir.mkpath(".");
    }

    if (!openFile(m_basePath)) {
        return false;
    }

    writeGreetingMessage(SessionType::ProgramSession);
    return true;
}


bool RotatingFileLogComponent::log(const LogMessage& message)
{
    if (!isHealthy()) return false;
    if (message.level < m_minLevel) return true;

    if (m_file->size() > m_maxSize) {
        rotateFileInternal();
    }

    QString formatted = formatMessage(message);
    *m_stream << formatted << Qt::endl;
    m_stream->flush();

    return true;
}


bool RotatingFileLogComponent::isHealthy() const
{
    return m_stream != nullptr;
}


bool RotatingFileLogComponent::shutdown()
{
    if (isHealthy()) {
        writeFinishMessage(SessionType::ProgramSession);
    }

    if (m_stream) {
        m_stream->flush();
        m_stream.reset();
    }
    if (m_file) {
        m_file->close();
        m_file.reset();
    }

    return true;
}


std::unique_ptr< RotatingFileLogComponent >
RotatingFileLogComponent::create(QString name,
                                 QString basePath,
                                 LogFormatterPtr formatter,
                                 qint64 maxSize,
                                 int maxFiles)
{
    return std::make_unique< RotatingFileLogComponent >(std::move(name),
                                                        std::move(basePath),
                                                        std::move(formatter),
                                                        maxSize,
                                                        maxFiles);
}


qint64 RotatingFileLogComponent::getEstimateMaxTotalSize() const
{
    return (m_maxFiles + 1) * m_maxSize;
}


void RotatingFileLogComponent::rotateFileInternal()
{
    writeFinishMessage(SessionType::LogSession);

    m_stream.reset();
    m_file->close();

    // Rotate existing files
    for (int i = m_maxFiles - 1; i > 0; --i) {
        QString oldPath = getFileName(i);
        QString newPath = getFileName(i + 1);

        if (QFile::exists(newPath)) {
            QFile::remove(newPath);
        }
        if (QFile::exists(oldPath)) {
            QFile::rename(oldPath, newPath);
        }
    }

    // Move current file to .1
    QString backupName = getFileName(1);
    if (QFile::exists(backupName)) {
        QFile::remove(backupName);
    }
    QFile::rename(m_basePath, backupName);

    // Create new file
    if (openFile(m_basePath))
    {
        writeGreetingMessage(SessionType::LogSession);
    }
}


bool RotatingFileLogComponent::openFile(const QString& path)
{
    m_file = std::make_unique< QFile >(path);

    if (!m_file->open(QIODevice::WriteOnly | QIODevice::Append)) {
        LOG_ERROR("Failed to open file: {}", path);
        m_stream.reset();
        return false;
    }

    m_stream = std::make_unique< QTextStream >(m_file.get());
    return true;
}


QString RotatingFileLogComponent::getFileName(int number)
{
    return QString("%1/%2.%3.%4").arg(m_logDirectory)
                                 .arg(m_fileName)
                                 .arg(QString::number(number))
                                 .arg(m_fileExtension);
}


const QString& RotatingFileLogComponent::SessionTypeToString(SessionType st) const
{
    Q_STATIC_ASSERT(static_cast< fmt::underlying_t< SessionType > >(SessionType::Max) == 2);
    static QList< QString > s_stringSessions = { "LOG SESSION", "PROGRAM SESSION" };
    return s_stringSessions.at(static_cast< fmt::underlying_t< SessionType > >(st));
}


static constexpr qsizetype s_repeatedSignCount = 70;

void RotatingFileLogComponent::writeGreetingMessage(SessionType st)
{
    QDateTime now = QDateTime::currentDateTime();
    QString greeting = QString("=").repeated(s_repeatedSignCount) +
                       QString(" %1 STARTED: %2 ").arg(SessionTypeToString(st))
                           .arg(now.toString(Qt::ISODate)) +
                       QString("=").repeated(s_repeatedSignCount) +
                       QChar('\n');

    *m_stream << greeting;
}


void RotatingFileLogComponent::writeFinishMessage(SessionType st)
{
    QDateTime now = QDateTime::currentDateTime();
    QString finish = QString("=").repeated(s_repeatedSignCount) +
                     QString(" %1 ENDED: %2 ").arg(SessionTypeToString(st))
                                            .arg(now.toString(Qt::ISODate)) +
                     QString("=").repeated(s_repeatedSignCount) +
                     QChar('\n');

    *m_stream << finish;
}
