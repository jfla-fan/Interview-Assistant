#pragma once

#include <QtTypes>
#include <QtPlugin>


/**
 * Every custom widget in Interview Assistant project should support this interface.
 * Declared as interface because Qt Moc compiler does not support multiple inheritance.
 * If you cannot derive from BaseWidget directly, implement this interface through
 * BaseWidgetImpl bridge.
*/
class IBaseWidget
{
public:
    virtual ~IBaseWidget() = default;

    virtual void moveBy(int dx, int dy, bool recursive = true) = 0;
    virtual bool setExcludeFromCapture(bool value, bool recursive = true) = 0;
    virtual bool isExcludedFromCapture() const = 0;
};


Q_DECLARE_INTERFACE(IBaseWidget, "IBaseWidget")
