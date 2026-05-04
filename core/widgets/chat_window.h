#pragma once

#include <QWidget>


QT_FORWARD_DECLARE_STRUCT(ChatMessage);
QT_FORWARD_DECLARE_CLASS(InteractiveLabel);
QT_FORWARD_DECLARE_CLASS(QScrollArea)
QT_FORWARD_DECLARE_CLASS(QLineEdit);
QT_FORWARD_DECLARE_CLASS(QLabel);
QT_FORWARD_DECLARE_CLASS(QPushButton);
QT_FORWARD_DECLARE_CLASS(QVBoxLayout);
QT_FORWARD_DECLARE_CLASS(QHBoxLayout);
QT_FORWARD_DECLARE_CLASS(ChatMessageWidget);


class ChatWindow : public QWidget
{
    Q_OBJECT

public:
    ChatWindow(QWidget *parent = nullptr);

    void addScreenshotPreview(const QPixmap& screenshot);
    bool removeScreenshotPreview(int index = -1);
    qsizetype screenshotPreviewsSize() const;
    void clearScreenshotPreviews();

    int addMessage(const ChatMessage& message);
    bool removeMessage(qint64 messageId);
    std::optional< std::reference_wrapper< ChatMessage > > findMessage(qint64 messageId);
    QList< ChatMessage > messages() const;
    void clearMessages();

signals:
    void screenshotPreviewRemoved(int index);
    void screenshotPreviewClicked(int index);
    void messageSent(const QString& prompt);
    void messageRemoved(const ChatMessage& message);
    void filesChosen(const QStringList& filesPaths);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onScreenshotPreviewLabelClicked();

private:
    void setupUi();

    QScrollArea *m_scrollArea;
    QWidget     *m_chatContainer;
    QWidget     *m_screenshotPreviewContainer;
    QHBoxLayout *m_screenshotPreviewLayout;
    QVBoxLayout *m_chatLayout;

    QLineEdit   *m_messageEdit;
    QPushButton *m_uploadButton;
    QPushButton *m_sendButton;
    QLabel      *m_dragDropOverlay;

    QMap< qint64, ChatMessageWidget* > m_messageWidgets;

    enum ImageSize : uint8_t
    {
        SmallSize,   // 16 pixels
        AverageSize, // 32 pixels
        LargeSize,   // 64 pixels
        NoneSize     // Hidden
    };

    struct ScreenshotPreviewEntry
    {
        QPixmap originalImage {};
        ImageSize currentState { NoneSize };
        std::array< std::optional< QPixmap >, 3 > images {};
        InteractiveLabel* widget { nullptr };
    };

    ImageSize calculateImageSize(int previewsCount) const;
    void updatePreviewImage(ImageSize size, ScreenshotPreviewEntry& entry);
    void recalculatePreviews();

    QList< ScreenshotPreviewEntry > m_screenshotPreviews;
};
