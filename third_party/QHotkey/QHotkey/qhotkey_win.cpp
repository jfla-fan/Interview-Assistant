#include "qhotkey.h"
#include "qhotkey_p.h"
#include <qt_windows.h>
#include <algorithm>
#include <QDebug>
#include <QList>
#include <QKeyEvent>
#include <QTimer>
#include <QWindow>

#define HKEY_ID(nativeShortcut) (((nativeShortcut.key ^ (nativeShortcut.modifier << 8)) & 0x0FFF) | 0x7000)

namespace {

struct KeyMap {
    Qt::Key qtKey;
    quint32 nativeKey;
};

// This static, constant map is now the single source of truth for all key translations.
static const KeyMap keyMap[] = {
    {Qt::Key_Escape, VK_ESCAPE},
    {Qt::Key_Tab, VK_TAB},
    {Qt::Key_Backtab, VK_TAB},
    {Qt::Key_Backspace, VK_BACK},
    {Qt::Key_Return, VK_RETURN},
    {Qt::Key_Enter, VK_RETURN},
    {Qt::Key_Insert, VK_INSERT},
    {Qt::Key_Delete, VK_DELETE},
    {Qt::Key_Pause, VK_PAUSE},
    {Qt::Key_Print, VK_PRINT},
    {Qt::Key_Clear, VK_CLEAR},
    {Qt::Key_Home, VK_HOME},
    {Qt::Key_End, VK_END},
    {Qt::Key_Left, VK_LEFT},
    {Qt::Key_Up, VK_UP},
    {Qt::Key_Right, VK_RIGHT},
    {Qt::Key_Down, VK_DOWN},
    {Qt::Key_PageUp, VK_PRIOR},
    {Qt::Key_PageDown, VK_NEXT},
    {Qt::Key_CapsLock, VK_CAPITAL},
    {Qt::Key_NumLock, VK_NUMLOCK},
    {Qt::Key_ScrollLock, VK_SCROLL},

    {Qt::Key_F1, VK_F1}, {Qt::Key_F2, VK_F2}, {Qt::Key_F3, VK_F3},
    {Qt::Key_F4, VK_F4}, {Qt::Key_F5, VK_F5}, {Qt::Key_F6, VK_F6},
    {Qt::Key_F7, VK_F7}, {Qt::Key_F8, VK_F8}, {Qt::Key_F9, VK_F9},
    {Qt::Key_F10, VK_F10}, {Qt::Key_F11, VK_F11}, {Qt::Key_F12, VK_F12},
    {Qt::Key_F13, VK_F13}, {Qt::Key_F14, VK_F14}, {Qt::Key_F15, VK_F15},
    {Qt::Key_F16, VK_F16}, {Qt::Key_F17, VK_F17}, {Qt::Key_F18, VK_F18},
    {Qt::Key_F19, VK_F19}, {Qt::Key_F20, VK_F20}, {Qt::Key_F21, VK_F21},
    {Qt::Key_F22, VK_F22}, {Qt::Key_F23, VK_F23}, {Qt::Key_F24, VK_F24},

    {Qt::Key_Menu, VK_APPS},
    {Qt::Key_Help, VK_HELP},
    {Qt::Key_MediaNext, VK_MEDIA_NEXT_TRACK},
    {Qt::Key_MediaPrevious, VK_MEDIA_PREV_TRACK},
    {Qt::Key_MediaPlay, VK_MEDIA_PLAY_PAUSE},
    {Qt::Key_MediaStop, VK_MEDIA_STOP},
    {Qt::Key_VolumeDown, VK_VOLUME_DOWN},
    {Qt::Key_VolumeUp, VK_VOLUME_UP},
    {Qt::Key_VolumeMute, VK_VOLUME_MUTE},
    {Qt::Key_Mode_switch, VK_MODECHANGE},
    {Qt::Key_Select, VK_SELECT},
    {Qt::Key_Printer, VK_PRINT},
    {Qt::Key_Execute, VK_EXECUTE},
    {Qt::Key_Sleep, VK_SLEEP},
    {Qt::Key_Period, VK_DECIMAL},
    {Qt::Key_Play, VK_PLAY},
    {Qt::Key_Cancel, VK_CANCEL},

    {Qt::Key_Forward, VK_BROWSER_FORWARD},
    {Qt::Key_Refresh, VK_BROWSER_REFRESH},
    {Qt::Key_Stop, VK_BROWSER_STOP},
    {Qt::Key_Search, VK_BROWSER_SEARCH},
    {Qt::Key_Favorites, VK_BROWSER_FAVORITES},
    {Qt::Key_HomePage, VK_BROWSER_HOME},

    {Qt::Key_LaunchMail, VK_LAUNCH_MAIL},
    {Qt::Key_LaunchMedia, VK_LAUNCH_MEDIA_SELECT},
    {Qt::Key_Launch0, VK_LAUNCH_APP1},
    {Qt::Key_Launch1, VK_LAUNCH_APP2},

    {Qt::Key_Massyo, VK_OEM_FJ_MASSHOU},
    {Qt::Key_Touroku, VK_OEM_FJ_TOUROKU},
    };

/**
 * @brief Converts a native Windows virtual key code to a Qt::Key.
 * @param nativeKey The Windows VK_... code.
 * @return The corresponding Qt::Key, or Qt::Key_unknown if not found.
 */
Qt::Key qtKeyFromNative(quint32 nativeKey)
{
    // First, search our map of special keys
    const auto it = std::find_if(std::begin(keyMap), std::end(keyMap),
                                 [nativeKey](const KeyMap& item) {
                                     return item.nativeKey == nativeKey;
                                 });

    if (it != std::end(keyMap)) {
        return it->qtKey;
    }

    // If not found, it's likely an alphanumeric key.
    // Windows VK codes for A-Z and 0-9 match their ASCII values.
    if ((nativeKey >= 'A' && nativeKey <= 'Z') || (nativeKey >= '0' && nativeKey <= '9')) {
        return static_cast<Qt::Key>(nativeKey);
    }

    return Qt::Key_unknown;
}

Qt::KeyboardModifiers qtModifiersFromNative(quint32 nativeMods)
{
    Qt::KeyboardModifiers mods = Qt::NoModifier;
    if (nativeMods & MOD_SHIFT) mods |= Qt::ShiftModifier;
    if (nativeMods & MOD_CONTROL) mods |= Qt::ControlModifier;
    if (nativeMods & MOD_ALT) mods |= Qt::AltModifier;
    if (nativeMods & MOD_WIN) mods |= Qt::MetaModifier;
    return mods;
}

} // end anonymous namespace

#if !defined(MOD_NOREPEAT)
#define MOD_NOREPEAT 0x4000
#endif

class QHotkeyPrivateWin : public QHotkeyPrivate
{
public:
	QHotkeyPrivateWin();
	// QAbstractNativeEventFilter interface
	bool nativeEventFilter(const QByteArray &eventType, void *message, _NATIVE_EVENT_RESULT *result) override;

protected:
	void pollForHotkeyRelease();
	// QHotkeyPrivate interface
	quint32 nativeKeycode(Qt::Key keycode, bool &ok) Q_DECL_OVERRIDE;
	quint32 nativeModifiers(Qt::KeyboardModifiers modifiers, bool &ok) Q_DECL_OVERRIDE;
    QKeyCombination keyCombinationFromNative(QHotkey::NativeShortcut shortcut) Q_DECL_OVERRIDE; // NEW
	bool registerShortcut(QHotkey::NativeShortcut shortcut) Q_DECL_OVERRIDE;
	bool unregisterShortcut(QHotkey::NativeShortcut shortcut) Q_DECL_OVERRIDE;

private:
	static QString formatWinError(DWORD winError);
	QTimer pollTimer;
	QList<QHotkey::NativeShortcut> polledShortcuts;
};
NATIVE_INSTANCE(QHotkeyPrivateWin)

QHotkeyPrivateWin::QHotkeyPrivateWin(){
	pollTimer.setInterval(50);
	connect(&pollTimer, &QTimer::timeout, this, &QHotkeyPrivateWin::pollForHotkeyRelease);
}

bool QHotkeyPrivate::isPlatformSupported()
{
	return true;
}

bool QHotkeyPrivateWin::nativeEventFilter(const QByteArray &eventType, void *message, _NATIVE_EVENT_RESULT *result) // changed
{
	Q_UNUSED(eventType)
	Q_UNUSED(result)

    MSG* msg = static_cast< MSG* >(message);
    if(msg->message == WM_HOTKEY) {
        QHotkey::NativeShortcut shortcut = { HIWORD(msg->lParam), LOWORD(msg->lParam) };
        if (isEnabled()) {
            this->activateShortcut(shortcut);
            if (this->polledShortcuts.empty())
                this->pollTimer.start();
            this->polledShortcuts.append(shortcut);
        }
        else {
            QKeyCombination combo = shortcut.toKeyCombination();

            if (combo.key() != Qt::Key_unknown) {
                auto* focusedWindow = qApp->focusWindow();

                QString text = QKeySequence(combo).toString();; // Get text representation

                // Post a synthesized key press event to the currently focused widget
                QKeyEvent* pressEvent = new QKeyEvent(QEvent::KeyPress,
                                                      combo.key(),
                                                      combo.keyboardModifiers(),
                                                      text);

                qApp->postEvent(focusedWindow, pressEvent);

                // Also post a release event for completeness
                QKeyEvent* releaseEvent = new QKeyEvent(QEvent::KeyRelease,
                                                        combo.key(),
                                                        combo.keyboardModifiers(),
                                                        text);

                qApp->postEvent(focusedWindow, releaseEvent);
            }
            else {
                // log
            }
        }

    }


	return false;
}

void QHotkeyPrivateWin::pollForHotkeyRelease()
{
	auto it = std::remove_if(this->polledShortcuts.begin(), this->polledShortcuts.end(), [this](const QHotkey::NativeShortcut &shortcut) {
		bool pressed = (GetAsyncKeyState(shortcut.key) & (1 << 15)) != 0;
		if (!pressed)
			this->releaseShortcut(shortcut);
		return !pressed;
	});
	this->polledShortcuts.erase(it, this->polledShortcuts.end());
	if (this->polledShortcuts.empty())
		this->pollTimer.stop();
}

// NEW
quint32 QHotkeyPrivateWin::nativeKeycode(Qt::Key keycode, bool &ok)
{
    // ok = true;
    // if(keycode <= 0xFFFF) {//Try to obtain the key from it's "character"
    // 	const SHORT vKey = VkKeyScanW(static_cast<WCHAR>(keycode));
    // 	if(vKey > -1)
    // 		return LOBYTE(vKey);
    // }

    // //find key from switch/case --> Only finds a very small subset of keys
    // switch (keycode)
    // {
    // case Qt::Key_Escape:
    // 	return VK_ESCAPE;
    // case Qt::Key_Tab:
    // case Qt::Key_Backtab:
    // 	return VK_TAB;
    // case Qt::Key_Backspace:
    // 	return VK_BACK;
    // case Qt::Key_Return:
    // case Qt::Key_Enter:
    // 	return VK_RETURN;
    // case Qt::Key_Insert:
    // 	return VK_INSERT;
    // case Qt::Key_Delete:
    // 	return VK_DELETE;
    // case Qt::Key_Pause:
    // 	return VK_PAUSE;
    // case Qt::Key_Print:
    // 	return VK_PRINT;
    // case Qt::Key_Clear:
    // 	return VK_CLEAR;
    // case Qt::Key_Home:
    // 	return VK_HOME;
    // case Qt::Key_End:
    // 	return VK_END;
    // case Qt::Key_Left:
    // 	return VK_LEFT;
    // case Qt::Key_Up:
    // 	return VK_UP;
    // case Qt::Key_Right:
    // 	return VK_RIGHT;
    // case Qt::Key_Down:
    // 	return VK_DOWN;
    // case Qt::Key_PageUp:
    // 	return VK_PRIOR;
    // case Qt::Key_PageDown:
    // 	return VK_NEXT;
    // case Qt::Key_CapsLock:
    // 	return VK_CAPITAL;
    // case Qt::Key_NumLock:
    // 	return VK_NUMLOCK;
    // case Qt::Key_ScrollLock:
    // 	return VK_SCROLL;

    // case Qt::Key_F1:
    // 	return VK_F1;
    // case Qt::Key_F2:
    // 	return VK_F2;
    // case Qt::Key_F3:
    // 	return VK_F3;
    // case Qt::Key_F4:
    // 	return VK_F4;
    // case Qt::Key_F5:
    // 	return VK_F5;
    // case Qt::Key_F6:
    // 	return VK_F6;
    // case Qt::Key_F7:
    // 	return VK_F7;
    // case Qt::Key_F8:
    // 	return VK_F8;
    // case Qt::Key_F9:
    // 	return VK_F9;
    // case Qt::Key_F10:
    // 	return VK_F10;
    // case Qt::Key_F11:
    // 	return VK_F11;
    // case Qt::Key_F12:
    // 	return VK_F12;
    // case Qt::Key_F13:
    // 	return VK_F13;
    // case Qt::Key_F14:
    // 	return VK_F14;
    // case Qt::Key_F15:
    // 	return VK_F15;
    // case Qt::Key_F16:
    // 	return VK_F16;
    // case Qt::Key_F17:
    // 	return VK_F17;
    // case Qt::Key_F18:
    // 	return VK_F18;
    // case Qt::Key_F19:
    // 	return VK_F19;
    // case Qt::Key_F20:
    // 	return VK_F20;
    // case Qt::Key_F21:
    // 	return VK_F21;
    // case Qt::Key_F22:
    // 	return VK_F22;
    // case Qt::Key_F23:
    // 	return VK_F23;
    // case Qt::Key_F24:
    // 	return VK_F24;

    // case Qt::Key_Menu:
    // 	return VK_APPS;
    // case Qt::Key_Help:
    // 	return VK_HELP;
    // case Qt::Key_MediaNext:
    // 	return VK_MEDIA_NEXT_TRACK;
    // case Qt::Key_MediaPrevious:
    // 	return VK_MEDIA_PREV_TRACK;
    // case Qt::Key_MediaPlay:
    // 	return VK_MEDIA_PLAY_PAUSE;
    // case Qt::Key_MediaStop:
    // 	return VK_MEDIA_STOP;
    // case Qt::Key_VolumeDown:
    // 	return VK_VOLUME_DOWN;
    // case Qt::Key_VolumeUp:
    // 	return VK_VOLUME_UP;
    // case Qt::Key_VolumeMute:
    // 	return VK_VOLUME_MUTE;
    // case Qt::Key_Mode_switch:
    // 	return VK_MODECHANGE;
    // case Qt::Key_Select:
    // 	return VK_SELECT;
    // case Qt::Key_Printer:
    // 	return VK_PRINT;
    // case Qt::Key_Execute:
    // 	return VK_EXECUTE;
    // case Qt::Key_Sleep:
    // 	return VK_SLEEP;
    // case Qt::Key_Period:
    // 	return VK_DECIMAL;
    // case Qt::Key_Play:
    // 	return VK_PLAY;
    // case Qt::Key_Cancel:
    // 	return VK_CANCEL;

    // case Qt::Key_Forward:
    // 	return VK_BROWSER_FORWARD;
    // case Qt::Key_Refresh:
    // 	return VK_BROWSER_REFRESH;
    // case Qt::Key_Stop:
    // 	return VK_BROWSER_STOP;
    // case Qt::Key_Search:
    // 	return VK_BROWSER_SEARCH;
    // case Qt::Key_Favorites:
    // 	return VK_BROWSER_FAVORITES;
    // case Qt::Key_HomePage:
    // 	return VK_BROWSER_HOME;

    // case Qt::Key_LaunchMail:
    // 	return VK_LAUNCH_MAIL;
    // case Qt::Key_LaunchMedia:
    // 	return VK_LAUNCH_MEDIA_SELECT;
    // case Qt::Key_Launch0:
    // 	return VK_LAUNCH_APP1;
    // case Qt::Key_Launch1:
    // 	return VK_LAUNCH_APP2;

    // case Qt::Key_Massyo:
    // 	return VK_OEM_FJ_MASSHOU;
    // case Qt::Key_Touroku:
    // 	return VK_OEM_FJ_TOUROKU;

    // default:
    // 	if(keycode <= 0xFFFF)
    // 		return static_cast<BYTE>(keycode);
    // 	else {
    // 		ok = false;
    // 		return 0;
    // 	}
    // }

    ok = true;

    // First, try the Windows API for simple character keys
    if (keycode <= 0xFFFF) {
        const SHORT vKey = VkKeyScanW(static_cast<WCHAR>(keycode));
        if (vKey > -1) {
            return LOBYTE(vKey);
        }
    }

    // If that fails, search our map of special keys
    const auto it = std::find_if(std::begin(keyMap), std::end(keyMap),
                                 [keycode](const KeyMap& item) {
                                     return item.qtKey == keycode;
                                 });

    if (it != std::end(keyMap)) {
        return it->nativeKey;
    }

    // Fallback for other keys that might map directly
    if (keycode <= 0xFF) {
        return static_cast<quint32>(keycode);
    }

    // If nothing is found, the key is unmappable
    ok = false;
    return 0;
}

quint32 QHotkeyPrivateWin::nativeModifiers(Qt::KeyboardModifiers modifiers, bool &ok)
{
	quint32 nMods = 0;
	if (modifiers & Qt::ShiftModifier)
		nMods |= MOD_SHIFT;
	if (modifiers & Qt::ControlModifier)
		nMods |= MOD_CONTROL;
	if (modifiers & Qt::AltModifier)
		nMods |= MOD_ALT;
	if (modifiers & Qt::MetaModifier)
		nMods |= MOD_WIN;
	ok = true;
	return nMods;
}

// NEW
QKeyCombination QHotkeyPrivateWin::keyCombinationFromNative(QHotkey::NativeShortcut shortcut)
{
    Qt::Key key = qtKeyFromNative(shortcut.key);
    Qt::KeyboardModifiers mods = qtModifiersFromNative(shortcut.modifier);
    return key | mods;
}

bool QHotkeyPrivateWin::registerShortcut(QHotkey::NativeShortcut shortcut)
{
	BOOL ok = RegisterHotKey(NULL,
							 HKEY_ID(shortcut),
                             shortcut.modifier /*+ MOD_NOREPEAT*/, // NEW changed default behaviour
							 shortcut.key);
	if(ok)
		return true;
	else {
		error = QHotkeyPrivateWin::formatWinError(::GetLastError());
		return false;
	}
}

bool QHotkeyPrivateWin::unregisterShortcut(QHotkey::NativeShortcut shortcut)
{
	BOOL ok = UnregisterHotKey(NULL, HKEY_ID(shortcut));
	if(ok)
		return true;
	else {
		error = QHotkeyPrivateWin::formatWinError(::GetLastError());
		return false;
	}
}

QString QHotkeyPrivateWin::formatWinError(DWORD winError)
{
	wchar_t *buffer = NULL;
	DWORD num = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
							   NULL,
							   winError,
							   0,
							   (LPWSTR)&buffer,
							   0,
							   NULL);
	if(buffer) {
		QString res = QString::fromWCharArray(buffer, num);
		LocalFree(buffer);
		return res;
	} else
		return QString();
}
