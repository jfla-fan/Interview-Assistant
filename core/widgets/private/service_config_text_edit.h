#pragma once

#include <QTextEdit>


class ServiceConfigTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    explicit ServiceConfigTextEdit(QWidget* parent = nullptr);

    int tabWidth() const { return m_tabWidth; }
    void setTabWidth(int spaces) { m_tabWidth = qMax(1, spaces); }

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    int m_tabWidth;
};
