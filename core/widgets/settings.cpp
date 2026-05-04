#include "settings.h"
#include "overlay_menu.h"

#include "private/proxy_table_widget.h"
#include "private/hotkey_table_widget.h"
#include "private/service_config_text_edit.h"
#include "private/base_widget_impl.h"

#include "log/log.h"

#include "config/app_config.h"
#include "config/service_config.h"
#include "config/service_config_loader.h"

#include "utils/containers.h"
#include "utils/style.h"

#include "network/proxy_checker.h"

#ifdef Q_OS_WIN
    #include <qt_windows.h>
#endif

#include <QTabWidget>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QTimer>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QMessageBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QKeySequenceEdit>
#include <QTextEdit>
#include <QLineEdit>
#include <QFontDatabase>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QStringLiteral>
#include <QDir>
#include <QDesktopServices>
#include <QShowEvent>
#include <QTabBar>
#include <QPainter>
#include <QStyledItemDelegate>



Settings::Settings(AppConfig::ConstConfigPtr appConfig,
                   ServiceConfig::ConstConfigPtr serviceConfig,
                   QWidget *parent,
                   bool excludeFromCapture)
    : QDialog(parent, Qt::WindowFlags(Qt::WindowStaysOnTopHint))
    , m_baseWidgetImpl(new BaseWidgetImpl(this))
    , m_serviceConfigLoader(new ServiceConfig::ConfigLoader(this))
{
    Q_ASSERT_X(appConfig, "Settings::Settings", "Application config must be provided. Use default configuration if cannot load user configuration.");

    setConfigFolderPath(QFileInfo(appConfig->path).dir().absolutePath());
    connect(m_serviceConfigLoader, &ServiceConfig::ConfigLoader::statusChanged, this, &Settings::onServiceConfigLoaderStatusChanged);

    setupUi();
    resetAll(SettingsView::create(appConfig, serviceConfig));

    m_baseWidgetImpl->initialize("Settings", excludeFromCapture);
}


Settings::~Settings() = default;


void Settings::setUiAppConfig(AppConfig::ConstConfigPtr config)
{
    if (!config) return;

    if (!m_currentAppConfig) m_currentAppConfig = config->clone();
    else *m_currentAppConfig = *config;

    populateAppUi();
}


void Settings::setUiServiceConfigRaw(const QString& data)
{
    m_serviceConfigLoader->reset(nullptr);
    m_serviceConfigControls.serviceConfigTextEdit->setText(data);

    populateServiceUi();
}


void Settings::setUiServiceConfig(ServiceConfig::ConstConfigPtr config)
{
    m_serviceConfigLoader->reset(config);

    populateServiceUi();
}


void Settings::resetAppConfig()
{
    setUiAppConfig(m_originalAppConfig);
    setLogLine("App Config has been reset");
}


void Settings::resetServiceConfig()
{
    setUiServiceConfig(m_originalServiceConfig);
    setLogLine(QStringLiteral("Service config has been reset"));
}


void Settings::resetAll()
{
    resetAppConfig();
    resetServiceConfig();
    setLogLine(QStringLiteral("Config has been reset"));
}


void Settings::resetAll(const SettingsView& newSettings)
{
    if (newSettings.ServicesSettings.loadedConfig) {
        m_originalServiceConfig = newSettings.ServicesSettings.loadedConfig;
        resetServiceConfig();
    }
    else if (newSettings.ServicesSettings.rawConfig) {
        setUiServiceConfigRaw(*newSettings.ServicesSettings.rawConfig);
    }

    if (newSettings.AppSettings.config) {
        m_originalAppConfig = newSettings.AppSettings.config->clone();
        resetAppConfig();
    }

    setLogLine(QStringLiteral("Config has been reset"));
}


void Settings::setLogLine(const QString& message)
{
    m_statusLabel->setText(message);
}


void Settings::setConfigFolderPath(const QString& path)
{
    QFileInfo info(path);
    if (!info.isDir()) {
        LOG_WARNING("Cannot set config folder path in settings, {} is not a directory.", path);
        return;
    }

    m_configFolderPath = path;
}


void Settings::incProxySuccessCounter(QStringView proxyName, int delta)
{
    int row = m_proxiesControls.proxiesTable->rowByName(proxyName);
    if (row >= 0)
        m_proxiesControls.proxiesTable->incSuccessItemCount(row, delta);
    else
        LOG_WARNING("Failed to find proxy {} in settings table", proxyName);
}


void Settings::incProxyFailedCounter(QStringView proxyName, int delta)
{
    int row = m_proxiesControls.proxiesTable->rowByName(proxyName);
    if (row >= 0)
        m_proxiesControls.proxiesTable->incFailedItemCount(row, delta);
    else
        LOG_WARNING("Failed to find proxy {} in settings table", proxyName);
}


void Settings::moveBy(int dx, int dy, bool recursive)
{
    m_baseWidgetImpl->moveBy(dx, dy, recursive);
}


bool Settings::setExcludeFromCapture(bool value, bool recursive)
{
    return m_baseWidgetImpl->setExcludeFromCapture(value, recursive);
}


bool Settings::isExcludedFromCapture() const
{
    return m_baseWidgetImpl->isExcludedFromCapture();
}


bool Settings::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    // Suppress popping up the system menu on Windows
#ifdef Q_OS_WIN
    if (eventType == QStringLiteral("windows_generic_MSG"))
    {
        MSG* msg = static_cast< MSG* >(message);

        // WM_SYSCOMMAND && SC_MOUSEMENU - left-click on the settings icon.
        // WM_NCRBUTTONUP - right-click on a non-client area button or border.
        // WM_CONTEXTMENU - right-click on a non-client empty area.
        // WM_SYSCHAR - Alt + <AnyKey>. Protect against Alt+Space pop-up call.
        if (msg->message == WM_SYSCOMMAND && GET_SC_WPARAM(msg->wParam) == SC_MOUSEMENU  ||
            msg->message == WM_NCRBUTTONUP ||
            msg->message == WM_CONTEXTMENU ||
            msg->message == WM_SYSCHAR)
        {
            *result = 0;
            return true;
        }
    }
#endif

    return QDialog::nativeEvent(eventType, message, result);
}


bool Settings::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == m_tabWidget->tabBar() && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = static_cast< QMouseEvent* >(event);
        if (mouseEvent->button() == Qt::RightButton) {
            QTabBar* tabBar = m_tabWidget->tabBar();
            int tabIndex = tabBar->tabAt(mouseEvent->pos());
            if (tabIndex >= 0) {
                QPoint globalPos = tabBar->mapToGlobal(mouseEvent->pos());
                onShowTabBarContextMenu(globalPos, tabIndex);
                return true;
            }
        }
    }

    return QDialog::eventFilter(obj, event);
}


void Settings::showEvent(QShowEvent* event)
{
    /*
     * Not sure how it works under the hood, but reopening "Settings" window
     * causes it to become "visible" again, so we have to undo it here.
     */
    if (m_originalAppConfig->excludeFromCapture) {
        setExcludeFromCapture(true);
    }

    QDialog::showEvent(event);
}


void Settings::contextMenuEvent(QContextMenuEvent *event)
{
    auto* contextMenu = new OverlayMenu(tr("SettingsMenu"), this);
    contextMenu->setAttribute(Qt::WA_DeleteOnClose);

    QAction* resetAllTablsAction = contextMenu->addAction("Reset All Tabs");
    QAction* openConfigFolderAction = contextMenu->addAction("Open Config Folder");
    QAction* resetAppConfigAction = contextMenu->addAction("Reset App Config");
    QAction* resetServiceConfigAction = contextMenu->addAction("Reset Service Config");

    connect(resetAllTablsAction, &QAction::triggered, this, &Settings::onResetAllTabs);

    connect(openConfigFolderAction, &QAction::triggered, this, [this](bool) {
        if (!QDesktopServices::openUrl(m_configFolderPath)) {
            LOG_ERROR("Failed to open folder at {}", m_configFolderPath);
        }
    });

    connect(resetAppConfigAction, &QAction::triggered, this, &Settings::resetAppConfig);
    connect(resetServiceConfigAction, &QAction::triggered, this, &Settings::resetServiceConfig);

    contextMenu->popup(event->globalPos());
    event->accept();
}


void Settings::setupUi()
{
    setWindowTitle("Settings");
    setMinimumSize(600, 550);
    setWindowIcon(QIcon(":/icons/settings.png"));

    utils::SetWidgetStyleSheet(this, ":/styles/settings.style");

    auto* mainLayout = new QVBoxLayout(this);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(createGeneralTab(), "General");
    m_tabWidget->addTab(createLoggingTab(), "Logging");
    m_tabWidget->addTab(createProxiesTab(), "Proxies");
    m_tabWidget->addTab(createHotkeysTab(), "Hotkeys");
    m_tabWidget->addTab(createServiceConfigTab(), "Service Config");

    auto* tabBar = m_tabWidget->tabBar();
    tabBar->installEventFilter(this);

    m_statusLabel = new QLabel("Ready.", this);
    m_statusLabel->setStyleSheet("color: gray;");

    m_buttonBox = new QDialogButtonBox(this);
    auto* okButton = m_buttonBox->addButton(QDialogButtonBox::Ok);
    auto* cancelButton = m_buttonBox->addButton(QDialogButtonBox::Cancel);
    auto* applyButton = m_buttonBox->addButton(QDialogButtonBox::Apply);
    auto* saveButton = m_buttonBox->addButton("Save", QDialogButtonBox::AcceptRole);

    connect(okButton, &QPushButton::clicked, this, [this]() {
        accept();
        emit okClicked(retrieveSettings());
    });

    connect(cancelButton, &QPushButton::clicked, this, [this]() {
        reject();
        emit cancelClicked();
    });

    connect(applyButton, &QPushButton::clicked, this, [this]() {
        emit applyClicked(retrieveSettings());
    });

    connect(saveButton, &QPushButton::clicked, this, [this]() {
        emit saveClicked(retrieveSettings()); }
    );

    mainLayout->addWidget(m_tabWidget);
    mainLayout->addWidget(m_statusLabel);
    mainLayout->addWidget(m_buttonBox);
}


void Settings::populateAppUi()
{
    // General Tab
    {
        QSignalBlocker bl1(m_generalControls.excludeFromCapture);
        m_generalControls.excludeFromCapture->setChecked(m_currentAppConfig->excludeFromCapture);
        ::SetWidgetDirty(m_generalControls.excludeFromCapture, m_originalAppConfig->excludeFromCapture != m_currentAppConfig->excludeFromCapture);

        QSignalBlocker bl2(m_generalControls.saveScreenshots);
        m_generalControls.saveScreenshots->setChecked(m_currentAppConfig->saveScreenshots);
        ::SetWidgetDirty(m_generalControls.saveScreenshots, m_originalAppConfig->saveScreenshots != m_currentAppConfig->saveScreenshots);
    }

    // Logging Tab
    {
        QSignalBlocker bl1(m_loggingControls.asyncMode);
        m_loggingControls.asyncMode->setChecked(m_currentAppConfig->asyncMode);
        ::SetWidgetDirty(m_loggingControls.asyncMode, m_originalAppConfig->asyncMode != m_currentAppConfig->asyncMode);

        QSignalBlocker bl2(m_loggingControls.maxQueueSize);
        m_loggingControls.maxQueueSize->setValue(m_currentAppConfig->maxQueueSize);
        ::SetWidgetDirty(m_loggingControls.maxQueueSize, m_originalAppConfig->maxQueueSize != m_currentAppConfig->maxQueueSize);

        QSignalBlocker bl3(m_loggingControls.minLevel);
        m_loggingControls.minLevel->setCurrentIndex(static_cast< int >(m_currentAppConfig->minLevel));
        ::SetWidgetDirty(m_loggingControls.minLevel, m_originalAppConfig->minLevel != m_currentAppConfig->minLevel);
    }

    // Proxies Tab
    {
        QSignalBlocker bl(m_proxiesControls.proxiesTable);

        m_proxiesControls.dirtyCellsCount = 0;
        m_proxiesControls.proxiesTable->setRowCount(0);
        for (int i = 0; i < m_currentAppConfig->proxies.size(); ++i) {
            m_proxiesControls.proxiesTable->insertProxy(i, i + 1, i, m_currentAppConfig->proxies[i]);
        }

        updateProxiesDirty();
    }

    // Hotkeys tab
    {
        QSignalBlocker bl(m_hotkeysControls.hotkeysTable);

        m_hotkeysControls.hotkeyChangesCount = 0;
        m_hotkeysControls.invalidHotkeysCount = 0;
        m_hotkeysControls.hotkeysTable->setHotkeys(m_currentAppConfig->hotkeys);

        updateHotkeysDirty();
    }
}


void Settings::populateServiceUi()
{
    auto*& textEdit = m_serviceConfigControls.serviceConfigTextEdit;

    QSignalBlocker bl(textEdit);

    if (m_serviceConfigLoader->lastValidConfig())
        textEdit->setText(m_serviceConfigLoader->lastValidConfig()->originalText());
    else if (m_originalServiceConfig)
        textEdit->setText(m_originalServiceConfig->originalText());
    else
        textEdit->clear();

    ::SetPropertyAndUpdateUi(textEdit, "checkStatus", "Idle");

    updateTabTitles();
}


void Settings::setProxiesCellDirty(int row, int column, bool isDirty)
{
    LOG_TRACE("Set table cell dirty, row({}), column({}), isDirty({}), is cell widget: {}, is item: {}",
              row, column, isDirty,
              m_proxiesControls.proxiesTable->cellWidget(row, column) != nullptr,
              m_proxiesControls.proxiesTable->item(row, column) != nullptr);

    m_proxiesControls.proxiesTable->setDirty(row, column, isDirty);

    const int delta = static_cast< int >(isDirty) * 2 - 1;

    m_proxiesControls.dirtyCellsCount += delta;
    m_proxiesControls.proxiesTable->incChangesCount(row, delta);

    updateProxiesDirty();
}


void Settings::updateProxiesDirty()
{
    if (m_proxiesControls.dirtyCellsCount != 0 ||
        m_originalAppConfig->proxies != m_currentAppConfig->proxies) {
        m_proxiesControls.proxiesTable->setProperty("isDirty", true);
    } else {
        m_proxiesControls.proxiesTable->setProperty("isDirty", false);
    }

    updateTabTitles();
}


void Settings::markNewProxyRow(int row, bool isNew)
{
    setProxiesCellDirty(row, -1, true); // header
    for (int column = 0; column < m_proxiesControls.proxiesTable->columnCount(); ++column)
        setProxiesCellDirty(row, column, true);
}


void Settings::setEnabledProxiesRow(int row, bool enabled)
{
    auto* originalProxy = utils::ValueAtOrNullptr(m_originalAppConfig->proxies, m_proxiesControls.proxiesTable->originalIndex(row));
    bool wasEnabled = m_currentAppConfig->proxies[row].Enabled;

    m_currentAppConfig->proxies[row].Enabled = enabled;
    m_proxiesControls.proxiesTable->setEnabled(row, enabled);

    if (originalProxy && wasEnabled != enabled) {
        const int delta = 2 * static_cast< int >(originalProxy->Enabled != enabled) - 1;

        m_proxiesControls.proxiesTable->incChangesCount(row, delta);
        m_proxiesControls.dirtyCellsCount += delta;

        updateProxiesDirty();
    }
}


void Settings::insertProxiesRow(int row, int originalIndex)
{
    int displayRow = 1;
    for (int r = 0; r < m_proxiesControls.proxiesTable->rowCount(); ++r)
        displayRow = qMax(displayRow, m_proxiesControls.proxiesTable->displayNumber(r) + 1);

    ServiceProxy newProxy;
    newProxy.Name = QString("New Proxy %1").arg(displayRow);
    newProxy.IgnoreSslErrors = true;
    newProxy.Enabled = true;
    newProxy.NetworkProxy = *::ParseProxyFromEndpoint("127.0.0.1:8080");

    m_currentAppConfig->proxies.insert(row, std::move(newProxy));

    QSignalBlocker bl(m_proxiesControls.proxiesTable);
    m_proxiesControls.proxiesTable->insertProxy(row, displayRow, originalIndex, m_currentAppConfig->proxies[row]);
    markNewProxyRow(row);

    m_proxiesControls.proxiesTable->editItem(m_proxiesControls.proxiesTable->item(row, 1));
}


void Settings::removeProxiesRow(int row)
{
    m_currentAppConfig->proxies.removeAt(row);
    m_proxiesControls.dirtyCellsCount -= m_proxiesControls.proxiesTable->changesCount(row);
    m_proxiesControls.proxiesTable->removeProxy(row);
    updateProxiesDirty();
}


void Settings::resetProxiesRow(int row)
{
    auto* originalProxy = utils::ValueAtOrNullptr(m_originalAppConfig->proxies, m_proxiesControls.proxiesTable->originalIndex(row));
    if (originalProxy) {
        // QSignalBlocker bl(m_proxiesControls.proxiesTable);
        // m_proxiesControls.dirtyCellsCount -= m_proxiesControls.proxiesTable->changesCount(row);

        setEnabledProxiesRow(row, originalProxy->Enabled);
        m_proxiesControls.proxiesTable->proxyChecker(row)->setProxy(*originalProxy);
        m_proxiesControls.proxiesTable->resetProxy(row, *originalProxy);

        updateProxiesDirty();
    }
}


void Settings::updateHotkeysDirty()
{
    if (m_hotkeysControls.hotkeyChangesCount != 0) {
        m_hotkeysControls.hotkeysTable->setProperty("isDirty", true);
    } else {
        m_hotkeysControls.hotkeysTable->setProperty("isDirty", false);
    }

    updateTabTitles();
}


SettingsView Settings::retrieveSettings()
{
    if (m_hotkeysControls.invalidHotkeysCount == 0) {
        m_currentAppConfig->hotkeys = m_hotkeysControls.hotkeysTable->collectHotkeys();
    }
    else {
        LOG_WARNING("There're {} invalid hotkeys, defaulting to original config hotkeys.", m_hotkeysControls.invalidHotkeysCount);
    }

    auto lastValidUiConfig = m_serviceConfigLoader->lastValidConfig();
    auto uiText = m_serviceConfigControls.serviceConfigTextEdit->toPlainText();

    if (lastValidUiConfig && lastValidUiConfig->originalText() == uiText) {
        // user checked syntax and hasn't changed config since then
        return SettingsView::create(m_currentAppConfig, lastValidUiConfig);
    }
    else if (m_originalServiceConfig && uiText == m_originalServiceConfig->originalText()) {
        // config text is basically the original config
        return SettingsView::create(m_currentAppConfig, m_originalServiceConfig);
    }

    return SettingsView::create(m_currentAppConfig, uiText);
}


void Settings::updateTabTitles()
{
    bool isGeneralDirty = m_generalControls.excludeFromCapture->property("isDirty").toBool() ||
                          m_generalControls.saveScreenshots->property("isDirty").toBool();
    m_tabWidget->setTabText(0, QStringLiteral("General") + (isGeneralDirty ? " *" : ""));

    bool isLoggingDirty = m_loggingControls.asyncMode->property("isDirty").toBool() ||
                          m_loggingControls.maxQueueSize->property("isDirty").toBool() ||
                          m_loggingControls.minLevel->property("isDirty").toBool();
    m_tabWidget->setTabText(1, QStringLiteral("Logging") + (isLoggingDirty ? " *" : ""));

    bool isProxiesDirty = m_proxiesControls.proxiesTable->property("isDirty").toBool();
    m_tabWidget->setTabText(2, QStringLiteral("Proxies") + (isProxiesDirty ? " *" : ""));

    bool isHotkeysDirty = m_hotkeysControls.hotkeysTable->property("isDirty").toBool();
    m_tabWidget->setTabText(3, QStringLiteral("Hotkeys") + (isHotkeysDirty ? " *" : ""));

    bool isServiceConfigDirty = m_serviceConfigControls.serviceConfigTextEdit->property("checkStatus").toString() == QStringLiteral("Changed");
    m_tabWidget->setTabText(4, QStringLiteral("Service Config") + (isServiceConfigDirty ? " *" : ""));
}


QWidget* Settings::createGeneralTab()
{
    auto* widget = new QWidget();
    auto* layout = new QFormLayout(widget);
    auto* resetButton = new QPushButton("Reset");

    m_generalControls.excludeFromCapture = new QCheckBox("Exclude application from screen capture");
    m_generalControls.saveScreenshots = new QCheckBox("Save screenshots automatically");

    connect(resetButton, &QPushButton::clicked, this, &Settings::onResetGeneral);

    connect(m_generalControls.excludeFromCapture, &QCheckBox::toggled, this, [this](bool checked) {
        m_currentAppConfig->excludeFromCapture = checked;
        ::SetWidgetDirty(m_generalControls.excludeFromCapture, checked != m_originalAppConfig->excludeFromCapture);
        updateTabTitles();
    });

    connect(m_generalControls.saveScreenshots, &QCheckBox::toggled, this, [this](bool checked) {
        m_currentAppConfig->saveScreenshots = checked;
        ::SetWidgetDirty(m_generalControls.saveScreenshots, checked != m_originalAppConfig->saveScreenshots);
        updateTabTitles();
    });

    layout->addRow("Stealth Mode:", m_generalControls.excludeFromCapture);
    layout->addRow("Screenshots:", m_generalControls.saveScreenshots);
    layout->addRow(resetButton);

    return widget;
}


QWidget* Settings::createLoggingTab()
{
    auto* widget = new QWidget();
    auto* layout = new QFormLayout(widget);
    auto* resetButton = new QPushButton("Reset");

    m_loggingControls.asyncMode = new QCheckBox("Enable asynchronous logging");
    m_loggingControls.maxQueueSize = new QSpinBox();
    m_loggingControls.maxQueueSize->setRange(1000, 100000);
    m_loggingControls.minLevel = new QComboBox();

    for (const auto& level : magic_enum::enum_values< LogLevel >()) {
        if (level == LogLevel_Max) continue;
        m_loggingControls.minLevel->addItem(ToQString(level).remove("LogLevel_"), QVariant::fromValue(level));
    }

    connect(resetButton, &QPushButton::clicked, this, &Settings::onResetLogging);

    connect(m_loggingControls.asyncMode, &QCheckBox::toggled, this, [this](bool checked) {
        m_currentAppConfig->asyncMode = checked;
        ::SetWidgetDirty(m_loggingControls.asyncMode, checked != m_originalAppConfig->asyncMode);
        updateTabTitles();
    });

    connect(m_loggingControls.maxQueueSize, &QSpinBox::valueChanged, this, [this](int newValue) {
        m_currentAppConfig->maxQueueSize = newValue;
        ::SetWidgetDirty(m_loggingControls.maxQueueSize, newValue != m_originalAppConfig->maxQueueSize);
        updateTabTitles();
    });

    connect(m_loggingControls.minLevel, &QComboBox::currentIndexChanged, this, [this](int newIndex) {
        m_currentAppConfig->minLevel = static_cast< LogLevel >(newIndex);
        ::SetWidgetDirty(m_loggingControls.minLevel, m_currentAppConfig->minLevel != m_originalAppConfig->minLevel);
        updateTabTitles();
    });

    layout->addRow("Performance:", m_loggingControls.asyncMode);
    layout->addRow("Max Queue Size:", m_loggingControls.maxQueueSize);
    layout->addRow("Minimum Log Level:", m_loggingControls.minLevel);
    layout->addRow(resetButton);

    return widget;
}


QWidget* Settings::createProxiesTab()
{
    auto* widget = new QWidget();

    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);

    auto*& proxiesTable = m_proxiesControls.proxiesTable;

    proxiesTable = new ProxyTableWidget(this);
    proxiesTable->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(proxiesTable, &QWidget::customContextMenuRequested, this, &Settings::onShowProxyContextMenu);

    connect(proxiesTable, &ProxyTableWidget::cellMoved, [this, proxiesTable](int prevRow, int currRow) {
        const int insertPos = qBound(0, currRow - (prevRow < currRow), proxiesTable->rowCount());
        LOG_DEBUG("Prev row - {}, currRow - {}, insertPos - {}", prevRow, currRow, insertPos);

        auto proxy = m_currentAppConfig->proxies.takeAt(prevRow);

        if (insertPos == proxiesTable->rowCount()) m_currentAppConfig->proxies.append(std::move(proxy));
        else m_currentAppConfig->proxies.insert(insertPos, std::move(proxy));

        updateProxiesDirty();
    });

    connect(proxiesTable, &ProxyTableWidget::cellChanged, [this, proxiesTable](int row, int column) {
        auto& proxy = m_currentAppConfig->proxies[row];
        auto* originalProxy = utils::ValueAtOrNullptr(m_originalAppConfig->proxies, proxiesTable->originalIndex(row));

        LOG_TRACE("Proxy table widget cell changed({}, {}, {})", row, column, proxiesTable->originalIndex(row));

        switch(column)
        {
            case 1:
            {
                auto* item = proxiesTable->item(row, column);
                proxy.Name = item->text();

                if (originalProxy)
                    setProxiesCellDirty(row, column, originalProxy->Name != proxy.Name);

                break;
            }

            case 2:
            {
                auto* item = proxiesTable->item(row, column);
                if (auto netProxy = ::ParseProxyFromEndpoint(item->text())) {
                    proxy.NetworkProxy = *netProxy;

                    if (originalProxy)
                        setProxiesCellDirty(row, column, originalProxy->NetworkProxy != proxy.NetworkProxy);

                    break;
                }

                LOG_ERROR("Failed to parse QNetworkProxy from table widget entry({}, {}) with text: {}.", item->row(), item->column(), item->text());
                break;
            }

            case 3:
            {
                auto* checkBox = proxiesTable->cell< QCheckBox* >(row, column);
                proxy.IgnoreSslErrors = checkBox->isChecked();

                if (originalProxy)
                    setProxiesCellDirty(row, column, originalProxy->IgnoreSslErrors != proxy.IgnoreSslErrors);

                break;
            }

            default:
                break;
        }
    });

    auto* toolbar = new QWidget();
    toolbar->setStyleSheet(R"(
        QWidget {
            background-color: rgb(55, 55, 55);
            border-top: 1px solid #444;
        }

        QPushButton {
            font-weight: bold;
            font-size: 12pt;
            padding: 2px 10px;
        }
    )");

    auto* buttonLayout = new QHBoxLayout(toolbar);
    buttonLayout->setContentsMargins(5, 5, 5, 5);

    auto* addButton = new QPushButton(QStringLiteral("＋ Add"), this);
    auto* removeButton = new QPushButton(QStringLiteral("－ Remove"), this);
    auto* resetButton = new QPushButton(QStringLiteral("⟳ Reset"), this);

    connect(addButton, &QPushButton::clicked, this, [this]() {
        insertProxiesRow(m_currentAppConfig->proxies.count());
    });

    connect(removeButton, &QPushButton::clicked, this, [this]() {
        int currentRow = m_proxiesControls.proxiesTable->currentRow();
        if (currentRow < 0) {
            LOG_WARNING("Nothing is selected. Please, select a row to remove");
            return;
        }

        removeProxiesRow(currentRow);
    });

    connect(resetButton, &QPushButton::clicked, this, &Settings::onResetProxies);

    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(removeButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(resetButton);

    layout->addWidget(proxiesTable);
    layout->addWidget(toolbar);

    return widget;
}


QWidget* Settings::createHotkeysTab()
{
    auto*& hotkeysTable = m_hotkeysControls.hotkeysTable;

    hotkeysTable = new HotkeyTableWidget(this);
    hotkeysTable->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(hotkeysTable, &HotkeyTableWidget::customContextMenuRequested, this, &Settings::onShowHotkeyContextMenu);

    connect(hotkeysTable, &HotkeyTableWidget::hotkeyDirtyChanged, this, [this](int row, bool isDirty) {
        const int delta = static_cast< int >(isDirty) * 2 - 1;
        m_hotkeysControls.hotkeyChangesCount += delta;
        m_hotkeysControls.hotkeysTable->incChangesCount(row, delta);
        updateHotkeysDirty();
    });

    connect(hotkeysTable, &HotkeyTableWidget::hotkeyEnabledChanged, this, [this](int row, bool isEnabled) {
        const int delta = static_cast< int >(isEnabled != m_hotkeysControls.hotkeysTable->originalEnabled(row)) * 2 - 1;
        m_hotkeysControls.hotkeyChangesCount += delta;
        m_hotkeysControls.hotkeysTable->incChangesCount(row, delta);
        updateHotkeysDirty();
    });

    connect(hotkeysTable, &HotkeyTableWidget::hotkeyValidChanged, this, [this](bool isValid) {
        m_hotkeysControls.invalidHotkeysCount += 2 * static_cast< int >(!isValid) - 1;
    });

    return hotkeysTable;
}


QWidget* Settings::createServiceConfigTab()
{
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);
    auto* buttonLayout = new QHBoxLayout();

    auto* resetButton = new QPushButton("Reset");

    m_serviceConfigControls.checkButton = new QPushButton("Check Syntax");
    auto*& checkButton = m_serviceConfigControls.checkButton;

    connect(resetButton, &QPushButton::clicked, this, &Settings::onResetServiceConfig);
    connect(checkButton, &QPushButton::clicked, this, &Settings::onCheckServiceConfig);

    buttonLayout->addWidget(resetButton);
    buttonLayout->addWidget(checkButton);
    buttonLayout->addStretch();

    auto*& serviceTextEdit = m_serviceConfigControls.serviceConfigTextEdit;

    serviceTextEdit = new ServiceConfigTextEdit(this);
    serviceTextEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    connect(serviceTextEdit, &QTextEdit::textChanged, this, [this, serviceTextEdit]() {
        bool configChanged = !m_originalServiceConfig || serviceTextEdit->toPlainText() != m_originalServiceConfig->originalText();
        // OperationStatus lastStatus = m_serviceConfigLoader->status();

        // @todo maybe use config status here somehow
        if (configChanged)
            ::SetPropertyAndUpdateUi(serviceTextEdit, "checkStatus", "Changed");
        else
            ::SetPropertyAndUpdateUi(serviceTextEdit, "checkStatus", ::ToQString(m_serviceConfigLoader->status()));

        updateTabTitles();
    });

    layout->addLayout(buttonLayout);
    layout->addWidget(serviceTextEdit);

    return widget;
}


void Settings::onResetGeneral()
{
    m_currentAppConfig->excludeFromCapture = m_originalAppConfig->excludeFromCapture;
    m_currentAppConfig->saveScreenshots = m_originalAppConfig->saveScreenshots;
    populateAppUi();
}


void Settings::onResetLogging()
{
    m_currentAppConfig->asyncMode = m_originalAppConfig->asyncMode;
    m_currentAppConfig->maxQueueSize = m_originalAppConfig->maxQueueSize;
    m_currentAppConfig->minLevel = m_originalAppConfig->minLevel;
    populateAppUi();
}


void Settings::onResetProxies()
{
    m_currentAppConfig->proxies = m_originalAppConfig->proxies;
    m_proxiesControls.dirtyCellsCount = 0;
    populateAppUi();
}


void Settings::onResetHotkeys()
{
    m_hotkeysControls.hotkeysTable->resetAll();
}


void Settings::onResetServiceConfig()
{
    resetServiceConfig();
}


void Settings::onResetAllTabs()
{
    onResetGeneral();
    onResetLogging();
    onResetProxies();
    onResetHotkeys();
    onResetServiceConfig();
}


void Settings::onCheckServiceConfig()
{
    if (m_serviceConfigLoader->isLoading()) {
        m_serviceConfigLoader->cancel();
        qTrace() << "Service Config Loader canceled";
        // m_statusLabel->setText("Loading canceled.");
        return;
    }

    m_statusLabel->setText("Checking service config syntax...");

    auto parser = ServiceConfig::CreateYamlParser();
    auto renderer = ServiceConfig::CreateInjaRenderer();

    m_serviceConfigLoader->loadFromData(m_serviceConfigControls.serviceConfigTextEdit->toPlainText(),
                                        std::move(parser),
                                        std::move(renderer));
}


void Settings::onServiceConfigLoaderStatusChanged(OperationStatus status)
{
    static_assert(static_cast< int >(OperationStatus::Max) == 5);

    switch (status)
    {
        case OperationStatus::Idle:
        {
            m_serviceConfigControls.serviceConfigTextEdit->setReadOnly(false);
            setLogLine(QStringLiteral("Check Syntax"));
            break;
        }

        case OperationStatus::Processing:
        {
            m_serviceConfigControls.serviceConfigTextEdit->setReadOnly(true);
            m_serviceConfigControls.checkButton->setText("Cancel");
            setLogLine(QStringLiteral("Checking config"));
            break;
        }

        case OperationStatus::Canceled:
        {
            m_serviceConfigControls.serviceConfigTextEdit->setReadOnly(false);
            m_serviceConfigControls.checkButton->setText("Check Syntax");
            setLogLine(QStringLiteral("Syntax check canceled"));
            break;
        }

        case OperationStatus::Failed:
        {
            m_serviceConfigControls.serviceConfigTextEdit->setReadOnly(false);
            m_serviceConfigControls.checkButton->setText("Check Syntax");
            setLogLine(QStringLiteral("Syntax check failed"));
            break;
        }

        case OperationStatus::Succeeded:
        {
            m_serviceConfigControls.serviceConfigTextEdit->setReadOnly(false);
            m_serviceConfigControls.checkButton->setText("Check Syntax");
            setLogLine(QStringLiteral("Syntax's alright"));
            break;
        }

        default:
        {
            LOG_ERROR("Unrecognized OperationStatus. Contact developers.");
            break;
        }
    }

    ::SetPropertyAndUpdateUi(m_serviceConfigControls.serviceConfigTextEdit, "checkStatus", ::ToQString(status));
}


void Settings::onShowProxyContextMenu(const QPoint &pos)
{
    OverlayMenu* contextMenu = new OverlayMenu(tr("ProxyContextMenu"), this, isExcludedFromCapture());
    contextMenu->setAttribute(Qt::WA_DeleteOnClose);

    auto index = m_proxiesControls.proxiesTable->indexAt(pos);
    if (index.isValid()) {
        int row = index.row();
        const auto& proxyConfig = m_currentAppConfig->proxies[row];

        QAction* checkAction = contextMenu->addAction("Check Proxy");
        QAction* disableEnableAction = contextMenu->addAction(proxyConfig.Enabled ? "Disable" : "Enable");

        QAction* sslAction = contextMenu->addAction("Ignore SSL Errors");
        sslAction->setCheckable(true);
        sslAction->setChecked(proxyConfig.IgnoreSslErrors);

        QAction* resetAction = contextMenu->addAction("Reset");
        QAction* removeAction = contextMenu->addAction("Remove");
        QAction* insertAboveAction = contextMenu->addAction("Insert Above");
        QAction* insertBelowAction = contextMenu->addAction("Insert Below");

        connect(checkAction, &QAction::triggered, this, [this, row]() {
            m_proxiesControls.proxiesTable->proxyChecker(row)->startCheck();
        });

        connect(disableEnableAction, &QAction::triggered, this, [this, row]() {
            setEnabledProxiesRow(row, !m_currentAppConfig->proxies[row].Enabled);
        });

        connect(sslAction, &QAction::triggered, this, [this, row, sslAction]() {
            m_proxiesControls.proxiesTable->cell< QCheckBox >(row, 3)->toggle();
        });

        connect(resetAction,       &QAction::triggered, this, [this, row]() { resetProxiesRow(row); });
        connect(removeAction,      &QAction::triggered, this, [this, row]() { removeProxiesRow(row); });
        connect(insertAboveAction, &QAction::triggered, this, [this, row]() { insertProxiesRow(row); });
        connect(insertBelowAction, &QAction::triggered, this, [this, row]() { insertProxiesRow(row + 1); });

    } else {
        QAction* addAction = contextMenu->addAction("Add Proxy");
        QAction* enableAllAction = contextMenu->addAction("Enable All");
        QAction* disableAllAction = contextMenu->addAction("Disable All");
        QAction* resetAllAction = contextMenu->addAction("Reset All");

        connect(addAction, &QAction::triggered, this, [this]() {
            insertProxiesRow(m_currentAppConfig->proxies.count());
        });

        connect(enableAllAction, &QAction::triggered, this, [this]() {
            for (int row = 0; row < m_proxiesControls.proxiesTable->rowCount(); ++row)
                m_proxiesControls.proxiesTable->setEnabled(row, true);
        });

        connect(disableAllAction, &QAction::triggered, this, [this]() {
            for (int row = 0; row < m_proxiesControls.proxiesTable->rowCount(); ++row)
                m_proxiesControls.proxiesTable->setEnabled(row, false);
        });

        connect(resetAllAction, &QAction::triggered, this, &Settings::onResetProxies);
    }

    contextMenu->popup(m_proxiesControls.proxiesTable->viewport()->mapToGlobal(pos));
}


void Settings::onShowHotkeyContextMenu(const QPoint& pos)
{
    OverlayMenu* contextMenu = new OverlayMenu(tr("HotkeyContextMenu"), this, isExcludedFromCapture());
    contextMenu->setAttribute(Qt::WA_DeleteOnClose);

    auto index = m_hotkeysControls.hotkeysTable->indexAt(pos);
    if (index.isValid()) {
        const int row = index.row();
        const bool isEnabled = m_hotkeysControls.hotkeysTable->isEnabled(row);

        QAction* toggleAction = contextMenu->addAction(isEnabled ? "Disable" : "Enable");
        QAction* resetAction = contextMenu->addAction("Reset");

        connect(toggleAction, &QAction::triggered, this, [this, row, isEnabled]() {
            m_hotkeysControls.hotkeysTable->setEnabled(row, !isEnabled);
        });

        connect(resetAction, &QAction::triggered, this, [this, row]() {
            m_hotkeysControls.hotkeysTable->resetRow(row);
        });
    }
    else {
        QAction* resetAllAction = contextMenu->addAction("Reset All");

        connect(resetAllAction, &QAction::triggered, this, [this]() {
            m_hotkeysControls.hotkeysTable->resetAll();
        });
    }

    contextMenu->popup(m_hotkeysControls.hotkeysTable->viewport()->mapToGlobal(pos));
}


void Settings::onShowTabBarContextMenu(const QPoint& globalPos, int tabIndex)
{
    auto* contextMenu = new OverlayMenu(tr("TabBarMenu"), this, isExcludedFromCapture());
    contextMenu->setAttribute(Qt::WA_DeleteOnClose);

    QAction* resetTabAction = contextMenu->addAction("Reset Tab");

    connect(resetTabAction, &QAction::triggered, this, [this, tabIndex](bool) {
        switch(tabIndex)
        {
        case 0:
            onResetGeneral();
            break;
        case 1:
            onResetLogging();
            break;
        case 2:
            onResetProxies();
            break;
        case 3:
            onResetHotkeys();
            break;
        case 4:
            onResetServiceConfig();
            break;
        default:
            LOG_ERROR("Failed to recognized tab at index {}. Contact developers.", tabIndex);
            break;
        }
    });

    contextMenu->popup(globalPos);
}
