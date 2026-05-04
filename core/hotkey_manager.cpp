#include "hotkey_manager.h"

#include "hotkey_manager_p.h"

#include "log/log.h"

#include <algorithm>

#include <QReadLocker>
#include <QWriteLocker>


HotkeyManager* HotkeyManager::instance()
{
    static HotkeyManager manager;
    return &manager;
}


HotkeyManager::HotkeyManager(QObject *parent)
    : QObject(parent)
    , d_ptr(new HotkeyManagerPrivate)
{ }


HotkeyManager::~HotkeyManager()
{ }


void HotkeyManager::setGlobalEnabled(bool value)
{
    QHotkey::setEnabled(value);
}


void HotkeyManager::override(const HotkeyList& hotkeys)
{
    // simple strategy for now, could change to more graceful way later
    clearAll();
    for (const auto& hotkey : hotkeys)
    {
        if (!hotkey.Enabled) continue;

        if (!registerAction(hotkey)) {
            qFatal() << "Failed to register action after clearing all up. This should not happen.";
        }
    }
}


bool HotkeyManager::registerAction(ActionType action, const QKeySequence& shortcut)
{
    Q_D(HotkeyManager);

    if (shortcut.isEmpty())
    {
        qWarning() << "Attempting to register empty key sequence ";
        return false;
    }

    QWriteLocker locker { &d->lock };

    auto actionIt = d->hotkeyMap.constFind(action);
    auto shortcutIt = std::find_if(d->hotkeyMap.cbegin(), d->hotkeyMap.cend(), [&shortcut](const auto& hotkeyData) { return hotkeyData.hotkey->shortcut() == shortcut; });

    if (actionIt == shortcutIt) {
        if (actionIt == d->hotkeyMap.cend()) {
            auto it = d->hotkeyMap.emplace(action, new QHotkey(shortcut, true));
            connect(it->hotkey, &QHotkey::activated, this, [action, this]() { emit actionTriggered(action); });
        }

        return true;
    }

    LOG_WARNING("Failed to register hotkey({}, {})", ToQString(action), shortcut.toString());
    return false;
}


bool HotkeyManager::registerAction(const ServiceHotkey& hotkey)
{
    return registerAction(hotkey.Action, hotkey.Shortcut);
}


bool HotkeyManager::removeAction(ActionType action)
{
    Q_D(HotkeyManager);
    QWriteLocker locker { &d->lock };
    return d->hotkeyMap.remove(action);
}


bool HotkeyManager::removeShortcut(const QKeySequence& shortcut)
{
    Q_D(HotkeyManager);
    QWriteLocker locker { &d->lock };
    return d->hotkeyMap.removeIf([&shortcut](const auto& item) { return item->hotkey->shortcut() == shortcut; });
}


bool HotkeyManager::actionExists(ActionType action)
{
    Q_D(HotkeyManager);
    QReadLocker locker { &d->lock };
    return d->hotkeyMap.contains(action);
}


bool HotkeyManager::shortcutExists(const QKeySequence& shortcut)
{
    Q_D(HotkeyManager);
    QReadLocker locker { &d->lock };
    return std::find_if(d->hotkeyMap.cbegin(), d->hotkeyMap.cend(), [&shortcut](const auto& hotkeyData) { return hotkeyData.hotkey->shortcut() == shortcut; }) != d->hotkeyMap.cend();
}


std::optional< ActionType > HotkeyManager::findActionByShortcut(const QKeySequence& shortcut)
{
    Q_D(HotkeyManager);
    QReadLocker locker { &d->lock };
    auto it = std::find_if(d->hotkeyMap.cbegin(), d->hotkeyMap.cend(), [&shortcut](const auto& hotkeyData) { return hotkeyData.hotkey->shortcut() == shortcut; });
    return it != d->hotkeyMap.end() ? std::make_optional(it.key()) : std::nullopt;
}


std::optional< QKeySequence > HotkeyManager::findShortcutByAction(ActionType action)
{
    Q_D(HotkeyManager);
    QReadLocker locker { &d->lock };
    auto it = d->hotkeyMap.find(action);
    return it != d->hotkeyMap.end() ? std::make_optional(it->hotkey->shortcut()) : std::nullopt;
}


void HotkeyManager::clearAll()
{
    Q_D(HotkeyManager);
    QWriteLocker locker { &d->lock };
    d->hotkeyMap.clear();
}


// bool HotkeyManager::registerAction(ActionType action, const QKeySequence& shortcut)
// {
//     if (shortcut.isEmpty())
//     {
//         qWarning() << "Attempting to register empty key sequence ";
//         return false;
//     }

//     Q_D(HotkeyManager);
//     QWriteLocker locker (&d->lock);

//     auto dataIt = d->hotkeyMap.find(action);
//     if (dataIt == d->hotkeyMap.end() || dataIt->hotkeys.isEmpty())
//     {
//         d->hotkeyMap[action].hotkeys.push_back(new QHotkey(shortcut, true));
//         dataIt = d->hotkeyMap.find(action);
//     } else if (std::ranges::find_if(dataIt->hotkeys, [shortcut](QHotkey* item) { return item->shortcut() == shortcut; }) != dataIt->hotkeys.end())
//     {
//         qWarning() << "Can't register action as it already contains " << shortcut.toString();
//         return false;
//     }

//     if (!dataIt->hotkeys.back()->isRegistered())
//     {
//         qFatal() << "Failed to register hotkey " << shortcut;
//         dataIt->hotkeys.removeLast();
//         return false;
//     }

//     connect(dataIt->hotkeys.back(), &QHotkey::activated, this, [action, this]() { emit actionTriggered(action); });

//     return true;
// }


// bool HotkeyManager::rebindAction(ActionType action, const QKeySequence& shortcut)
// {
//     if (shortcut.isEmpty())
//     {
//         qWarning() << "Attempting to rebind empty key sequence ";
//         return false;
//     }

//     if (!clearAction(action))
//     {
//         qWarning() << "HotkeyManager::rebindAction - failed to clear action. Register it first.";
//         return false;
//     }

//     if (!registerAction(action, shortcut))
//     {
//         qWarning() << "HotkeyManager::rebindAction - failed to register action.";
//         return false;
//     }

//     return true;
// }


// bool HotkeyManager::removeShortcut(ActionType action, const QKeySequence& shortcut)
// {
//     Q_D(HotkeyManager);
//     QWriteLocker locker(&d->lock);
//     auto dataIt = d->hotkeyMap.find(action);
//     return dataIt != d->hotkeyMap.end() && dataIt->hotkeys.removeIf([shortcut](QHotkey* item) { return item->shortcut() == shortcut; });
// }


// bool HotkeyManager::clearAction(ActionType action)
// {
//     Q_D(HotkeyManager);

//     QWriteLocker locker(&d->lock);
//     auto actionIt = d->hotkeyMap.find(action);

//     if (actionIt == d->hotkeyMap.end())
//     {
//         // need to change that to proper enum names using Qt Meta System
//         qWarning() << QString("HotkeyManager::clearAction - faild to find action %1").arg((int)action);
//         return false;
//     }

//     actionIt->UnregisterAndDestroy();

//     return true;
// }


// bool HotkeyManager::actionExists(ActionType action) const
// {
//     const Q_D(HotkeyManager);
//     QReadLocker locker(&d->lock);
//     return d->hotkeyMap.contains(action);
// }


// QList< QKeySequence > HotkeyManager::findBinding(ActionType action) const
// {
//     const Q_D(HotkeyManager);
//     QList< QKeySequence > result;

//     QReadLocker locker(&d->lock);
//     for (const auto key : d->hotkeyMap[action].hotkeys)
//     {
//         result.push_back(key->shortcut());
//     }

//     return result;
// }


// void HotkeyManager::clearAll()
// {
//     Q_D(HotkeyManager);
//     QWriteLocker locker(&d->lock);

//     for (auto& data : d->hotkeyMap)
//     {
//         data.UnregisterAndDestroy();
//     }

//     d->hotkeyMap.clear();
// }


/////////////////////////////////////////////////////////////////////////////////////////////////////////
//////// HotkeyManagerPrivate::HotkeyData

// void HotkeyManagerPrivate::HotkeyData::UnregisterAndDestroy()
// {
//     qDeleteAll(hotkeys);
//     hotkeys.clear();
// }

/////////////////////////////////////////////////////////////////////////////////////////////////////////
