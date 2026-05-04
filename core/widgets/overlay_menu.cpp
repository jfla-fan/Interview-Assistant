#include "overlay_menu.h"

#include "private/base_widget_impl.h"


OverlayMenu::OverlayMenu(QWidget *parent, bool excludeFromCapture)
    : OverlayMenu("OverlayMenu", parent, excludeFromCapture)
{ }


OverlayMenu::OverlayMenu(const QString &title, QWidget *parent, bool excludeFromCapture)
    : QMenu(title, parent)
    , m_baseWidgetImpl(std::make_unique< BaseWidgetImpl >(this))
{
    m_baseWidgetImpl->initialize(title, excludeFromCapture);
    applyStyling();
}


OverlayMenu::~OverlayMenu() = default;


void OverlayMenu::showEvent(QShowEvent *event)
{
    m_baseWidgetImpl->setExcludeFromCapture(true, true);
    QMenu::showEvent(event);
}


void OverlayMenu::applyStyling()
{
    setStyleSheet(R"(
        QMenu {
            background-color: rgb(40, 40, 40);
            color: #e0e0e0;
            border: 1px solid #555;
            padding: 2px;
            font-size: 9pt;
        }

        QMenu::item {
            padding: 3px 20px 3px 20px;
            border-radius: 3px;
            margin: 0px;
        }

        QMenu::item:selected {
            background-color: #4da6ff;
            color: white;
        }

        QMenu::item:disabled {
            color: #777;
        }

        QMenu::separator {
            height: 1px;
            background-color: #555;
            margin: 3px 2px;
        }

        QMenu::indicator:checkable {
            width: 13px;
            height: 13px;
            padding-left: 3px;
        }

        QMenu::indicator:checkable:checked {
            image: url(:/qt-project.org/styles/commonstyle/images/standardbutton-apply-16.png);
            /* Center the checkmark */
            position: absolute;
            top: 2px;
            left: 3px;
        }

        QMenu::icon {
            padding-left: 5px;
        }
    )");
}


void OverlayMenu::moveBy(int dx, int dy, bool recursive)
{
    m_baseWidgetImpl->moveBy(dx, dy, recursive);
}


bool OverlayMenu::setExcludeFromCapture(bool value, bool recursive)
{
    return m_baseWidgetImpl->setExcludeFromCapture(value, recursive);
}


bool OverlayMenu::isExcludedFromCapture() const
{
    return m_baseWidgetImpl->isExcludedFromCapture();
}
