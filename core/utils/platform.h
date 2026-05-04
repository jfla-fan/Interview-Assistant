#pragma once

#include <QObject>
#include <QtTypes>


namespace utils
{
    bool SetWindowNoCapture(quintptr wid, bool value);
    bool IsRunningInTerminal();
    bool AttachToParentTerminal();
    bool IsStdoutValid();
    bool IsAlreadyRunning();

#ifdef Q_OS_WIN
    /**
        This installs windows procedure hook that intercepts all the messages
        coming to windows. We need it to intercept special WM_INITDIALOG messages
        that go to newly created dialogs.
    */
    class ScopedCallWndProcHook
    {
    public:
        ScopedCallWndProcHook();
        ~ScopedCallWndProcHook();

        ScopedCallWndProcHook(const ScopedCallWndProcHook&) = delete;
        ScopedCallWndProcHook& operator=(const ScopedCallWndProcHook&) = delete;

        static void setNoCapture(bool value);
    };

#endif
}
