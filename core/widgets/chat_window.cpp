#include "chat_window.h"
#include "chat_message_widget.h"
#include "interactive_label.h"

#include "../log/log.h"
#include "../utils/containers.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QScrollBar>
#include <QBuffer>
#include <QGraphicsOpacityEffect>
#include <QFileDialog>
#include <QPainter>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>


namespace
{
    inline constexpr int MAX_SMALL_PREVIEWS_COUNT   = 16;
    inline constexpr int MAX_AVERAGE_PREVIEWS_COUNT = 12;
    inline constexpr int MAX_LARGE_PREVIEWS_COUNT   = 8;

    inline constexpr int SMALL_PREVIEW_SIZE   = 32;
    inline constexpr int AVERAGE_PREVIEW_SIZE = 48;
    inline constexpr int LARGE_PREVIEW_SIZE   = 64;
}


ChatWindow::ChatWindow(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}


void ChatWindow::addScreenshotPreview(const QPixmap& screenshot)
{
    if (screenshot.isNull()) {
        qWarning() << "Screenshot preview is null. Unable to add a preview";
        return;
    }

    ScreenshotPreviewEntry entry;

    entry.originalImage = screenshot;

    {
        QGraphicsOpacityEffect* opacityEffect = new QGraphicsOpacityEffect(this);
        opacityEffect->setOpacity(0.7);

        InteractiveLabel* previewLabel = new InteractiveLabel(this);
        previewLabel->setScaledContents(true);
        previewLabel->setGraphicsEffect(opacityEffect);
        previewLabel->setStyleSheet(QStringLiteral("border: 1px solid #444; border-radius: 5px;"));

        entry.widget = previewLabel;

        connect(entry.widget, &InteractiveLabel::clicked, this, &ChatWindow::onScreenshotPreviewLabelClicked);
    }

    m_screenshotPreviews.push_back(std::move(entry));

    recalculatePreviews();
}


bool ChatWindow::removeScreenshotPreview(int index)
{
    if (m_screenshotPreviews.isEmpty()) {
        qWarning("Screenshots preview widgets list is empty");
        return false;
    }

    // Default to last element if index is -1
    if (index == -1) {
        index = m_screenshotPreviews.count() - 1;
    }

    if (index < 0 || index >= m_screenshotPreviews.count()) {
        qWarning() << QString("Index %1 is out of range, must be: 0 <= %1 < %2").arg(index).arg(m_screenshotPreviews.count());
        return false;
    }

    ScreenshotPreviewEntry entry = m_screenshotPreviews.takeAt(index);
    m_screenshotPreviewLayout->removeWidget(entry.widget);
    emit screenshotPreviewRemoved(index);

    entry.widget->deleteLater();

    if (m_screenshotPreviews.isEmpty()) {
        m_screenshotPreviewContainer->hide();
        return true;
    }

    recalculatePreviews();

    return true;
}


qsizetype ChatWindow::screenshotPreviewsSize() const
{
    return m_screenshotPreviews.size();
}


void ChatWindow::clearScreenshotPreviews()
{
    for (const auto& entry : m_screenshotPreviews) {
        entry.widget->deleteLater();
    }

    m_screenshotPreviews.clear();
    m_screenshotPreviewContainer->hide();
}


int ChatWindow::addMessage(const ChatMessage &message)
{
    static int s_lastId = 0;

    auto *messageWidget = new ChatMessageWidget(message, m_chatContainer);
    messageWidget->message().Id = ++s_lastId;

    connect(messageWidget, &ChatMessageWidget::deleteRequested, this, [this, id = messageWidget->message().Id]() {
        removeMessage(id);
    });

    m_chatLayout->insertWidget(m_chatLayout->count() - 1, messageWidget);
    m_messageWidgets.insert(messageWidget->message().Id, messageWidget);

    // Scroll to bottom after the layout has had a chance to update
    QTimer::singleShot(50, this, [this](){
        m_scrollArea->verticalScrollBar()->setValue(m_scrollArea->verticalScrollBar()->maximum());
    });

    return messageWidget->message().Id;
}


bool ChatWindow::removeMessage(qint64 messageId)
{
    if (!m_messageWidgets.contains(messageId)) {
        return false;
    }

    ChatMessageWidget* widget = m_messageWidgets.take(messageId);
    emit messageRemoved(widget->message());
    widget->deleteLater();

    return true;
}


std::optional< std::reference_wrapper< ChatMessage > > ChatWindow::findMessage(qint64 messageId)
{
    if (m_messageWidgets.contains(messageId)) {
        return m_messageWidgets.value(messageId)->message();
    }

    return std::nullopt;
}


QList< ChatMessage > ChatWindow::messages() const
{
    QList< ChatMessage > messages;
    for (const ChatMessageWidget* widget : m_messageWidgets)
        messages.append(widget->message());

    return messages;
}


void ChatWindow::clearMessages()
{
    QLayoutItem* item;
    while ((item = m_chatLayout->takeAt(0)) != nullptr) {
        if (item->spacerItem()) {
            m_chatLayout->addStretch(); // @todo (a mess?) Put it back at the end
            delete item;
            break;
        }

        if (item->widget()) {
            item->widget()->deleteLater();
        }

        delete item;
    }

    m_messageWidgets.clear();
}


void ChatWindow::dragEnterEvent(QDragEnterEvent *event)
{
    // Only files
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
        m_dragDropOverlay->show();
    } else {
        event->ignore();
    }
}


void ChatWindow::dragLeaveEvent(QDragLeaveEvent *event)
{
    m_dragDropOverlay->hide();
    event->accept();
}


void ChatWindow::dropEvent(QDropEvent *event)
{
    m_dragDropOverlay->hide();
    const QMimeData* mimeData = event->mimeData();

    if (mimeData->hasUrls()) {
        QStringList pathList;
        QList< QUrl > urlList = mimeData->urls();

        for (const QUrl &url : urlList) {
            if (url.isLocalFile()) {
                pathList.append(url.toLocalFile());
            }
        }
        // processFilePaths(pathList);
        for (const auto& filePath: pathList) {
            qDebug() << filePath << " file chosen";
        }

        emit filesChosen(pathList);
        event->acceptProposedAction();
    } else {
        qWarning() << "No urls in mime data for drop event";
        event->ignore();
    }
}


void ChatWindow::resizeEvent(QResizeEvent* event)
{
    if (m_dragDropOverlay) {
        m_dragDropOverlay->setGeometry(m_scrollArea->geometry());
    }

    QWidget::resizeEvent(event);
}


void ChatWindow::onScreenshotPreviewLabelClicked()
{
    InteractiveLabel* label = qobject_cast< InteractiveLabel* >(sender());
    int index = utils::IndexOrNullptr< int >(m_screenshotPreviews, [label](const auto& entry) { return entry.widget == label; });
    LOG_TRACE("Screenshot preview at index {} has been clicked", index);
    emit screenshotPreviewClicked(index);
}


void ChatWindow::setupUi()
{
    setAcceptDrops(true);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
    m_scrollArea->setStyleSheet("QScrollArea { background-color: rgba(30, 30, 30, 220); border: none; }");

    m_chatContainer = new QWidget();
    m_chatContainer->setStyleSheet("background-color: transparent;");

    m_chatLayout = new QVBoxLayout(m_chatContainer);
    m_chatLayout->addStretch();

    m_scrollArea->setWidget(m_chatContainer);

    // --- Screenshot Preview Area ---
    m_screenshotPreviewContainer = new QWidget();
    m_screenshotPreviewLayout = new QHBoxLayout(m_screenshotPreviewContainer);
    m_screenshotPreviewLayout->setSpacing(5);
    m_screenshotPreviewLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_screenshotPreviewContainer->setStyleSheet("background: rgba(30, 30, 30, 200);");
    m_screenshotPreviewContainer->hide();

    // // Chat Input Bar with Icons
    QWidget* inputBarContainer = new QWidget();
    inputBarContainer->setStyleSheet("background: rgba(50, 50, 50, 220);");
    QHBoxLayout* inputLayout = new QHBoxLayout(inputBarContainer);
    inputLayout->setSpacing(5);

    m_uploadButton = new QPushButton("+");
    m_uploadButton->setFlat(true);
    m_uploadButton->setStyleSheet(R"(
        QPushButton {
            background: transparent;
            font-size: 20px;
            border: 2px solid #e0e0e0;
            color: #e0e0e0;
            border-radius: 15px; /* half of fixed size for perfect circle */
            width: 30px;
            height: 30px;
            padding: 0px;
            text-align: center;
            padding-bottom: 4px;
            font-weight: bold;
            line-height: 26px; /* aligns "+" vertically inside the circle */
        }

        QPushButton:hover {
            border-color: #4da6ff;
            color: #4da6ff;
        }
    )");
    m_uploadButton->setFixedSize(30, 30);

    m_messageEdit = new QLineEdit();
    m_messageEdit->setPlaceholderText("Type your message... (Shift+Enter for newline, Enter to send)");
    m_messageEdit->setStyleSheet(R"(
        QLineEdit {
            background: rgba(30, 30, 30, 220);
            color: #e0e0e0;
            border: 1px solid #666;
            border-bottom: 1px solid #666;
            border-radius: 5px;
            padding: 5px;
        }

        QLineEdit:focus {
            border: 1px solid #777;
            border-bottom: 1px solid #777;
        }
    )");
    m_messageEdit->setMinimumHeight(30);
    m_messageEdit->setMaximumHeight(100);

    m_sendButton = new QPushButton("Send");
    m_sendButton->setStyleSheet("QPushButton { background-color: #4da6ff; color: white; border: none; border-radius: 5px; padding: 8px 15px; }"
                                "QPushButton:hover { background-color: #3b8fe0; }"
                                "QPushButton:disabled { background-color: #666; color: #aaa; }");
    m_sendButton->setFixedSize(60, 30);

    inputLayout->addWidget(m_messageEdit);
    inputLayout->addWidget(m_uploadButton);
    inputLayout->addWidget(m_sendButton);

    // --- Create the custom Drop Overlay ---
    m_dragDropOverlay = new QLabel(this);
    m_dragDropOverlay->setAlignment(Qt::AlignCenter);
    m_dragDropOverlay->setStyleSheet(QStringLiteral(R"(
        QLabel {
            background-color: rgba(0, 0, 0, 0.65);
            border-radius: 8px;
        }
    )"));

    // Programmatically create the '+' icon @todo change this
    const int iconSize = 80;
    const int lineThickness = 6;
    QPixmap plusIcon(iconSize, iconSize);
    plusIcon.fill(Qt::transparent); // Start with a transparent pixmap

    QPainter painter(&plusIcon);
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen(Qt::white, lineThickness, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(pen);

    // Draw horizontal line
    painter.drawLine(iconSize / 4, iconSize / 2, iconSize * 3 / 4, iconSize / 2);
    // Draw vertical line
    painter.drawLine(iconSize / 2, iconSize / 4, iconSize / 2, iconSize * 3 / 4);
    painter.end();

    m_dragDropOverlay->setPixmap(plusIcon);
    m_dragDropOverlay->hide();

    connect(m_uploadButton, &QPushButton::clicked, this, [this]()
    {
        QFileDialog dialog(this, "Attach File");
        dialog.setFileMode(QFileDialog::ExistingFile);

        if (dialog.exec() == QDialog::Accepted) {
            QString filePath = dialog.selectedFiles().first();
            if (!filePath.isEmpty()) {
                qDebug() << "Selected file:" << filePath;
                emit filesChosen({ filePath });
            }
        }
    });

    // // --- Layouts ---
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    mainLayout->addWidget(m_scrollArea);
    mainLayout->addWidget(m_screenshotPreviewContainer);
    mainLayout->addWidget(inputBarContainer);

    setLayout(mainLayout);

    m_dragDropOverlay->raise();

    // // --- Connections ---
    connect(m_sendButton, &QPushButton::clicked, this, [this]() {
        if (!m_messageEdit->text().isEmpty()) {
            emit messageSent(m_messageEdit->text().trimmed());
            m_messageEdit->clear();
        }
    });

    connect(m_messageEdit, &QLineEdit::returnPressed, this, [this]() {
        if (!m_messageEdit->text().isEmpty()) {
            emit messageSent(m_messageEdit->text().trimmed());
            m_messageEdit->clear();
        }
    });

    resize(500, 700);
}


ChatWindow::ImageSize ChatWindow::calculateImageSize(int previewsCount) const
{
    previewsCount = qBound(0, previewsCount, previewsCount); // clamp >= 0

    if (previewsCount > MAX_SMALL_PREVIEWS_COUNT)
        return NoneSize;
    else if (previewsCount > MAX_AVERAGE_PREVIEWS_COUNT)
        return SmallSize;
    else if (previewsCount > MAX_LARGE_PREVIEWS_COUNT)
        return AverageSize;
    else
        return LargeSize;
}


void ChatWindow::updatePreviewImage(ImageSize size, ScreenshotPreviewEntry& entry)
{
    if (size == entry.currentState) {
        return;
    }

    if (size != NoneSize) {
        m_screenshotPreviewLayout->addWidget(entry.widget);
    }

    entry.currentState = size;

    switch (size)
    {
        case NoneSize:
        {
            m_screenshotPreviewLayout->removeWidget(entry.widget);
            return;
        }

        case SmallSize:
        {
            if (!entry.images[SmallSize]) {
                entry.images[SmallSize] = entry.originalImage.scaled(SMALL_PREVIEW_SIZE,
                                                                     SMALL_PREVIEW_SIZE,
                                                                     Qt::KeepAspectRatio,
                                                                     Qt::SmoothTransformation);
            }

            break;
        }

        case AverageSize:
        {
            if (!entry.images[AverageSize]) {
                entry.images[AverageSize] = entry.originalImage.scaled(AVERAGE_PREVIEW_SIZE,
                                                                       AVERAGE_PREVIEW_SIZE,
                                                                       Qt::KeepAspectRatio,
                                                                       Qt::SmoothTransformation);
            }

            break;
        }

        case LargeSize:
        {
            if (!entry.images[LargeSize]) {
                entry.images[LargeSize] = entry.originalImage.scaled(LARGE_PREVIEW_SIZE,
                                                                     LARGE_PREVIEW_SIZE,
                                                                     Qt::KeepAspectRatio,
                                                                     Qt::SmoothTransformation);
            }

            break;
        }

        default:
            LOG_ERROR() << "Failed to identify image size. Contact developers." << ::ToQString(size);
            return;
    }

    entry.widget->setPixmap(*entry.images[size]);
    entry.widget->setFixedSize(entry.images[size]->size());
}


void ChatWindow::recalculatePreviews()
{
    // @todo think about the state before. Maybe we shouldn't iterate over all the previews every time
    if (!m_screenshotPreviews.isEmpty()) {

        /* Possible layouts:
         * - Big Size
         * - Average Size
         * - Small Size + (Hidden Previews)
        */

        ImageSize expectedSize = calculateImageSize(m_screenshotPreviews.size());

        switch (expectedSize)
        {
            case NoneSize:
            {
                for (int i = 0; i < MAX_SMALL_PREVIEWS_COUNT; ++i) {
                    updatePreviewImage(SmallSize, m_screenshotPreviews[i]);
                }

                for (int i = MAX_SMALL_PREVIEWS_COUNT; i < m_screenshotPreviews.count(); ++i) {
                    updatePreviewImage(NoneSize, m_screenshotPreviews[i]);
                }

                break;
            }

            default:
            {
                for (auto& preview : m_screenshotPreviews) {
                    updatePreviewImage(expectedSize, preview);
                }

                break;
            }
        }
    }

    if (m_screenshotPreviews.isEmpty()) {
        m_screenshotPreviewContainer->hide();
    }
    else {
        m_screenshotPreviewContainer->show();
    }
}
