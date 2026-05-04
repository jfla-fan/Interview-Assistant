#include "hotkey_table_widget.h"

#include "../../hotkey_manager.h"

#include <magic_enum/magic_enum_utility.hpp>

#include <QHeaderView>
#include <QKeyEvent>
#include <QStyledItemDelegate>
#include <QKeySequenceEdit>
#include <QLineEdit>
#include <QPainter>
#include <QDebug>


class HotkeyTableWidget::HotkeyEdit : public QKeySequenceEdit
{
    Q_OBJECT

public:
    explicit HotkeyEdit(const QKeySequence& initialSequence, QWidget* parent = nullptr)
        : QKeySequenceEdit(initialSequence, parent)
        , m_originalSequence(initialSequence)
    {
        setMaximumSequenceLength(1);
        setObjectName(initialSequence.toString());

        setDirty(false);
        setValid(true);
    }

    bool isValid() const { return property("isValid").toBool(); }
    void setValid(bool value)
    {
        ::SetWidgetValid(this, value, false);
    }

    bool isDirty() const { return property("isDirty").toBool(); }
    void setDirty(bool value)
    {
        setProperty("isDirty", value);
        ::SetWidgetDirty(this, value, false);
    }

    const QKeySequence& originalKey() const { return m_originalSequence; }
    void refreshLineStyle() { ::RefreshStyle(findChild< QLineEdit* >()); }

protected:
    void focusInEvent(QFocusEvent* event) override
    {
        HotkeyManager::instance()->setGlobalEnabled(false);
        QKeySequenceEdit::focusInEvent(event);
    }

    void focusOutEvent(QFocusEvent* event) override
    {
        HotkeyManager::instance()->setGlobalEnabled(true);
        QKeySequenceEdit::focusOutEvent(event);
    }

    void keyPressEvent(QKeyEvent* event) override
    {
        if (event->key() == Qt::Key_Escape) {
            setKeySequence(m_originalSequence);
            clearFocus();
            event->accept();
            return;
        }

        QKeySequenceEdit::keyPressEvent(event);
    }

    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() == QEvent::ContextMenu) {
            auto* menuEvent = static_cast< QContextMenuEvent* >(event);
            emit customContextMenuRequested(mapToParent(menuEvent->pos()));
            return true;
        }

        return QKeySequenceEdit::eventFilter(watched, event);
    }

private:
    QKeySequence m_originalSequence;
};


class HotkeyTableWidget::HotkeyDelegate : public QStyledItemDelegate
{
public:
    HotkeyDelegate(HotkeyTableWidget* owner)
        : QStyledItemDelegate(owner)
    { }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        QStyleOptionViewItem newOption = option;

        bool isEnabled = owner()->isEnabled(index.row());
        bool isDirty = owner()->isDirty(index.row());

        if (isDirty) {
            newOption.palette.setColor(QPalette::Text, QColor("#c8a032"));
        }

        QStyledItemDelegate::paint(painter, newOption, index);

        if (!isEnabled) {
            painter->save();
            painter->fillRect(option.rect, QColor(0, 0, 0, 70));
            painter->restore();
        }
    }

    HotkeyTableWidget* owner() const { return qobject_cast< HotkeyTableWidget* >(parent()); }
};


HotkeyTableWidget::HotkeyTableWidget(QWidget* parent)
    : QTableWidget(parent)
{
    setObjectName("HotkeyTableWidget");

    setColumnCount(2);
    setHorizontalHeaderLabels({ "Action", "Hotkey" });
    verticalHeader()->setVisible(false);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);

    horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    setStyleSheet(R"(
        * {
            background: transparent;
        }

        QKeySequenceEdit[isValid="false"] QLineEdit {
            border: 1px solid #BE5050;
        }

        QKeySequenceEdit[isDirty="true"] QLineEdit {
            color: #c8a032;
        }
    )");

    setItemDelegate(new HotkeyDelegate(this));
}


void HotkeyTableWidget::setHotkeys(const HotkeyList& hotkeys)
{
    m_hotkeys.clear();

    magic_enum::enum_for_each< ActionType >([this](ActionType action) {
        if (action < ActionType::Test1)
            m_hotkeys[action] = ServiceHotkey { .Action = action,
                                                .Shortcut = {},
                                                .Enabled = false };
    });

    for (const auto& hotkey : hotkeys)
        m_hotkeys[hotkey.Action] = hotkey;

    setRowCount(0);
    for (const auto& [action, hotkey] : m_hotkeys) {
        int row = rowCount();
        insertRow(row);
        populateRow(row, m_hotkeys[action]);
    }

    validateAllRows();
}


bool HotkeyTableWidget::isEnabled(int row) const
{
    return model()->index(row, 0).data(IsEnabledRole).toBool();
}


void HotkeyTableWidget::setEnabled(int row, bool value)
{
    const bool wasEnabled = model()->index(row, 0).data(IsEnabledRole).toBool();

    model()->setData(model()->index(row, 0), value, IsEnabledRole);
    model()->setData(model()->index(row, 1), value, IsEnabledRole);

    if (wasEnabled != value)
        emit hotkeyEnabledChanged(row, value);
}


bool HotkeyTableWidget::isDirty(int row) const
{
    return model()->index(row, 0).data(IsDirtyRole).toBool();
}


void HotkeyTableWidget::setDirty(int row, bool value)
{
    if (isDirty(row) != value) {
        model()->setData(model()->index(row, 0), value, IsDirtyRole);
        model()->setData(model()->index(row, 1), value, IsDirtyRole);

        auto* hotkey = getHotkey(row);
        hotkey->setDirty(value);
        hotkey->refreshLineStyle();

        emit hotkeyDirtyChanged(row, value);
    }
}


bool HotkeyTableWidget::isValid(int row) const
{
    return model()->index(row, 0).data(IsValidRole).toBool();
}


void HotkeyTableWidget::setValid(int row, bool value)
{
    if (isValid(row) != value) {
        model()->setData(model()->index(row, 0), value, IsValidRole);
        model()->setData(model()->index(row, 1), value, IsValidRole);

        auto* hotkey = getHotkey(row);
        hotkey->setValid(value);
        hotkey->refreshLineStyle();

        emit hotkeyValidChanged(row, value);
    }
}


int HotkeyTableWidget::changesCount(int row) const
{
    return model()->index(row, 0).data(ChangesCountRole).toInt();
}


void HotkeyTableWidget::setChangesCount(int row, int value)
{
    model()->setData(model()->index(row, 0), value, ChangesCountRole);
    model()->setData(model()->index(row, 0), ::ToQString(actionType(row)) + (value ? "*" : ""));
}


void HotkeyTableWidget::incChangesCount(int row, int delta)
{
    setChangesCount(row, changesCount(row) + delta);
}


ActionType HotkeyTableWidget::actionType(int row) const
{
    return model()->index(row, 0).data(ActionTypeRole).value< ActionType >();
}


QKeySequence HotkeyTableWidget::originalSequence(int row) const
{
    return model()->index(row, 0).data(OriginalSequenceRole).value< QKeySequence >();
}


bool HotkeyTableWidget::originalEnabled(int row) const
{
    return m_hotkeys.at(this->actionType(row)).Enabled;
}


QKeySequence HotkeyTableWidget::currentSequence(int row) const
{
    return getHotkey(row)->keySequence();
}


void HotkeyTableWidget::setCurrentSequence(int row, const QKeySequence& sequence)
{
    if (currentSequence(row) != sequence) {
        getHotkey(row)->setKeySequence(sequence);
        emit hotkeyShortcutChanged(row, sequence);
    }
}


void HotkeyTableWidget::resetKey(int row)
{
    setCurrentSequence(row, getHotkey(row)->originalKey());
}


HotkeyList HotkeyTableWidget::collectHotkeys() const
{
    HotkeyList result;
    result.reserve(static_cast< int >(ActionType::Max));

    for (int row = 0; row < rowCount(); ++row) {
        auto shortcut = currentSequence(row);

        if (!shortcut.isEmpty()) {
            result.append(ServiceHotkey {
                .Action = actionType(row),
                .Shortcut = std::move(shortcut),
                .Enabled = isEnabled(row)
            });
        }
    }

    return result;
}


void HotkeyTableWidget::resetRow(int row)
{
    const auto actionType = this->actionType(row);
    const auto& hotkey = m_hotkeys[actionType];

    setEnabled(row, hotkey.Enabled);
    resetKey(row);
    setChangesCount(row, 0);
}


void HotkeyTableWidget::resetAll()
{
    for (int i = 0; i < rowCount(); ++i) {
        resetRow(i);
    }
}


void HotkeyTableWidget::onKeySequenceChanged(const QKeySequence& sequence)
{
    Q_UNUSED(sequence);
    validateAllRows();
}


void HotkeyTableWidget::validateAllRows()
{
    QHash< QKeySequence, int > sequenceCounts;

    // @todo change this

    // Pass 1: Count occurrences of each sequence
    for (int i = 0; i < rowCount(); ++i) {
        if(auto* keyEdit = qobject_cast< HotkeyEdit* >(cellWidget(i, 1))) {
            QKeySequence seq = keyEdit->keySequence();
            if (!seq.isEmpty()) {
                sequenceCounts[seq]++;
            }
        }
    }

    // Pass 2: Update UI based on state
    for (int row = 0; row < rowCount(); ++row) {
        auto* actionItem = item(row, 0);
        auto* keyEdit = qobject_cast< HotkeyEdit* >(cellWidget(row, 1));

        QKeySequence currentSeq = keyEdit->keySequence();
        QKeySequence originalSeq = actionItem->data(OriginalSequenceRole).value< QKeySequence >();

        // Update dirty state
        bool isDirty = (currentSeq != originalSeq);
        setDirty(row, isDirty);

        bool isValid = true;
        if (!currentSeq.isEmpty() && sequenceCounts.value(currentSeq, 0) > 1) {
            isValid = false;
        }

        setValid(row, isValid);
    }
}


void HotkeyTableWidget::populateRow(int row, const ServiceHotkey& hotkey)
{
    auto* actionItem = new QTableWidgetItem(ToQString(hotkey.Action));

    actionItem->setFlags(actionItem->flags() & ~Qt::ItemIsEditable);

    actionItem->setData(ActionTypeRole, QVariant::fromValue(hotkey.Action));
    actionItem->setData(OriginalSequenceRole, QVariant::fromValue(hotkey.Shortcut));
    actionItem->setData(IsEnabledRole, hotkey.Enabled);
    actionItem->setData(IsDirtyRole, false);
    actionItem->setData(IsValidRole, true);
    actionItem->setData(ChangesCountRole, 0);

    setItem(row, 0, actionItem);

    auto* keyEdit = new HotkeyEdit(hotkey.Shortcut, this);
    keyEdit->setMaximumSequenceLength(1);
    keyEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    setCellWidget(row, 1, keyEdit);

    connect(keyEdit, &QKeySequenceEdit::keySequenceChanged, this, &HotkeyTableWidget::onKeySequenceChanged);
    connect(keyEdit, &QKeySequenceEdit::customContextMenuRequested, this, &HotkeyTableWidget::customContextMenuRequested);
}


HotkeyTableWidget::HotkeyEdit* HotkeyTableWidget::getHotkey(int row) const
{
    return qobject_cast< HotkeyEdit* >(cellWidget(row, 1));
}


#include "hotkey_table_widget.moc"
