#include "launcher.h"

#include "utils/platform.h"

#include <QApplication>
#include <QCommandLineParser>


int main(int argc, char* argv[])
{
    utils::AttachToParentTerminal();

    if (utils::IsAlreadyRunning()) {
        qCritical() << "App is already running, closing";
        return EXIT_FAILURE;
    }

#ifdef Q_OS_WIN
    utils::ScopedCallWndProcHook hook {};
#endif

    QApplication app(argc, argv);

    QApplication::setQuitOnLastWindowClosed(false);
    QApplication::setApplicationName("AI_Assistant");

    Launcher launcher;

    return app.exec();
}
