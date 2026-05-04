#pragma once

#include "overlay_widget.h"


QT_FORWARD_DECLARE_CLASS(QVBoxLayout)
QT_FORWARD_DECLARE_CLASS(QLabel)


class FileListMenu : public OverlayWidget
{
    Q_OBJECT

public:
    explicit FileListMenu(QWidget* parent = nullptr);

public slots:
    void setItems(const QStringList& items);
    bool deleteItem(const QString& item);
    void addItem(const QString& item);
    void clearItems();

    void showAt(const QPoint& pos);
    qsizetype size() const;

signals:
    void itemSelected(const QString& item);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateEmptyState();
    QLabel* createItemLabel(const QString& item);

    QVBoxLayout* m_layout;
    QLabel*      m_emptyLabel;
    bool         m_isPositioned;
};
