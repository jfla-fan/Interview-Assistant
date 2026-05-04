#pragma once

#include <QTableWidget>
#include <QUuid>


class ProxyChecker;
class QCheckBox;
struct ServiceProxy;


class ProxyTableWidget : public QTableWidget
{
    Q_OBJECT
public:
    using DropIndicatorPosition = QAbstractItemView::DropIndicatorPosition;

    template< class T >
    using PointerType = std::remove_pointer_t< T >*;

    template< class T >
    using PointerCType = std::add_const_t< std::remove_pointer_t< T > >*;

    enum ProxyTableItemRole
    {
        IsEnabledRole = Qt::UserRole + 1,
        IsDirtyRole,
        OriginalIndexRole,
        DisplayIndexRole,
        UUIdRole,
        ProxyCheckerRole,
        ChangesCountRole
    };

public:
    explicit ProxyTableWidget(QWidget* parent);

    template< class T >
    PointerType< T > cell(int row, int column)
    {
        using ReturnType = PointerType< T >;

        if constexpr (std::is_same_v< ReturnType, QTableWidgetItem* >)
            return this->item(row, column);
        else {
            switch (column)
            {
                case  3: return qobject_cast< ReturnType >(cellWidget(row, column)->findChild< QCheckBox* >());
                default: return qobject_cast< ReturnType >(cellWidget(row, column));
            }
        }
    }

    template< class T >
    PointerCType< T > cell(int row, int column) const
    {
        return const_cast< ProxyTableWidget* >(this)->cell< T >(row, column);
    }

    QUuid uuid(int row) const { return this->verticalHeaderItem(row)->data(UUIdRole).toUuid(); }
    void setUuid(int row, QUuid value) { this->verticalHeaderItem(row)->setData(UUIdRole, value); }
    int  displayNumber(int row) const { return this->verticalHeaderItem(row)->data(DisplayIndexRole).toInt(); }
    int  originalIndex(int row) const { return this->verticalHeaderItem(row)->data(OriginalIndexRole).toInt(); }
    void setOriginalIndex(int row, int value) { this->verticalHeaderItem(row)->setData(OriginalIndexRole, value); }
    int  changesCount(int row) const { return this->verticalHeaderItem(row)->data(ChangesCountRole).toInt(); }
    void incChangesCount(int row, int delta) { this->setChangesCount(row, changesCount(row) + delta); }
    bool isEnabled(int row) const { return this->verticalHeaderItem(row)->data(IsEnabledRole).toBool(); }
    void setEnabled(int row, bool value) { this->verticalHeaderItem(row)->setData(IsEnabledRole, value); }
    bool isDirty(int row, int column) const { return column < 0 ? verticalHeaderItem(row)->data(IsDirtyRole).toBool()
                                                                : model()->index(row, column).data(IsDirtyRole).toBool(); }

    void setProxyChecker(int row, ProxyChecker* value) { this->verticalHeaderItem(row)->setData(ProxyCheckerRole, reinterpret_cast< quintptr >(value)); }
    ProxyChecker* proxyChecker(int row) const { return reinterpret_cast< ProxyChecker* >(this->verticalHeaderItem(row)->data(ProxyCheckerRole).value< quintptr >()); }
    QString name(int row) const;
    int  rowByUuid(const QUuid& value) const;
    int  rowByName(QStringView name) const;
    void setDirty(int row, int column, bool value);
    void setDirty(int row, bool value);
    void insertProxy(int row, int displayRow, int index, const ServiceProxy& proxy);
    void removeProxy(int row);
    void resetProxy(int row, const ServiceProxy& proxy);
    void setChangesCount(int row, int value);
    int  successItemCount(int row) const;
    void setSuccessItemCount(int row, int value);
    void incSuccessItemCount(int row, int delta);
    int  failedItemCount(int row) const;
    void setFailedItemCount(int row, int value);
    void incFailedItemCount(int row, int delta);

signals:
    void cellMoved(int prevRow, int currRow);

protected:
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    class ProxyDelegate;

    DropIndicatorPosition dropIndicatorPosition(const QPoint& pos, const QRect& rect, int& index) const;
    QPoint modifiedPosition(const QDropEvent* event, DropIndicatorPosition indicator, const QRect& rect) const;
};
