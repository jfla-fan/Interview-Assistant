#pragma once

#include "common.h"

#include <QObject>


class HotkeyManagerPrivate;


/**
    Allows binding hotkeys for various actions in a thread-safe manner.
*/
class HotkeyManager final : public QObject
{
    Q_OBJECT
public:
    Q_DISABLE_COPY_MOVE(HotkeyManager)

    static HotkeyManager* instance();

    /**
     * @brief allows disabling/enabling hotkeys
     * @details if not enabled, hotkeys get transformed and sent as
     * ordinary key press and key release events to the focused widget.
     */
    void setGlobalEnabled(bool value);

    void override(const HotkeyList& hotkeys);
    bool registerAction(ActionType action, const QKeySequence& shortcut);
    bool registerAction(const ServiceHotkey& hotkey);
    bool removeAction(ActionType action);
    bool removeShortcut(const QKeySequence& shortcut);
    bool actionExists(ActionType action);
    bool shortcutExists(const QKeySequence& shortcut);
    std::optional< ActionType > findActionByShortcut(const QKeySequence& shortcut);
    std::optional< QKeySequence > findShortcutByAction(ActionType action);
    void clearAll();

signals:
    void actionTriggered(ActionType actionType);

private:
    HotkeyManager(QObject* parent = nullptr);
    ~HotkeyManager() override;

    std::unique_ptr< HotkeyManagerPrivate > d_ptr;
    Q_DECLARE_PRIVATE(HotkeyManager)
};

