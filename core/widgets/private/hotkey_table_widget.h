#pragma once

#include <QTableWidget>

#include <tsl/ordered_map.h>

#include "../../common.h"


class HotkeyTableWidget : public QTableWidget
{
    Q_OBJECT
public:
    using HotkeyMap = tsl::ordered_map< ActionType, ServiceHotkey >;

    enum ItemRole
    {
        ActionTypeRole = Qt::UserRole + 1,
        OriginalSequenceRole,
        IsEnabledRole,
        IsDirtyRole,
        IsValidRole,
        ChangesCountRole
    };

public:
    explicit HotkeyTableWidget(QWidget* parent = nullptr);

    void setHotkeys(const HotkeyList& hotkeys);
    const HotkeyMap& hotkeyMap() const { return m_hotkeys; }

    bool isEnabled(int row) const;
    void setEnabled(int row, bool value);

    bool isDirty(int row) const;
    void setDirty(int row, bool value);

    bool isValid(int row) const;
    void setValid(int row, bool value);

    int changesCount(int row) const;
    void setChangesCount(int row, int value);
    void incChangesCount(int row, int delta);

    ActionType actionType(int row) const;
    QKeySequence originalSequence(int row) const;
    bool originalEnabled(int row) const;

    QKeySequence currentSequence(int row) const;
    void setCurrentSequence(int row, const QKeySequence& sequence);

    void resetKey(int row);

    ::HotkeyList collectHotkeys() const;

public slots:
    void resetRow(int row);
    void resetAll();

private slots:
    void onKeySequenceChanged(const QKeySequence& sequence);
    void validateAllRows();

signals:
    void hotkeyDirtyChanged(int row, bool isDirty);
    void hotkeyEnabledChanged(int row, bool isEnabled);
    void hotkeyShortcutChanged(int row, const QKeySequence& sequence);
    void hotkeyValidChanged(int row, bool isValid);

private:
    class HotkeyDelegate;
    class HotkeyEdit;

    void populateRow(int row, const ServiceHotkey& hotkey);
    HotkeyEdit* getHotkey(int row) const;

    HotkeyMap m_hotkeys; // probably get rid of this, can just keep all the data in UI properties
};
