#pragma once

#include "../log_component.h"

#include <QFile>



class RotatingFileLogComponent : public ILogComponent
{
public:
    RotatingFileLogComponent(QString name,
                             QString basePath,
                             LogFormatterPtr formatter,
                             qint64 maxSize,
                             int maxFiles);

    bool initialize() override;
    bool log(const LogMessage& message) override;
    bool isHealthy() const override;
    bool shutdown() override;

    static std::unique_ptr< RotatingFileLogComponent > create(QString name,
                                                              QString basePath,
                                                              LogFormatterPtr formatter,
                                                              qint64 maxSize = 10 * 1024 * 1024,
                                                              int maxFiles = 5);
    qint64 getEstimateMaxTotalSize() const;
    void rotateFileInternal();
private:
    bool openFile(const QString& path);
    QString getFileName(int number);

    enum class SessionType : uint8_t {
        LogSession,     // new log session after log rotation
        ProgramSession, // new program start
        Max             // meta
    };
    const QString& SessionTypeToString(SessionType st) const;

    void writeGreetingMessage(SessionType st);
    void writeFinishMessage(SessionType st);

    QString m_basePath;
    QString m_logDirectory;
    QString m_fileName;
    QString m_fileExtension;
    qint64 m_maxSize;
    int m_maxFiles;
    std::unique_ptr<QFile> m_file;
    std::unique_ptr<QTextStream> m_stream;
};
