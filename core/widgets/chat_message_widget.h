#pragma once

#include <QWidget>

#include "../common.h"


QT_FORWARD_DECLARE_CLASS(AutoResizingTextBrowser)

namespace QSourceHighlite
{
    class QSourceHighliter;
}


class ChatMessageWidget : public QWidget {
    Q_OBJECT

public:
    explicit ChatMessageWidget(const ChatMessage &message, QWidget *parent = nullptr);

    ChatMessage& message() { return m_message; }
    const ChatMessage& message() const { return m_message; }
    QSize minimumSizeHint() const override;

signals:
    void deleteRequested();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onContextMenuRequested(const QPoint &pos);

private:
    void setupUi();

    ChatMessage m_message;
    AutoResizingTextBrowser *m_textBrowser;
    QWidget *m_attachmentContainer;
    QWidget *m_bubbleContainer;
    QSourceHighlite::QSourceHighliter* m_highlighter;
};
