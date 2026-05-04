#include "platform.h"

#include <qt_windows.h>
#include <io.h>

#include <QDebug>


namespace
{
#ifdef Q_OS_WIN
bool g_noCaptureFlag = true;
HHOOK g_callWndProcHook = nullptr;

LRESULT CALLBACK CallWndProcHook(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0) {
        const CWPSTRUCT* pCwp = reinterpret_cast<const CWPSTRUCT*>(lParam);

        if (pCwp->message == WM_INITDIALOG)
        {
            utils::SetWindowNoCapture(reinterpret_cast< quintptr >(pCwp->hwnd), g_noCaptureFlag);
        }
    }

    return CallNextHookEx(g_callWndProcHook, nCode, wParam, lParam);
}
#endif
}


bool utils::SetWindowNoCapture(quintptr wid, bool value)
{
#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast< HWND >(wid);
    return SetWindowDisplayAffinity(hwnd, value ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE);
#else
    return false;
#endif
}


bool utils::IsRunningInTerminal()
{
#ifdef Q_OS_WIN
    // static bool result = _isatty(_fileno(stdout));
    return _isatty(_fileno(stdout));
#else
    return false;
#endif
}


bool utils::AttachToParentTerminal()
{
#ifdef Q_OS_WIN
    bool success = AttachConsole(ATTACH_PARENT_PROCESS) &&
                   std::freopen("CONOUT$", "w", stdout) &&
                   std::freopen("CONOUT$", "w", stderr) &&
                   std::freopen("CONIN$", "r", stdin);

    return success;
#else
    return false;
#endif
}


bool utils::IsStdoutValid()
{
#ifdef Q_OS_WIN
    return _fileno(stdout) >= 0;
#else
    return false;
#endif
}


bool utils::IsAlreadyRunning()
{
#ifdef Q_OS_WIN
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"Global\\InterviewAssistant_SingleInstanceMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return true;
    }

    return false;
#else
#error Not defined
#endif
}


utils::ScopedCallWndProcHook::ScopedCallWndProcHook()
{
    g_callWndProcHook = SetWindowsHookEx(WH_CALLWNDPROC, CallWndProcHook, nullptr, GetCurrentThreadId());
}


utils::ScopedCallWndProcHook::~ScopedCallWndProcHook()
{
    if (g_callWndProcHook)
    {
        UnhookWindowsHookEx(g_callWndProcHook);
        g_callWndProcHook = nullptr;
    }
}


void utils::ScopedCallWndProcHook::setNoCapture(bool value)
{
    g_noCaptureFlag = value;
}
