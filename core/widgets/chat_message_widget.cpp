#include "chat_message_widget.h"

#include "../highlighters/markdown_highlighter.h"

#include "../utils/platform.h"

#include "../config/app_config.h"
#include "../config_manager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextBrowser>
#include <QPainter>
#include <QPainterPath>
#include <QLabel>
#include <QFileInfo>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QTextBlock>
#include <QMenu>
#include <QTextCursor>
#include <QAbstractTextDocumentLayout>


namespace
{
const int    PADDING                   = 10;
const int    BORDER_RADIUS             = 15;
const int    ATTACHMENT_SPACING        = 5;
const int    MIN_ATTACHMENT_WIDTH      = 150;
const int    MIN_ATTACHMENT_HEIGHT     = 150;
const int    MAX_ATTACHMENTS_PER_ROW   = 3;
const QColor TEXT_COLOR                = QColor("#FFFFFF");
}

class AutoResizingTextBrowser : public QTextBrowser {
    Q_OBJECT
public:
    explicit AutoResizingTextBrowser(QWidget *parent = nullptr)
    {
        connect(document()->documentLayout(), &QAbstractTextDocumentLayout::documentSizeChanged,
                this, &AutoResizingTextBrowser::updateGeometryAndSize);
    }

    QSize sizeHint() const override { return QSize(-1, document()->size().height()); }
    QSize minimumSizeHint() const override { return QSize(-1, document()->size().height()); }

public slots:
    void updateGeometryAndSize() { updateGeometry(); }
};


ChatMessageWidget::ChatMessageWidget(const ChatMessage &message, QWidget *parent)
    : QWidget(parent)
    , m_message(message)
    , m_bubbleContainer(nullptr)
{
    setupUi();
}


QSize ChatMessageWidget::minimumSizeHint() const {
    // We tell the layout system that this widget can be shrunk horizontally
    // to zero without any issue.
    // However, we still want to respect the calculated vertical hint,
    // which is essential for the QVBoxLayout in the QScrollArea to work correctly.
    return QSize(0, QWidget::minimumSizeHint().height());
}


void ChatMessageWidget::setupUi()
{
    m_textBrowser = new AutoResizingTextBrowser(this);
    m_textBrowser->setMarkdown(m_message.Text);
    m_textBrowser->setReadOnly(true);
    m_textBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_textBrowser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_textBrowser->setFrameShape(QFrame::NoFrame);

    m_textBrowser->setStyleSheet(QString("QTextBrowser { background: transparent; color: %1; border: none; }")
                                     .arg(TEXT_COLOR.name()));

    m_textBrowser->document()->setDocumentMargin(0);

    // Iterate through all text blocks and disable the "non-breakable" flag.
    // for (QTextBlock block = m_textBrowser->document()->begin(); block.isValid(); block = block.next()) {
    //     QTextBlockFormat blockFormat = block.blockFormat();
    //     if (blockFormat.nonBreakableLines()) {
    //         blockFormat.setNonBreakableLines(false);
    //         QTextCursor cursor(block);
    //         cursor.setBlockFormat(blockFormat);
    //     }
    // }

    m_textBrowser->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_textBrowser, &QWidget::customContextMenuRequested, this, &ChatMessageWidget::onContextMenuRequested);

    m_highlighter = new QSourceHighlite::QSourceHighliter(m_textBrowser->document());

    m_attachmentContainer = new QWidget(this);
    auto *attachmentLayout = new QVBoxLayout(m_attachmentContainer);
    attachmentLayout->setContentsMargins(0,0,0,0);
    attachmentLayout->setSpacing(ATTACHMENT_SPACING);

    QList< ChatMessage::Attachment > nonHiddenAttachments;
    for (const auto& attachment : m_message.Attachments) { if (!attachment.IsHidden) nonHiddenAttachments.push_back(attachment); }

    if (!nonHiddenAttachments.empty())
    {
        int remaining = nonHiddenAttachments.count();
        int attachmentIndex = 0;

        while(remaining > 0)
        {
            auto *rowLayout = new QHBoxLayout();
            rowLayout->setSpacing(ATTACHMENT_SPACING);

            int columns = qMin(remaining, MAX_ATTACHMENTS_PER_ROW);
            for (int i=0; i < columns; ++i)
            {
                QLabel *attachmentLabel = new QLabel();
                attachmentLabel->setMinimumSize(MIN_ATTACHMENT_WIDTH, MIN_ATTACHMENT_HEIGHT);
                attachmentLabel->setStyleSheet("background-color: #464646; padding: 0px; border-radius: 5px;");
                attachmentLabel->setAlignment(Qt::AlignCenter);

                const auto& attachment = nonHiddenAttachments.at(attachmentIndex++);

                QPixmap pixmap;
                if (!attachment.Image)
                {
                    pixmap.load(":/images/file_icon.png");
                    pixmap = pixmap.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                } else {
                    pixmap = attachment.Image.scaled(MIN_ATTACHMENT_WIDTH * 2, MIN_ATTACHMENT_HEIGHT * 2, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                }

                attachmentLabel->setPixmap(pixmap);
                rowLayout->addWidget(attachmentLabel);
            }

            remaining -= columns;
            attachmentLayout->addLayout(rowLayout);
        }
    } else {
        m_attachmentContainer->setVisible(false);
    }

    // Main Widget Layout
    auto *bubbleLayout = new QVBoxLayout();
    bubbleLayout->setContentsMargins(PADDING, PADDING, PADDING, PADDING);
    if (!m_message.Text.isEmpty()) {
        bubbleLayout->addWidget(m_textBrowser);
    } else {
        m_textBrowser->setVisible(false);
    }
    bubbleLayout->addWidget(m_attachmentContainer);

    m_bubbleContainer = new QWidget(this);
    m_bubbleContainer->setLayout(bubbleLayout);
    m_bubbleContainer->setStyleSheet("background: transparent;");

    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    switch (m_message.Alignment)
    {
        case ChatMessage::AlignLeft:
            mainLayout->addWidget(m_bubbleContainer, 8);
            mainLayout->addStretch(2);
            break;
        case ChatMessage::AlignRight:
            mainLayout->addStretch(2);
            mainLayout->addWidget(m_bubbleContainer, 8);
            break;
        default:
            mainLayout->addWidget(m_bubbleContainer);
            break;
    }

    setLayout(mainLayout);
}


void ChatMessageWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    if (!m_textBrowser->document() || !m_bubbleContainer) {
        return;
    }

    const int textWidth = m_bubbleContainer->width() - 2 * PADDING;

    if (m_textBrowser->document()->textWidth() != textWidth) {
        m_textBrowser->document()->setTextWidth(textWidth);
    }
}


void ChatMessageWidget::onContextMenuRequested(const QPoint &pos)
{
    auto* menu = m_textBrowser->createStandardContextMenu();
    menu->setStyleSheet("background: yellow;");
    menu->setAttribute(Qt::WA_DeleteOnClose);

    if (!utils::SetWindowNoCapture(menu->winId(), ConfigManager::instance()->appConfig()->excludeFromCapture))
    {
        qCritical() << "Failed to set NoCapture_Flag to menu, not gonna show it";
        menu->close();
        return;
    }

    menu->addSeparator();
    QAction *deleteAction = menu->addAction("Delete Message");
    connect(deleteAction, &QAction::triggered, this, &ChatMessageWidget::deleteRequested);

    menu->exec(m_textBrowser->mapToGlobal(pos));
}


void ChatMessageWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (!m_bubbleContainer) return;

    QRect bubbleRect = m_bubbleContainer->geometry();

    QPainterPath path;
    path.addRoundedRect(bubbleRect, BORDER_RADIUS, BORDER_RADIUS);
    painter.fillPath(path, m_message.MessageColor);
}

#include "chat_message_widget.moc"
