#include "../i_base_widget.h"

#include "../../utils/platform.h"

#include "base_widget_impl.h"

#include <QtAssert>
#include <QWidget>

#include <Windows.h>


BaseWidgetImpl::BaseWidgetImpl(QWidget* owner)
    : m_owner(owner)
{
    Q_ASSERT(owner);
}


void BaseWidgetImpl::initialize(const QString& name, bool excludeFromCapture)
{
    m_owner->setObjectName(name);
    setExcludeFromCapture(excludeFromCapture, false);
}


void BaseWidgetImpl::moveBy(int dx, int dy, bool recursive)
{
    m_owner->move(m_owner->pos() + QPoint(dx, dy));

    if (recursive) {
        for (auto* child : m_owner->findChildren< QObject* >()) {
            auto* movableChild = qobject_cast< IBaseWidget* >(child);
            if (movableChild) {
                movableChild->moveBy(dx, dy, false);
            }
        }
    }
}


bool BaseWidgetImpl::setExcludeFromCapture(bool value, bool recursive)
{
    if (value && !m_isExcludedFromCapture) {
        if (!utils::SetWindowNoCapture(m_owner->winId(), true)) {
            qCritical() << "Failed to exclude from capture window " << m_owner->objectName();
            return false;
        } else {
            m_isExcludedFromCapture = true;
        }
    } else if (!value && m_isExcludedFromCapture) {
        if (!utils::SetWindowNoCapture(m_owner->winId(), false)) {
            qCritical() << "Failed to UNexclude from capture window " << m_owner->objectName();
            return false;
        } else {
            m_isExcludedFromCapture = false;
        }
    }

    if (recursive) {
        for (auto* child : m_owner->findChildren< QObject* >()) {
            auto* baseWidgetChild = qobject_cast< IBaseWidget* >(child);
            if (baseWidgetChild) {
                if (!baseWidgetChild->setExcludeFromCapture(value, false)) {
                    return false;
                }
            }
        }
    }

    return true;
}


bool BaseWidgetImpl::isExcludedFromCapture() const
{
    return m_isExcludedFromCapture;
}


bool BaseWidgetImpl::toggleExcludedFromCapture(bool recursive)
{
    return setExcludeFromCapture(!m_isExcludedFromCapture, recursive);
}
