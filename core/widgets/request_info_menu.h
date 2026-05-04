#pragma once

#include "../common.h"

#include "overlay_widget.h"

#include <QList>

QT_FORWARD_DECLARE_CLASS(QVBoxLayout)
QT_FORWARD_DECLARE_CLASS(InteractiveLabel)


class RequestInfoMenu : public OverlayWidget {
    Q_OBJECT

public:
    explicit RequestInfoMenu(QWidget* parent = nullptr, bool excludeFromCapture = false);

public slots:
    void setItems(const QList< ServiceRequestLookUpData >& items);
    void showAt(const QPoint& pos);

signals:
    void itemSelected(const ServiceRequestLookUpData& item);

private:
    QVBoxLayout* m_layout;
    QList< InteractiveLabel* > m_labels;
    QList< ServiceRequestLookUpData > m_dataModel;
};
