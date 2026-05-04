#pragma once

#include "i_base_widget.h"

#include <QWidget>


QT_FORWARD_DECLARE_CLASS(BaseWidgetImpl);


class BaseWidget : public QWidget, public IBaseWidget
{
    Q_OBJECT
    Q_INTERFACES(IBaseWidget)
public:
    explicit BaseWidget(const QString& objectName, QWidget *parent = nullptr, bool excludeFromCapture = false);
    explicit BaseWidget(QWidget *parent = nullptr, bool excludeFromCapture = true);

    ~BaseWidget() override;

public:
    bool isExcludedFromCapture() const override;

public slots:
    void moveBy(int dx, int dy, bool recursive = true) override;
    bool setExcludeFromCapture(bool value, bool recursive = true) override;
    bool toggleExcludeFromCapture(bool recursive = true);

protected:
    std::unique_ptr< BaseWidgetImpl > m_impl;
};
