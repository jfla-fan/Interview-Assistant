#pragma once

#include <QLabel>
#include <QObject>


class InteractiveLabel : public QLabel
{
    Q_OBJECT
public:
    InteractiveLabel(QWidget* parent = nullptr);
    InteractiveLabel(const QString& text, QWidget* parent = nullptr);

signals:
    void clicked();
    void hovered();

protected:
    void mousePressEvent(QMouseEvent* event) override;
};
