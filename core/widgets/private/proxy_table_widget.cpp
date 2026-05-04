#include "../../common.h"

#include "../../log/log.h"
#include "../../network/proxy_checker.h"

#include "proxy_table_widget.h"

#include <QHeaderView>
#include <QHBoxLayout>
#include <QAbstractItemView>
#include <QStyledItemDelegate>
#include <QDropEvent>
#include <QPainter>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>


class ProxyTableWidget::ProxyDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ProxyDelegate(ProxyTableWidget *parent = nullptr)
        : QStyledItemDelegate(parent)
    { }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyledItemDelegate::paint(painter, option, index);

        bool isEnabled = owner()->isEnabled(index.row());

        painter->save();

        if (!isEnabled) {
            QColor overlayColor(0, 0, 0, 70);
            painter->fillRect(option.rect, overlayColor);
        }

        painter->restore();
    }

    // Override to provide a QLineEdit editor and connect its signals
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        auto *editor = new QLineEdit(parent);

        if (index.column() == 1) {
            connect(editor, &QLineEdit::textChanged,
                    const_cast< ProxyDelegate* >(this), &ProxyDelegate::onEditorTextChanged);
        }
        else if (index.column() == 2) {
            // hostname/IP followed by a colon and a port number.
            QRegularExpression regex(R"(^([a-zA-Z0-9\.\-]+):(\d{1,5})$)");
            auto *validator = new QRegularExpressionValidator(regex, editor);
            editor->setValidator(validator);

            connect(editor, &QLineEdit::textChanged,
                    const_cast< ProxyDelegate* >(this), &ProxyDelegate::onEditorTextChanged);
        }
        else {
            delete editor;
            return QStyledItemDelegate::createEditor(parent, option, index);
        }

        return editor;
    }

    void setEditorData(QWidget *editor, const QModelIndex &index) const override
    {
        QString value = index.model()->data(index, Qt::EditRole).toString();
        auto *lineEdit = static_cast< QLineEdit* >(editor);
        lineEdit->setText(value);
    }

    // This is where we check the final validation state before committing
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override
    {
        auto *lineEdit = static_cast< QLineEdit* >(editor);

        if (lineEdit->property("isValid").toBool() == false) {
            return;
        }

        if (auto* validator = lineEdit->validator()) {
            QString text = lineEdit->text();
            int pos = 0;
            if (validator->validate(text, pos) != QValidator::Acceptable) {
                return;
            }
        }

        model->setData(index, lineEdit->text().trimmed(), Qt::EditRole);
    }

private slots:
    // This slot will be called every time the user types a character
    void onEditorTextChanged(const QString &text)
    {
        auto *editor = qobject_cast< QLineEdit* >(sender());
        if (!editor) return;

        auto *table = qobject_cast< QTableWidget* >(editor->parentWidget()->parentWidget());
        QModelIndex index = table->indexAt(editor->pos());

        bool isValid = true;

        if (index.column() == 1) {
            if (text.trimmed().isEmpty()) {
                isValid = false;
            } else {
                for (int i = 0; i < table->rowCount(); ++i) {
                    if (i == index.row()) continue;
                    if (table->item(i, 1) && table->item(i, 1)->text() == text.trimmed()) {
                        qDebug() << "Invalid editor text at index " << i;
                        qDebug() << "Text itself: " << text;
                        isValid = false;
                        break;
                    }
                }
            }
        }
        else if (index.column() == 2) {
            // The QValidator prevents typing invalid characters, but we need to check
            // if the current string is in a fully "Acceptable" state.
            if (auto* validator = editor->validator()) {
                QString currentText = editor->text();
                int pos = 0;
                if (validator->validate(currentText, pos) != QValidator::Acceptable) {
                    isValid = false;
                }

                // Also check for empty string, which the regex might allow as Intermediate.
                if (currentText.trimmed().isEmpty()) {
                    isValid = false;
                }
            }
        }

        bool wasValid = editor->property("isValid").toBool();
        editor->setProperty("isValid", isValid);

        if (wasValid != isValid) {
            editor->style()->unpolish(editor);
            editor->style()->polish(editor);
        }
    }

    ProxyTableWidget* owner() const { return qobject_cast< ProxyTableWidget* >(parent()); }
};


ProxyTableWidget::ProxyTableWidget(QWidget* parent)
    : QTableWidget(parent)
{
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDefaultDropAction(Qt::MoveAction);
    setDragDropOverwriteMode(false);
    setCornerButtonEnabled(false);

    verticalHeader()->setFixedWidth(30);
    verticalHeader()->setDefaultAlignment(Qt::AlignLeft);
    verticalHeader()->setSectionsMovable(true);
    verticalHeader()->setDragDropMode(QAbstractItemView::InternalMove);

    setColumnCount(6);
    setHorizontalHeaderLabels({ "Status", "Name", "Endpoint", "Ignore SSL", "Success", "Fail" });

    horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

    verticalHeader()->setStyleSheet(R"(
        QHeaderView {
            background-color: rgb(35, 35, 35);
        }
    )");

    auto* button = findChild< QAbstractButton* >(Qt::FindDirectChildrenOnly);

    button->setStyleSheet("background-color: rgb(35, 35, 35);");

    setItemDelegate(new ProxyDelegate(this));
}


QString ProxyTableWidget::name(int row) const
{
    return this->cell< QTableWidgetItem >(row, 1)->text();
}


int ProxyTableWidget::rowByUuid(const QUuid& value) const
{
    for (int row = 0; row < this->rowCount(); ++row) {
        if (uuid(row) == value) return row;
    }

    return -1;
}


int ProxyTableWidget::rowByName(QStringView name) const
{
    for (int row = 0; row < this->rowCount(); ++row) {
        if (this->name(row) == name) return row;
    }

    return -1;
}


void ProxyTableWidget::setDirty(int row, int column, bool value)
{
    switch (column)
    {
        case -1:
        {
            model()->setHeaderData(row, Qt::Vertical, QBrush("#c8a032"), Qt::ForegroundRole);
            break;
        }

        case 0:
        case 3:
        {
            auto* widget = cell< QWidget* >(row, column);
            ::SetWidgetDirty(widget, value);
            break;
        }

        default:
        {
            QString color = value ? "#c8a032" : "#d4d4d4";
            cell< QTableWidgetItem* >(row, column)->setForeground(QColor(color));
            break;
        }
    }
}


void ProxyTableWidget::setDirty(int row, bool value)
{
    for (int col = -1; col < columnCount(); ++col) {
        setDirty(row, col, value);
    }
}


void ProxyTableWidget::insertProxy(int row, int displayRow, int index, const ServiceProxy& proxy)
{
    insertRow(row);

    auto itemId = QUuid::createUuid();
    auto* proxyChecker = new ProxyChecker(proxy, this);

    LOG_TRACE("Insert row {} with uuid {}", row, itemId.toString());

    connect(proxyChecker, &ProxyChecker::statusChanged, [this, itemId](OperationStatus newStatus, const QString& details) {
        static_assert(static_cast< int >(OperationStatus::Max) <= 5, "Add extra case statement");

        int row = this->rowByUuid(itemId);
        if (row >= 0) {
            auto* statusLabel = this->cell< QLabel* >(row, 0);

            switch (newStatus)
            {
                case OperationStatus::Idle:       statusLabel->setText("⚪"); break;
                case OperationStatus::Processing: statusLabel->setText("🔵"); break;
                case OperationStatus::Canceled:   statusLabel->setText("🟡"); break;
                case OperationStatus::Failed:     statusLabel->setText("🔴"); break;
                case OperationStatus::Succeeded:  statusLabel->setText("🟢"); break;
                default: break;
            }
        }
        else {
            LOG_ERROR("Couldn't find row {} with id {}", row, itemId.toString());
        }

        LOG_DEBUG("Proxy status changed: {}, {}", newStatus, details);
    });

    QTableWidgetItem* headerItem = new QTableWidgetItem();

    headerItem->setData(Qt::DisplayRole, QString::number(displayRow));
    headerItem->setData(IsEnabledRole, proxy.Enabled);
    headerItem->setData(ChangesCountRole, 0);
    headerItem->setData(OriginalIndexRole, index);
    headerItem->setData(DisplayIndexRole, displayRow);
    headerItem->setData(UUIdRole, itemId);
    headerItem->setData(ProxyCheckerRole, reinterpret_cast< quintptr >(proxyChecker));
    headerItem->setData(IsDirtyRole, false);
    headerItem->setForeground(QColor("#e0e0e0"));

    setVerticalHeaderItem(row, headerItem);

    auto* statusLabel = new QLabel("⚪");
    statusLabel->setAlignment(Qt::AlignCenter);
    setCellWidget(row, 0, statusLabel);

    setItem(row, 1, new QTableWidgetItem(proxy.Name));
    setItem(row, 2, new QTableWidgetItem(::NetworkProxyToEndpoint(proxy.NetworkProxy)));

    auto* sslCheckBox = new QCheckBox();

    auto* containerWidget = new QWidget();
    auto* layout = new QHBoxLayout(containerWidget);

    layout->setContentsMargins(0, 0, 0, 0);

    layout->addStretch();
    layout->addWidget(sslCheckBox);
    layout->addStretch();

    containerWidget->setLayout(layout);

    sslCheckBox->setChecked(proxy.IgnoreSslErrors);
    connect(sslCheckBox, &QCheckBox::toggled, sslCheckBox, [this, containerWidget] () {
        emit cellChanged(rowAt(containerWidget->y()), 3);
    }, Qt::QueuedConnection); // wait for table geometry recalculation
    setCellWidget(row, 3, containerWidget);

    QTableWidgetItem* successItem = new QTableWidgetItem("0");
    successItem->setFlags(successItem->flags() & ~Qt::ItemIsEditable);
    successItem->setTextAlignment(Qt::AlignCenter);
    setItem(row, 4, successItem);

    QTableWidgetItem* failItem = new QTableWidgetItem("0");
    failItem->setFlags(failItem->flags() & ~Qt::ItemIsEditable);
    failItem->setTextAlignment(Qt::AlignCenter);
    setItem(row, 5, failItem);
}


void ProxyTableWidget::removeProxy(int row)
{
    if (row < 0 || row >= rowCount()) return;

    auto* headerItem = verticalHeaderItem(row);
    if (headerItem) {
        quintptr ptr = headerItem->data(ProxyCheckerRole).value< quintptr >();
        ProxyChecker* checker = reinterpret_cast< ProxyChecker* >(ptr);

        if (checker) {
            checker->cancelCheck();
            checker->deleteLater();
        }
    }

    QTableWidget::removeRow(row);
}


void ProxyTableWidget::resetProxy(int row, const ServiceProxy& proxy)
{
    // setEnabled(row, proxy.Enabled);
    // proxyChecker(row)->setProxy(proxy);
    // setChangesCount(row, 0);
    // setDirty(row, false);

    cell< QTableWidgetItem >(row, 1)->setText(proxy.Name);
    cell< QTableWidgetItem >(row, 2)->setText(::NetworkProxyToEndpoint(proxy.NetworkProxy));
    cell< QCheckBox >(row, 3)->setChecked(proxy.IgnoreSslErrors);
    cell< QTableWidgetItem >(row, 4)->setText(QStringLiteral("0"));
    cell< QTableWidgetItem >(row, 5)->setText(QStringLiteral("0"));
}


void ProxyTableWidget::setChangesCount(int row, int value)
{
    verticalHeaderItem(row)->setData(ChangesCountRole, value);
    verticalHeaderItem(row)->setData(Qt::DisplayRole, QString::number(displayNumber(row)) + (changesCount(row) ? "*" : ""));
    LOG_DEBUG() << "set Changes count: " << value;
}


int ProxyTableWidget::successItemCount(int row) const
{
    return this->cell< QTableWidgetItem >(row, 4)->text().toInt();
}


void ProxyTableWidget::setSuccessItemCount(int row, int value)
{
    this->cell< QTableWidgetItem >(row, 4)->setText(QString::number(value));
}


void ProxyTableWidget::incSuccessItemCount(int row, int delta)
{
    setSuccessItemCount(row, successItemCount(row) + delta);
}


int ProxyTableWidget::failedItemCount(int row) const
{
    return this->cell< QTableWidgetItem >(row, 5)->text().toInt();
}


void ProxyTableWidget::setFailedItemCount(int row, int value)
{
    this->cell< QTableWidgetItem >(row, 5)->setText(QString::number(value));
}


void ProxyTableWidget::incFailedItemCount(int row, int delta)
{
    setFailedItemCount(row, failedItemCount(row) + delta);
}


void ProxyTableWidget::dragMoveEvent(QDragMoveEvent* event)
{
    const QPoint pos = event->position().toPoint();
    const QModelIndex targetIndex = indexAt(pos);

    // Disallow dropping on the viewport (empty area)
    if (!targetIndex.isValid()) {
        event->ignore();
        return;
    }

    int index = -1;
    const QRect targetRect = visualRect(targetIndex);
    const auto indicator = dropIndicatorPosition(pos, targetRect, index);
    const QPoint modifiedPos = modifiedPosition(event, indicator, targetRect);

    QDragMoveEvent modifiedEvent(modifiedPos, event->proposedAction(),
                                 event->mimeData(), event->buttons(),
                                 event->modifiers(), event->type());

    QTableWidget::dragMoveEvent(&modifiedEvent);

    if (modifiedEvent.isAccepted()) {
        event->accept();
    } else {
        event->ignore();
    }
}


void ProxyTableWidget::dropEvent(QDropEvent* event)
{
    const QPoint pos = event->position().toPoint();
    const QModelIndex targetIndex = indexAt(pos);

    if (!targetIndex.isValid()) {
        event->ignore();
        return;
    }

    const int selectedRow = selectedIndexes().front().row();
    const int dropRow = targetIndex.row();

    const QRect targetRect = visualRect(targetIndex);
    int index = -1;
    const auto indicator = dropIndicatorPosition(pos, targetRect, index);
    const QPoint modifiedPos = modifiedPosition(event, indicator, targetRect);

    if (!(dropRow == selectedRow - 1 && indicator == QAbstractItemView::BelowItem) &&
        !(dropRow == selectedRow + 1 && indicator == QAbstractItemView::AboveItem)) {
        emit cellMoved(selectedRow, dropRow + (indicator == QAbstractItemView::BelowItem));
    }

    QDropEvent modifiedEvent(modifiedPos, event->proposedAction(),
                             event->mimeData(), event->buttons(),
                             event->modifiers());

    QTableWidget::dropEvent(&modifiedEvent);

    if (modifiedEvent.isAccepted()) {
        event->accept();
    }
}


void ProxyTableWidget::mousePressEvent(QMouseEvent *event)
{
    if (!itemAt(event->pos())) {
        clearSelection();
    }

    QTableWidget::mousePressEvent(event);
}


ProxyTableWidget::DropIndicatorPosition ProxyTableWidget::dropIndicatorPosition(const QPoint& pos, const QRect& rect, int& index) const
{
    index = rowAt(pos.y());

    // If dragging to the empty area below the last item, always insert after.
    if (index == -1) {
        index = rowCount();
        return QAbstractItemView::BelowItem;
    }

    if (pos.y() - rect.top() < rect.height() / 2) {
        return QAbstractItemView::AboveItem;
    } else {
        return QAbstractItemView::BelowItem;
    }
}


QPoint ProxyTableWidget::modifiedPosition(const QDropEvent* event, DropIndicatorPosition indicator, const QRect& rect) const
{
    QPoint pos = event->position().toPoint();
    if (indicator == QAbstractItemView::AboveItem) {
        pos.setY(rect.top());
    } else {
        pos.setY(rect.bottom());
    }
    return pos;
}


#include "proxy_table_widget.moc"
