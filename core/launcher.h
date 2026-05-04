#pragma once

#include <QObject>

QT_FORWARD_DECLARE_CLASS(MarkdownViewer)
QT_FORWARD_DECLARE_CLASS(LauncherPrivate)


class Launcher : public QObject
{
    Q_OBJECT
public:
    explicit Launcher(QObject *parent = nullptr);
    ~Launcher() override;

private:
    void initializeAppConfiguration();
    void initializeLoggingSystem();
    void initializeFinalConfiguration();
    void initializeHotkeys();
    void createConnections();

    std::unique_ptr< LauncherPrivate > d_ptr;
    Q_DECLARE_PRIVATE(Launcher)
};
