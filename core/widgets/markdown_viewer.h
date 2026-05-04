#pragma once

#include <QWidget>
#include <QList>

#include "../common.h"

#include "overlay_widget.h"


QT_FORWARD_DECLARE_CLASS(QTextBrowser)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(Settings)
QT_FORWARD_DECLARE_CLASS(QSyntaxHighlighter)
QT_FORWARD_DECLARE_CLASS(QTextDocument)
QT_FORWARD_DECLARE_CLASS(QHBoxLayout)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QStackedWidget)
QT_FORWARD_DECLARE_CLASS(InteractiveLabel)
QT_FORWARD_DECLARE_CLASS(FileListMenu)
QT_FORWARD_DECLARE_CLASS(RequestInfoMenu)
QT_FORWARD_DECLARE_CLASS(RequestInfoItem)
QT_FORWARD_DECLARE_CLASS(LogPanel)
QT_FORWARD_DECLARE_CLASS(ChatWindow)
QT_FORWARD_DECLARE_CLASS(ScreenshotTool)


namespace QSourceHighlite
{
    class QSourceHighliter;
}


class MarkdownViewer : public OverlayWidget
{
    Q_OBJECT
public:
    enum Mode {
        Mode_SinglePage,
        Mode_Chat
    };

public:
    MarkdownViewer(QWidget *parent = nullptr);
    ~MarkdownViewer() override;

public slots:
    void setLastOperationStatus(OperationStatus status);
    OperationStatus getLastOperationStatus() const;

    void showSettings();
    void switchMode(Mode mode);
    void toggleMode();
    void clearText();
    void clearChatMessages();
    void clearChatPreviews();
    int  appendChatMessage(const ChatMessage& msg);
    void addScreenshotPreview(const QPixmap& screenshot);
    bool deleteScreenshotPreview(int index = -1);
    void takeCroppedScreenshot();
    void addFileMenuItem(const QString& fileName);
    bool deleteFileMenuItem(const QString& fileName);
    void resetSettings();
    void resetSettings(const SettingsView& data);

    void setMarkdownText(const QString& text);
    void setCounter(int value);
    void setRequestInfo(const ServiceRequestLookUpData& item);
    void setRequestsData(const QList< ServiceRequestLookUpData >& items);
    void setMoveStep(QPair< int, int > moveStep);
    void incProxySuccessCounter(QStringView proxyName, int delta);
    void incProxyFailedCounter(QStringView proxyName, int delta);

    bool isEmpty();
    int  counter();
    OptionalRef< ChatMessage > findChatMessage(qint64 id) const;
    ServiceRequestLookUpData requestInfo() const;
    const QPair< int, int >& moveStep() const;
    qsizetype screenshotPreviewsSize() const;
    qsizetype fileMenuListSize() const;

    bool handleAction(ActionType action);

signals:
    void statusChanged(OperationStatus oldStatus, OperationStatus newStatus);

    void filesChosen(const QStringList& filePaths);

    void messageSent(const QString& prompt);
    void messageRemoved(const ChatMessage& message);

    void screenshotPreviewClicked(int index);
    void screenshotReady(const QPixmap& screenshot);
    void screenshotCancelled();

    void fileMenuItemClicked(const QString& fileName);

    void settingsOkClicked(const SettingsView& data);
    void settingsApplyClicked(const SettingsView& data);
    void settingsSaveClicked(const SettingsView& data);
    void settingsCancelClicked();

private slots:
    void onCropButtonClicked();
    void onScreenshotReady(const QPixmap& screenshot);
    void onScreenshotCancelled();

private:
    void setupSingePageWindow();
    void setupChatWindow();
    void setupSettings();
    void setupUi();

    // top bar
    QWidget*            m_topBar;
    QLabel*             m_statusIndicator;
    QPushButton*        m_toggleModeButton;
    QPushButton*        m_cropButton;
    QPushButton*        m_settingsButton;
    Settings*           m_settingsDialog;

    // chat panel
    QStackedWidget*     m_stackedContent;;
    ChatWindow*         m_chatWindow;
    QTextBrowser*       m_textBrowser;

    // bottom bar
    QWidget*            m_bottomBar;
    InteractiveLabel*   m_counterLabel;
    InteractiveLabel*   m_requestInfoLabel;
    FileListMenu*       m_fileListMenu;
    RequestInfoMenu*    m_requestInfoMenu;
    LogPanel*           m_logPanel;

    // other
    Mode                         m_currentMode;
    ScreenshotTool*              m_screenshotTool;
    QPair< int, int >            m_moveStep;
    ServiceRequestLookUpData     m_requestLabelData;
    OperationStatus              m_lastOperationStatus;
    QSourceHighlite::QSourceHighliter* m_highlighter;
};

