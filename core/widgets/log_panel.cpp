#include "log_panel.h"

#include "../log/formatters/fmt.h"
#include "../log/components/qt_text_edit.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QTextCursor>
#include <QPalette>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QTextBlock>
#include <QWheelEvent>


static QString s_buttonStyleSheet = R"(
    QPushButton {
        background: transparent;
        color: white;
        border: none;
        padding: 4px 8px;
        border-radius: 4px;
    }
    QPushButton:hover {
        background: #e64c4c;
    }
)";


LogPanel::LogPanel(QWidget* parent)
    : OverlayWidget("LogPanel", parent)
    , m_loggingStopped(false)
{

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    // mainLayout->setSpacing(0);

    // Top bar with controls
    QWidget* menuBar = new QWidget(this);
    menuBar->setStyleSheet("background: rgba(50, 50, 50, 220); border-top-left-radius: 10px; border-top-right-radius: 10px;");
    menuBar->setMaximumHeight(38);

    QHBoxLayout* controlLayout = new QHBoxLayout(menuBar);

    QLabel* filterLabel = new QLabel("Level:");
    filterLabel->setStyleSheet("color: white; font-size: 12px;");
    controlLayout->addWidget(filterLabel);

    m_levelFilter = new QComboBox();
    m_levelFilter->addItems({ "Trace", "Debug", "Info", "Warn", "Error", "Fatal" });
    m_levelFilter->setStyleSheet(R"(
        QComboBox {
            background: transparent;
            color: white;
            border: none;
            padding: 4px 8px;
            border-radius: 4px;
        }
    )");
    connect(m_levelFilter, &QComboBox::currentIndexChanged, this, [this](int index) {
        setLogLevel(static_cast< LogLevel >(index));
    });

    controlLayout->addWidget(m_levelFilter);

    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    spacer->setStyleSheet("background: transparent;");
    controlLayout->addWidget(spacer);

    m_pauseButton = new QPushButton("Pause", this);
    m_pauseButton->setStyleSheet(s_buttonStyleSheet);
    connect(m_pauseButton, &QPushButton::clicked, this, &LogPanel::toggleLogging);
    controlLayout->addWidget(m_pauseButton);

    m_clearButton = new QPushButton("Clear", this);
    m_clearButton->setStyleSheet(s_buttonStyleSheet);
    connect(m_clearButton, &QPushButton::clicked, this, &LogPanel::clear);
    controlLayout->addWidget(m_clearButton);

    mainLayout->addWidget(menuBar);

    // Log display area
    m_logDisplay = new QTextEdit();
    m_logDisplay->setReadOnly(true);
    m_logDisplay->setStyleSheet(R"(
        QTextEdit {
            background: rgba(50, 50, 50, 220);
            color: gray;
            font-family: Consolas, monospace;
            font-size: 12px;
            border-radius: 8px;
            padding: 5px;
            border: none;
        }
    )");
    // m_logDisplay

    mainLayout->addWidget(m_logDisplay);

    setMinimumSize(400, 300);

    // Position next to MarkdownViewer
    if (parent) {
        QPoint pos = parent->pos() + QPoint(parent->width() + 10, 0);
        int height = parent->height();
        resize(width(), height);
        move(pos);
    }

    m_logDisplay->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    auto logFormatter = FmtLogFormatter::create();
    logFormatter->getFormat().pattern = "[{level}] {message}";

    m_textEditLogComponent = QtTextEditLogComponent::create("TextEditLog", m_logDisplay, logFormatter);

    if (!LogManager::instance()->addComponent(m_textEditLogComponent.get())) {
        qError() << "Failed to add text edit component";

        appendMessage("Failed to initialized text edit component. This panel is now useless.",
                      m_textEditLogComponent->colorScheme().error);
    } else {
        qDebug() << "Initialized text edit log component";
    }

}


void LogPanel::appendMessage(const QString& message, const QColor& color)
{
    if (m_loggingStopped) {
        return;
    }

    QString formatted = QString("<font color='%1'>%3</font><br>")
                            .arg(color.rgb())
                            .arg(message.toHtmlEscaped());

    m_logDisplay->insertHtml(formatted);

    // Auto-scroll
    m_logDisplay->verticalScrollBar()->setValue(m_logDisplay->verticalScrollBar()->maximum());
}


void LogPanel::clear()
{
    m_logDisplay->clear();
}


void LogPanel::toggleLogging()
{
    if (m_loggingStopped) {
        m_loggingStopped = false;
        m_pauseButton->setText("Pause");
    } else {
        m_loggingStopped = true;
        m_pauseButton->setText("Resume");
    }

    emit loggingChanged(m_loggingStopped);
}


void LogPanel::setLogLevel(LogLevel lvl)
{
    m_textEditLogComponent->setMinLevel(lvl);
}


LogLevel LogPanel::getLogLevel() const
{
    return m_logLevel;
}
