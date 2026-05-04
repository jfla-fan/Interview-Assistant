#pragma once

#include "i_base_widget.h"
#include <QMenu>
#include <memory>

QT_FORWARD_DECLARE_CLASS(BaseWidgetImpl);

/**
 * @brief A QMenu subclass that integrates with the application's style and functionality.
 *
 * This menu is automatically styled to match the dark theme and is made
 * capture-proof using the application's utility functions. It also implements
 * the IBaseWidget interface for consistent behavior with other app widgets.
 */
class OverlayMenu : public QMenu, public IBaseWidget
{
    Q_OBJECT
    Q_INTERFACES(IBaseWidget)

public:
    explicit OverlayMenu(QWidget *parent = nullptr, bool excludeFromCapture = false);
    explicit OverlayMenu(const QString &title, QWidget *parent = nullptr, bool excludeFromCapture = false);
    ~OverlayMenu() override;

public slots:
    void moveBy(int dx, int dy, bool recursive = true) override;
    bool setExcludeFromCapture(bool value, bool recursive = true) override;
    bool isExcludedFromCapture() const override;

protected:
    void showEvent(QShowEvent *event) override;

private:
    void applyStyling();

    std::unique_ptr< BaseWidgetImpl > m_baseWidgetImpl;
};
