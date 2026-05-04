#pragma once

#include "../action.h"

#include "base_widget.h"

enum class ActionType;


/**

*/
class OverlayWidget : public BaseWidget
{
    Q_OBJECT
public:
    explicit OverlayWidget(QString objectName, QWidget *parent = nullptr, bool excludeFromCapture = false);
    explicit OverlayWidget(QWidget *parent = nullptr, bool excludeFromCapture = false);

public slots:
    virtual void toggleVisibility(bool recursive = true);

protected:
    void hideEvent(QHideEvent *event) override;
    void showEvent(QShowEvent *event) override;

protected:
    bool m_wasVisible { false };
};

