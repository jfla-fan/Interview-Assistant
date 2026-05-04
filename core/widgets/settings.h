#pragma once

#include <QDialog>

#include "../common.h"

#include "../config/app_config_fwd.h"
#include "../config/service_config_fwd.h"

#include "base_widget.h"


QT_FORWARD_DECLARE_CLASS(BaseWidgetImpl)
QT_FORWARD_DECLARE_CLASS(QTabWidget)
QT_FORWARD_DECLARE_CLASS(QDialogButtonBox)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QTableWidget)
QT_FORWARD_DECLARE_CLASS(QTableWidgetItem)
QT_FORWARD_DECLARE_CLASS(QTextEdit)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QSpinBox)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(ShowEvent)
QT_FORWARD_DECLARE_CLASS(ProxyTableWidget)
QT_FORWARD_DECLARE_CLASS(HotkeyTableWidget)
QT_FORWARD_DECLARE_CLASS(ServiceConfigTextEdit)


class Settings : public QDialog, public IBaseWidget
{
    Q_OBJECT
    Q_INTERFACES(IBaseWidget)

public:
    explicit Settings(AppConfig::ConstConfigPtr appConfig,
                      ServiceConfig::ConstConfigPtr serviceConfig,
                      QWidget *parent = nullptr,
                      bool excludeFromCapture = false);

    ~Settings() override;

public slots:
    // UI changes
    void setUiAppConfig(AppConfig::ConstConfigPtr config);
    void setUiServiceConfigRaw(const QString& data);
    void setUiServiceConfig(ServiceConfig::ConstConfigPtr config);

    // Reset with original data
    void resetAppConfig();
    void resetServiceConfig();
    void resetAll();

    // Reset with new data
    void resetAll(const SettingsView& newSettings);

public:
    void setLogLine(const QString& message);
    void setConfigFolderPath(const QString& path);
    void incProxySuccessCounter(QStringView proxyName, int delta);
    void incProxyFailedCounter(QStringView proxyName, int delta);

    // IBaseWidget
    void moveBy(int dx, int dy, bool recursive = true) override;
    bool setExcludeFromCapture(bool value, bool recursive = true) override;
    bool isExcludedFromCapture() const override;

signals:
    void okClicked(const SettingsView& data);
    void applyClicked(const SettingsView& data);
    void saveClicked(const SettingsView& data);
    void cancelClicked();

protected:
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
    bool eventFilter(QObject* obj, QEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    void onResetGeneral();
    void onResetLogging();
    void onResetProxies();
    void onResetHotkeys();
    void onResetServiceConfig();
    void onResetAllTabs();

    void onCheckServiceConfig();
    void onServiceConfigLoaderStatusChanged(OperationStatus status);

    void onShowProxyContextMenu(const QPoint &pos);
    void onShowHotkeyContextMenu(const QPoint& pos);
    void onShowTabBarContextMenu(const QPoint& globalPos, int tabIndex);

private:
    void setupUi();

    QWidget* createGeneralTab();
    QWidget* createLoggingTab();
    QWidget* createProxiesTab();
    QWidget* createHotkeysTab();
    QWidget* createServiceConfigTab();

    // @todo encapsulate proxy table functions
    void populateAppUi();
    void populateServiceUi();
    void updateTabTitles();
    void setProxiesCellDirty(int row, int column, bool isDirty);
    void updateProxiesDirty();
    void markNewProxyRow(int row, bool isNew = true);
    void setEnabledProxiesRow(int row, bool enabled);
    void insertProxiesRow(int row, int originalIndex = -1);
    void removeProxiesRow(int row);
    void resetProxiesRow(int row);
    void updateHotkeysDirty();
    SettingsView retrieveSettings();

    // --- UI ---
    struct GeneralControls {
        QCheckBox* excludeFromCapture;
        QCheckBox* saveScreenshots;
    } m_generalControls;

    struct LoggingControls {
        QCheckBox* asyncMode;
        QSpinBox*  maxQueueSize;
        QComboBox* minLevel;
    } m_loggingControls;

    struct ProxiesControls {
        ProxyTableWidget* proxiesTable;
        int dirtyCellsCount = 0;
    } m_proxiesControls;

    struct HotkeysControls {
        int hotkeyChangesCount = 0;
        int invalidHotkeysCount = 0;
        HotkeyTableWidget* hotkeysTable;
    } m_hotkeysControls;

    struct ServiceConfigControls {
        ServiceConfigTextEdit* serviceConfigTextEdit;
        QPushButton* checkButton;
    } m_serviceConfigControls;

    // --- State ---
    AppConfig::ConfigPtr m_originalAppConfig;
    AppConfig::ConfigPtr m_currentAppConfig;

    ServiceConfig::ConstConfigPtr m_originalServiceConfig;
    ServiceConfig::ConfigLoader* m_serviceConfigLoader;

    // --- Other ---
    std::unique_ptr< BaseWidgetImpl > m_baseWidgetImpl;
    QString           m_configFolderPath;
    QTabWidget*       m_tabWidget;
    QDialogButtonBox* m_buttonBox;
    QLabel*           m_statusLabel;
};
