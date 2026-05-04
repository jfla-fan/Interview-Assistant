#pragma once


class QWidget;
class QString;


class BaseWidgetImpl
{
public:
    explicit BaseWidgetImpl(QWidget* owner);
    ~BaseWidgetImpl() = default;

    void initialize(const QString& name, bool excludeFromCapture = false);
    void moveBy(int dx, int dy, bool recursive);
    bool setExcludeFromCapture(bool value, bool recursive);
    bool isExcludedFromCapture() const;
    bool toggleExcludedFromCapture(bool recursive);

private:
    QWidget* m_owner;
    bool m_isExcludedFromCapture { false };
};
