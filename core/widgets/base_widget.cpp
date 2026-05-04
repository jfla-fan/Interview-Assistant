#include "base_widget.h"

#include "private/base_widget_impl.h"

#include <QDebug>


BaseWidget::BaseWidget(QWidget *parent, bool excludeFromCapture)
    : QWidget{ parent }
    , m_impl(std::make_unique< BaseWidgetImpl >(this))
{
    m_impl->initialize("", excludeFromCapture);
}


BaseWidget::~BaseWidget() = default;


BaseWidget::BaseWidget(const QString& objectName, QWidget *parent, bool excludeFromCapture)
    : QWidget{ parent }
    , m_impl(std::make_unique< BaseWidgetImpl >(this))
{
    m_impl->initialize(objectName, excludeFromCapture);
}


bool BaseWidget::isExcludedFromCapture() const
{
    return m_impl->isExcludedFromCapture();
}


void BaseWidget::moveBy(int dx, int dy, bool recursive)
{
    m_impl->moveBy(dx, dy, recursive);
}


bool BaseWidget::setExcludeFromCapture(bool value, bool recursive)
{
    return m_impl->setExcludeFromCapture(value, recursive);
}


bool BaseWidget::toggleExcludeFromCapture(bool recursive)
{
    return m_impl->toggleExcludedFromCapture(recursive);
}
