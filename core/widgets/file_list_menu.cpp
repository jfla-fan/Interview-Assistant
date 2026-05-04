#include "file_list_menu.h"
#include "interactive_label.h"

#include <QResizeEvent>
#include <QVBoxLayout>
#include <QLabel>


FileListMenu::FileListMenu(QWidget* parent)
    : OverlayWidget("FileListMenu", parent)
    , m_isPositioned(false)
{
    m_layout = new QVBoxLayout(this);

    m_layout->setSpacing(2);
    m_layout->setContentsMargins(5, 5, 5, 5);

    setStyleSheet("background-color: rgba(40, 40, 40, 200); border-radius: 6px;");

    m_emptyLabel = new QLabel("No files uploaded.");
    m_emptyLabel->setStyleSheet("color: gray; padding: 4px; font-style: italic;");
    m_layout->addWidget(m_emptyLabel);
}


void FileListMenu::setItems(const QStringList& items)
{
    clearItems();
    for (const QString& item : items) {
        addItem(item);
    }
}


void FileListMenu::showAt(const QPoint& pos)
{
    move(pos);
    show();
    raise();
}


qsizetype FileListMenu::size() const
{
    return m_layout->count();
}


void FileListMenu::addItem(const QString& item)
{
    // m_layout->insertWidget(0, createItemLabel(item));
    m_layout->addWidget(createItemLabel(item));
    updateEmptyState();
}


bool FileListMenu::deleteItem(const QString& item)
{
    for (int i = 0; i < m_layout->count(); ++i) {
        QLayoutItem* layoutItem = m_layout->itemAt(i);
        if (auto* label = qobject_cast< QLabel* >(layoutItem->widget())) {
            if (label->text() == item) {
                m_layout->takeAt(i);

                label->deleteLater();
                updateEmptyState();

                return true;
            }
        }
    }
    return false;
}

void FileListMenu::clearItems()
{
    QLayoutItem* child;
    while ((child = m_layout->takeAt(0)) != nullptr) {
        if (child->widget() != m_emptyLabel) {
            delete child->widget();
            delete child;
        }
    }

    updateEmptyState();
}


void FileListMenu::resizeEvent(QResizeEvent *event)
{
    OverlayWidget::resizeEvent(event);

    if (event->oldSize().height() > 0)
    {
        int heightDifference = event->size().height() - event->oldSize().height();
        if (heightDifference != 0) {
            move(x(), y() - heightDifference);
        }
    }
}


void FileListMenu::updateEmptyState()
{
    // A layout has more than one item if it's not empty
    // (the m_emptyLabel is always there, but might be hidden)
    bool isEmpty = (m_layout->count() <= 1);
    m_emptyLabel->setVisible(isEmpty);
    adjustSize();
    updateGeometry();
}


QLabel* FileListMenu::createItemLabel(const QString& item)
{
    auto* label = new InteractiveLabel(item);
    qInfo() << "Created item label with text: " << item;
    label->setStyleSheet("color: white; padding: 4px 8px; border-radius: 4px;");
    label->setCursor(Qt::PointingHandCursor);

    connect(label, &InteractiveLabel::clicked, [this, item]() {
        emit itemSelected(item);
        hide();
    });

    return label;
}
