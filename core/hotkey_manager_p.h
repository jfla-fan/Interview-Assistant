#pragma once

#include "action.h"

#include <QHotkey>
#include <QHash>
#include <QReadWriteLock>


class HotkeyManagerPrivate
{
public:
    struct HotkeyData
    {
        ~HotkeyData() { delete hotkey; }
        QHotkey* hotkey;
    };

    QHash< ActionType, HotkeyData > hotkeyMap;
    mutable QReadWriteLock lock;
};
