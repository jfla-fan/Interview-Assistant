#include "markdown_viewer.h"
#include "chat_window.h"
#include "screenshot_tool.h"
#include "interactive_label.h"
#include "file_list_menu.h"
#include "request_info_menu.h"
#include "log_panel.h"
#include "settings.h"

#include "../config_manager.h"
#include "../highlighters/markdown_highlighter.h"

#include <QTextBrowser>
#include <QLineEdit>
#include <QFileDialog>
#include <QLabel>
#include <QStackedWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QBuffer>
#include <QScrollBar>
#include <QGuiApplication>
#include <QSyntaxHighlighter>
#include <QGraphicsOpacityEffect>
#include <QScreen>
#include <QTimer>



MarkdownViewer::MarkdownViewer(QWidget* parent)
    : OverlayWidget("MarkdownViewer", parent)
{
    setupUi();

    m_highlighter = new QSourceHighlite::QSourceHighliter(m_textBrowser->document());

    switchMode(Mode_Chat);
}


MarkdownViewer::~MarkdownViewer() = default;


void MarkdownViewer::setLastOperationStatus(OperationStatus status)
{
    if (m_lastOperationStatus != status) {
        QString color = OperationStatusToColorString(status);

        OperationStatus lastStatus = m_lastOperationStatus;
        m_lastOperationStatus = status;

        m_statusIndicator->setStyleSheet(QString(
                                             "background-color: %1; border-radius: 8px; border: 1px solid #333;"
                                             ).arg(color));

        emit statusChanged(lastStatus, status);
    }
}


OperationStatus MarkdownViewer::getLastOperationStatus() const
{
    return m_lastOperationStatus;
}


void MarkdownViewer::showSettings()
{
    m_settingsDialog->show();
}


void MarkdownViewer::switchMode(Mode mode)
{
    if (mode == m_currentMode) return;

    switch(mode)
    {
        case Mode_SinglePage:
            m_stackedContent->setCurrentWidget(m_textBrowser);
            m_toggleModeButton->setText(QString::fromUtf8("📄"));
            break;

        case Mode_Chat:
            m_stackedContent->setCurrentWidget(m_chatWindow);
            m_toggleModeButton->setText(QString::fromUtf8("💬"));
            break;
    }

    m_currentMode = mode;
}


void MarkdownViewer::toggleMode()
{
    switchMode(static_cast< Mode >(1 - m_currentMode));
}


void MarkdownViewer::clearText()
{
    m_textBrowser->clear();
}


void MarkdownViewer::clearChatMessages()
{
    m_chatWindow->clearMessages();
}


void MarkdownViewer::clearChatPreviews()
{
    m_chatWindow->clearScreenshotPreviews();
}


int MarkdownViewer::appendChatMessage(const ChatMessage& message)
{
    return m_chatWindow->addMessage(message);
}


void MarkdownViewer::addScreenshotPreview(const QPixmap& screenshot)
{
    m_chatWindow->addScreenshotPreview(screenshot);
}


bool MarkdownViewer::deleteScreenshotPreview(int index)
{
    return m_chatWindow->removeScreenshotPreview(index);
}


void MarkdownViewer::takeCroppedScreenshot()
{
    m_screenshotTool->show();
}


void MarkdownViewer::addFileMenuItem(const QString& fileName)
{
    m_fileListMenu->addItem(fileName);
}


bool MarkdownViewer::deleteFileMenuItem(const QString& fileName)
{
    return m_fileListMenu->deleteItem(fileName);
}


void MarkdownViewer::resetSettings()
{
    m_settingsDialog->resetAll();
}


void MarkdownViewer::resetSettings(const SettingsView& data)
{
    m_settingsDialog->resetAll(data);
}


void MarkdownViewer::setMarkdownText(const QString& text)
{
    m_textBrowser->setMarkdown(text);
}


void MarkdownViewer::setCounter(int value)
{
    m_counterLabel->setText(QString::number(value));
}


void MarkdownViewer::setRequestInfo(const ServiceRequestLookUpData& item)
{
    qTrace() << "Set request info" << item.toString();
    m_requestLabelData = item;
    m_requestInfoLabel->setText(item.toString());
}


void MarkdownViewer::setRequestsData(const QList< ServiceRequestLookUpData >& items)
{
    m_requestInfoMenu->setItems(items);
    if (!items.empty()) {
        setRequestInfo(items.front());
    }
}


void MarkdownViewer::setMoveStep(QPair< int, int > moveStep)
{
    m_moveStep = std::move(moveStep);
}


void MarkdownViewer::incProxySuccessCounter(QStringView proxyName, int delta)
{
    m_settingsDialog->incProxySuccessCounter(proxyName, delta);
}


void MarkdownViewer::incProxyFailedCounter(QStringView proxyName, int delta)
{
    m_settingsDialog->incProxyFailedCounter(proxyName, delta);
}


bool MarkdownViewer::isEmpty()
{
    return m_textBrowser->document()->isEmpty();
}


int MarkdownViewer::counter()
{
    return m_counterLabel->text().toInt();
}


OptionalRef< ChatMessage > MarkdownViewer::findChatMessage(qint64 id) const
{
    return m_chatWindow->findMessage(id);
}


ServiceRequestLookUpData MarkdownViewer::requestInfo() const
{
    return m_requestLabelData;
}


const QPair< int, int >& MarkdownViewer::moveStep() const
{
    return m_moveStep;
}


qsizetype MarkdownViewer::screenshotPreviewsSize() const
{
    return m_chatWindow->screenshotPreviewsSize();
}


qsizetype MarkdownViewer::fileMenuListSize() const
{
    return m_fileListMenu->size();
}


bool MarkdownViewer::handleAction(ActionType action)
{
    switch (action)
    {
        case ActionType::MoveLeft:
            moveBy(-m_moveStep.first, 0);
            return true;

        case ActionType::MoveRight:
            moveBy(m_moveStep.first, 0);
            return true;

        case ActionType::MoveUp:
            moveBy(0, -m_moveStep.second);
            return true;

        case ActionType::MoveDown:
            moveBy(0, m_moveStep.second);
            return true;

        case ActionType::ToggleVisibility:
            toggleVisibility();
            return true;

        case ActionType::TogglePageMode:
            toggleMode();
            return true;

        case ActionType::ToggleLogPanel:
            m_logPanel->setVisible(!m_logPanel->isVisible());
            return true;

        case ActionType::Crop:
            takeCroppedScreenshot();
            return true;

        default:
            return false;
    }
}


void MarkdownViewer::onCropButtonClicked()
{
    takeCroppedScreenshot();
}


void MarkdownViewer::onScreenshotReady(const QPixmap& screenshot)
{
    qDebug() << "Screenshot ready";

    this->show();
    this->raise();
    this->activateWindow();

    emit screenshotReady(screenshot);
}


void MarkdownViewer::onScreenshotCancelled()
{
    qDebug() << "Screenshot cancelled";

    this->show();
    this->raise();
    this->activateWindow();

    emit screenshotCancelled();
}


void MarkdownViewer::setupSingePageWindow()
{
    m_textBrowser = new QTextBrowser(this);
    m_textBrowser->setStyleSheet(R"(
            QTextBrowser {
                background: rgba(30, 30, 30, 220);
                color: #e0e0e0;
                font-family: Consolas, monospace;
                padding: 15px;
                border: none;
            }
            a { color: #4da6ff; }
            pre {
                background: rgba(0, 0, 0, 0.3);
                border-left: 3px solid #4da6ff;
                padding: 10px;
                margin: 5px 0;
            }
            code {
                font-family: Consolas, monospace;
            }
    )");

    m_textBrowser->setReadOnly(true);

    m_textBrowser->setFrameShape(QFrame::NoFrame);
    m_textBrowser->setLineWrapMode(QTextEdit::WidgetWidth);
    m_textBrowser->setWordWrapMode(QTextOption::WrapAnywhere);

    m_stackedContent->addWidget(m_textBrowser);
}


void MarkdownViewer::setupChatWindow()
{
    m_chatWindow = new ChatWindow(this);
    m_stackedContent->addWidget(m_chatWindow);
}


void MarkdownViewer::setupSettings()
{
    m_settingsDialog = new Settings(ConfigManager::instance()->appConfig(), ConfigManager::instance()->serviceConfig(), this);

    connect(m_settingsDialog, &Settings::okClicked,     this, &MarkdownViewer::settingsOkClicked);
    connect(m_settingsDialog, &Settings::applyClicked,  this, &MarkdownViewer::settingsApplyClicked);
    connect(m_settingsDialog, &Settings::saveClicked,   this, &MarkdownViewer::settingsSaveClicked);
    connect(m_settingsDialog, &Settings::cancelClicked, this, &MarkdownViewer::settingsCancelClicked);
}


void MarkdownViewer::setupUi()
{
    resize(700, 740);
    move(QGuiApplication::primaryScreen()->geometry().center() - rect().center());

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ------------------------------------------------------- Top bar -------------------------------------------------------
    QWidget* m_topBar = new QWidget(this);
    m_topBar->setStyleSheet("background: rgba(50, 50, 50, 220); border-top-left-radius: 10px; border-top-right-radius: 10px;");
    QHBoxLayout* menuLayout = new QHBoxLayout(m_topBar);
    menuLayout->setContentsMargins(10, 5, 10, 5);
    m_topBar->setObjectName("topBar");

    // Status indicator
    m_statusIndicator = new QLabel();
    m_statusIndicator->setFixedSize(16, 16);
    setLastOperationStatus(OperationStatus::Idle);

    // Spacer to push settings button to right
    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    spacer->setObjectName("topSpacer");
    spacer->setStyleSheet("background-color: transparent;");

    // toggle mode button
    m_toggleModeButton = new QPushButton();
    m_toggleModeButton->setFlat(true);
    m_toggleModeButton->setStyleSheet("QPushButton { background: transparent; font-size: 16px; border: none; color: #e0e0e0; }");
    m_toggleModeButton->setFixedSize(28, 28);
    m_toggleModeButton->setObjectName("toggleModeButton");

    m_cropButton = new QPushButton(QString::fromUtf8("✂"));
    m_cropButton->setFlat(true);
    m_cropButton->setStyleSheet("QPushButton { background: transparent; font-size: 16px; border: none; color: #e0e0e0; }");
    m_cropButton->setFixedSize(28, 28);
    m_cropButton->setObjectName("cropButton");

    // Settings button
    m_settingsButton = new QPushButton("⚙");
    m_settingsButton->setFlat(true);
    m_settingsButton->setStyleSheet("QPushButton { background: transparent; font-size: 16px; border: none; }");
    m_settingsButton->setFixedSize(20, 28);
    m_settingsButton->setObjectName("settingsButton");

    menuLayout->addWidget(m_statusIndicator);
    menuLayout->addWidget(spacer);
    menuLayout->addWidget(m_toggleModeButton);
    menuLayout->addWidget(m_cropButton);
    menuLayout->addWidget(m_settingsButton);

    // ----------------------------------------------------- Main Content -----------------------------------------------------
    m_stackedContent = new QStackedWidget(this);

    setupSingePageWindow();
    setupChatWindow();

    // ----------------------------------------------------- Bottom Bar ------------------------------------------------------
    m_bottomBar = new QWidget();
    m_bottomBar->setStyleSheet("background: rgba(50, 50, 50, 220); border-bottom-left-radius: 10px; border-bottom-right-radius: 10px;");
    m_bottomBar->setObjectName("bottomBar");
    QHBoxLayout* bottomLayout = new QHBoxLayout(m_bottomBar);
    bottomLayout->setContentsMargins(10, 5, 10, 5);

    m_counterLabel = new InteractiveLabel("0");
    m_counterLabel->setFixedSize(20, 20);
    m_counterLabel->setStyleSheet("color: gray; font-weight: bold; background: transparent;");
    m_counterLabel->setAlignment(Qt::AlignCenter);
    m_counterLabel->setObjectName("counterLabel");

    // Request info label
    m_requestInfoLabel = new InteractiveLabel();
    m_requestInfoLabel->setStyleSheet("color: gray; background: transparent;");
    m_requestInfoLabel->setTextFormat(Qt::RichText);
    m_requestInfoLabel->setTextInteractionFlags(Qt::NoTextInteraction);
    m_requestInfoLabel->setOpenExternalLinks(false);
    m_requestInfoLabel->setText("Default text");
    m_requestInfoLabel->setObjectName("requestInfoLabel");

    QWidget* bottomSpacer = new QWidget();
    bottomSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    // bottomSpacer->setStyleSheet("background: transparent;");
    bottomSpacer->setObjectName("bottomSpacer");

    bottomLayout->addWidget(m_counterLabel);
    bottomLayout->addWidget(bottomSpacer);
    bottomLayout->addWidget(m_requestInfoLabel);

    // -------------------------------------------------------- Other --------------------------------------------------------

    mainLayout->addWidget(m_topBar);
    mainLayout->addWidget(m_stackedContent);
    mainLayout->addWidget(m_bottomBar);

    m_logPanel = new LogPanel(this);
    m_logPanel->show();

    m_requestInfoMenu = new RequestInfoMenu(this);
    m_requestInfoMenu->hide();

    m_fileListMenu = new FileListMenu(this);
    m_fileListMenu->hide();

    m_screenshotTool = new ScreenshotTool(this);

    setupSettings();

    connect(m_toggleModeButton, &QPushButton::clicked,          this, &MarkdownViewer::toggleMode);
    connect(m_cropButton,       &QPushButton::clicked,          this, &MarkdownViewer::onCropButtonClicked);
    connect(m_settingsButton,   &QPushButton::clicked,          this, &MarkdownViewer::showSettings);
    connect(m_fileListMenu,     &FileListMenu::itemSelected,    this, &MarkdownViewer::fileMenuItemClicked);
    connect(m_requestInfoMenu,  &RequestInfoMenu::itemSelected, this, &MarkdownViewer::setRequestInfo);

    connect(m_counterLabel, &InteractiveLabel::clicked, this, [this]() {
        if (m_fileListMenu->isHidden()) {
            QPoint pos = m_counterLabel->mapToGlobal(QPoint(0, 0));
            pos.setY(pos.y() - m_fileListMenu->height() - 5);
            m_fileListMenu->showAt(pos);
        } else {
            m_fileListMenu->hide();
        }
    });

    connect(m_requestInfoLabel, &InteractiveLabel::clicked, this, [this]() {
        InteractiveLabel* label = qobject_cast< InteractiveLabel* >(sender());
        QPoint pos = mapToGlobal(m_requestInfoLabel->pos());
        pos.setY(pos.y() + m_requestInfoLabel->height());

        qTrace() << "Request info label item selected " << label->text();

        if (m_requestInfoMenu->isHidden()) {
            m_requestInfoMenu->showAt(pos);
        } else {
            m_requestInfoMenu->hide();
        }
    });

    connect(m_chatWindow,     &ChatWindow::filesChosen,              this, &MarkdownViewer::filesChosen);
    connect(m_chatWindow,     &ChatWindow::messageSent,              this, &MarkdownViewer::messageSent);
    connect(m_chatWindow,     &ChatWindow::messageRemoved,           this, &MarkdownViewer::messageRemoved);
    connect(m_chatWindow,     &ChatWindow::screenshotPreviewClicked, this, &MarkdownViewer::screenshotPreviewClicked);

    connect(m_screenshotTool, &ScreenshotTool::screenshotTaken,      this, &MarkdownViewer::onScreenshotReady);
    connect(m_screenshotTool, &ScreenshotTool::cancelled,            this, &MarkdownViewer::onScreenshotCancelled);
}
