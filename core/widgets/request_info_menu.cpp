#include "request_info_menu.h"
#include "interactive_label.h"

#include <QMouseEvent>
#include <QScreen>
#include <QGuiApplication>
#include <QVBoxLayout>
#include <QLabel>


RequestInfoMenu::RequestInfoMenu(QWidget* parent, bool excludeFromCapture)
    : OverlayWidget("RequestInfoMenu", parent, excludeFromCapture)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(2);
    m_layout->setContentsMargins(5, 5, 5, 5);

    setStyleSheet("background-color: rgba(40, 40, 40, 200); border-radius: 6px;");
}


void RequestInfoMenu::setItems(const QList< ServiceRequestLookUpData >& items) {
    qDeleteAll(m_labels);
    m_labels.clear();
    m_dataModel = items;

    for (int i = 0; i < items.size(); ++i) {
        const ServiceRequestLookUpData& item = items[i];
        InteractiveLabel* label = new InteractiveLabel(item.toString());
        label->setStyleSheet("color: white; padding: 4px 8px; border-radius: 4px;");
        label->setCursor(Qt::PointingHandCursor);
        m_layout->addWidget(label);
        m_labels.append(label);

        connect(label, &InteractiveLabel::clicked, [this, item]() {
            emit itemSelected(item);
            hide();
        });
    }

    adjustSize();
}


void RequestInfoMenu::showAt(const QPoint& pos)
{
    move(pos);
    show();
}
