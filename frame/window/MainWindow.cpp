//
// Created by septemberhx on 2020/5/23.
//

#include <xcb/xcb_misc.h>
#include "MainWindow.h"
#include "controller/dockitemmanager.h"
#include "dbustoppaneladaptors.h"

#include <QScreen>
#include <QApplication>
#include <QFile>
#include <KWindowSystem>

#include "util/XUtils.h"

#define SNI_WATCHER_SERVICE "org.kde.StatusNotifierWatcher"
#define SNI_WATCHER_PATH "/StatusNotifierWatcher"

static MainSettingWidget *m_settingWidget=nullptr;

MainWindow::MainWindow(QScreen *screen, bool disablePlugin, QWidget *parent)
    : DBlurEffectWidget(parent)
    , m_itemManager(disablePlugin ? nullptr : DockItemManager::instance(this))
    , m_mainPanel(new MainPanelControl(this))
    , m_xcbMisc(XcbMisc::instance())
    , m_platformWindowHandle(this, this)
    , m_sniWatcher(disablePlugin ? nullptr : new StatusNotifierWatcher(SNI_WATCHER_SERVICE, SNI_WATCHER_PATH, QDBusConnection::sessionBus(), this))
    , m_dockInter(new DBusDock("com.deepin.dde.daemon.Dock", "/com/deepin/dde/daemon/Dock", QDBusConnection::sessionBus(), this))
    , m_eventInter(new XEventMonitor("com.deepin.api.XEventMonitor", "/com/deepin/api/XEventMonitor", QDBusConnection::sessionBus(), this))
    , m_showAnimation(new QVariantAnimation(this))
    , m_hideAnimation(new QVariantAnimation(this))
    , m_leaveTimer(new QTimer(this))
    , fullscreenIds({})
{
    // setWindowFlag(Qt::WindowDoesNotAcceptFocus);
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_AlwaysShowToolTips);

    m_mainPanel->setObjectName("mainWindow");

    QVBoxLayout *layout(new QVBoxLayout(this));
    layout->addWidget(m_mainPanel);
    layout->setSpacing(0);
    layout->setMargin(0);

    m_settings = new TopPanelSettings(screen, this);
    m_xcbMisc->set_window_type(winId(), XcbMisc::Dock);

    if(!disablePlugin)
        initSNIHost();

    this->initConnections();

    m_showAnimation->setStartValue(-m_settings->windowSize().height());
    m_showAnimation->setEndValue(0);
    m_showAnimation->setDuration(300);
    m_showAnimation->setEasingCurve(QEasingCurve::Linear);
    connect(m_showAnimation, &QPropertyAnimation::valueChanged, [ this ](const QVariant &value){
        move(pos().x(), m_settings->windowRect().y() + value.toDouble());
    });
    connect(m_showAnimation, &QVariantAnimation::finished, [ this ] {
        move(m_settings->windowRect().topLeft());
    });

    m_hideAnimation->setStartValue(0);
    m_hideAnimation->setEndValue(-m_settings->windowSize().height());
    m_hideAnimation->setDuration(300);
    m_hideAnimation->setEasingCurve(QEasingCurve::Linear);
    connect(m_hideAnimation, &QPropertyAnimation::valueChanged, [ this ](const QVariant &value){ move(pos().x(), m_settings->windowRect().y() + value.toDouble()); show(); });
    connect(m_hideAnimation, &QVariantAnimation::finished, this, [ this ]{
        hide();
        setWindowFlag(Qt::X11BypassWindowManagerHint, false);
        setWindowFlag(Qt::Tool, false);
        // setWindowFlag(Qt::WindowDoesNotAcceptFocus);
        move(m_settings->windowRect().topLeft());
        m_xcbMisc->set_window_type(winId(), XcbMisc::Dock);
        setStrutPartial();
        show();
        updateRegionMonitorWatch();
     });

    m_leaveTimer->setSingleShot(true);
    m_leaveTimer->setInterval(1000);
    connect(m_leaveTimer, &QTimer::timeout, [this]{
        if(m_settings->autoHide() && (windowFlags() & Qt::X11BypassWindowManagerHint) && m_hideAnimation->state() != QVariantAnimation::Running && !m_settings->windowRect().contains(QCursor::pos()))
        {
            if(m_showAnimation->state() == QVariantAnimation::Running)
                m_showAnimation->stop();
            m_hideAnimation->start();
        }
    });

    // platformwindowhandle only works when the widget is visible...
    if(DWindowManagerHelper::instance()->hasComposite())
    {
        DPlatformWindowHandle::enableDXcbForWindow(this, true);
        m_platformWindowHandle.setEnableBlurWindow(true);
    }
    // m_platformWindowHandle.setTranslucentBackground(true);
    m_platformWindowHandle.setWindowRadius(0);  // have no idea why it doesn't work :(
    m_platformWindowHandle.setShadowOffset(QPoint(0, 5));
    m_platformWindowHandle.setShadowColor(QColor(0, 0, 0, 0.3 * 255));
    m_platformWindowHandle.setBorderWidth(1);
    m_platformWindowHandle.setShadowRadius(15);

    moveToScreen(screen);

    this->applyCustomSettings();
    if(m_itemManager)
        this->m_itemManager->startLoadPlugins();

    KWindowSystem::self()->setState(winId(), NET::SkipTaskbar);
}

MainWindow::~MainWindow() {
    delete m_xcbMisc;
    QDBusConnection::sessionBus().unregisterService(m_sniHostService);
}

void MainWindow::setStrutPartial()
{
    // first, clear old strut partial
    m_xcbMisc->clear_strut_partial(winId());

    const QRect &primaryRawRect = m_settings->primaryRawRect();

    uint strut = primaryRawRect.top() + m_settings->windowSize().height() * devicePixelRatioF();
    uint strutTop = primaryRawRect.top();
    uint strutStart = primaryRawRect.left();
    uint strutEnd = primaryRawRect.right();
    QRect strutArea(strutStart, strutTop, strutEnd, strut);

    // pass if strut area is intersect with other screen
    int count = 0;
    const QRect pr = m_settings->primaryRect();
    for (auto *screen : qApp->screens()) {
        const QRect sr = screen->geometry();
        if (sr == pr)
            continue;

        if (sr.intersects(strutArea))
            ++count;
    }
    if (count > 0) {
        return;
    }
    m_xcbMisc->set_strut_partial(winId(), XcbMisc::OrientationTop, strut, strutStart, strutEnd);
}

void MainWindow::initSNIHost()
{
    // registor dock as SNI Host on dbus
    QDBusConnection dbusConn = QDBusConnection::sessionBus();
    m_sniHostService = QString("me.imever.dde.TopPanel");
    dbusConn.registerService(m_sniHostService);
    dbusConn.registerObject("/StatusNotifierHost", this);

    if (m_sniWatcher->isValid()) {
        m_sniWatcher->RegisterStatusNotifierHost(m_sniHostService);
    } else {
        qDebug() << SNI_WATCHER_SERVICE << "SNI watcher daemon is not exist for now!";
        connect(QDBusConnection::sessionBus().interface(), &QDBusConnectionInterface::serviceOwnerChanged, this, &MainWindow::onDbusNameOwnerChanged);
    }
}

void MainWindow::onDbusNameOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner)
{
    Q_UNUSED(oldOwner);

    if (name == SNI_WATCHER_SERVICE && !newOwner.isEmpty()) {
        qDebug() << SNI_WATCHER_SERVICE << "SNI watcher daemon started, register dock to watcher as SNI Host";
        m_sniWatcher->RegisterStatusNotifierHost(m_sniHostService);
    }
}

void MainWindow::initConnections() {
    if(m_itemManager)
    {
        connect(m_itemManager, &DockItemManager::itemInserted, m_mainPanel, &MainPanelControl::insertItem, Qt::DirectConnection);
        connect(m_itemManager, &DockItemManager::itemUpdated, m_mainPanel, &MainPanelControl::itemUpdated, Qt::DirectConnection);
        connect(m_itemManager, &DockItemManager::itemRemoved, m_mainPanel, &MainPanelControl::removeItem, Qt::DirectConnection);
        connect(m_itemManager, &DockItemManager::requestWindowAutoHide, m_settings, &TopPanelSettings::setAutoHide);

        connect(m_mainPanel, &MainPanelControl::itemMoved, m_itemManager, &DockItemManager::itemMoved, Qt::DirectConnection);
    }

        connect(m_settings, &TopPanelSettings::autoHideChanged, [ this ](bool autoHide) {
            if(fullscreenIds.isEmpty())
                return;

            if(autoHide)
            {
                if((windowFlags() & Qt::X11BypassWindowManagerHint) && !m_leaveTimer->isActive() && m_hideAnimation->state() != QVariantAnimation::Running)
                    m_leaveTimer->start();
            }
            else
            {
                if(m_leaveTimer->isActive())
                    m_leaveTimer->stop();
                // if(m_hideAnimation->state() == QVariantAnimation::Running)
                // {
                //     m_hideAnimation->stop();
                //     m_showAnimation->start();
                // }
            }
        });

    connect(DWindowManagerHelper::instance(), &DWindowManagerHelper::hasCompositeChanged, this, [this]{
        QTimer::singleShot(1000, this, [ this ]{
            const bool composite = DWindowManagerHelper::instance()->hasComposite();
            DPlatformWindowHandle::enableDXcbForWindow(this, composite);
            m_platformWindowHandle.setEnableBlurWindow(composite);
            m_platformWindowHandle.setTranslucentBackground(composite);
        });

    }, Qt::QueuedConnection);

    auto createSettingWindow = [this]{
        int screenNum = QApplication::desktop()->screenNumber(this);
        if(m_settingWidget == nullptr) {
            m_settingWidget = new MainSettingWidget();
            connect(m_settingWidget, &MainSettingWidget::pluginVisibleChanged, this, &MainWindow::pluginVisibleChanged);
            connect(m_settingWidget, &QWidget::destroyed, []{ m_settingWidget = nullptr; });
        }
        m_settingWidget->move(qApp->screens().at(screenNum)->availableGeometry().center() - m_settingWidget->rect().center());
    };

    connect(m_settings, &TopPanelSettings::settingActionClicked, this, [createSettingWindow]{
        createSettingWindow();
        m_settingWidget->showSetting();
    });
    connect(m_settings, &TopPanelSettings::aboutActionClicked, this, [createSettingWindow]{
        createSettingWindow();
        m_settingWidget->showAbout();
    });

    connect(CustomSettings::instance(), &CustomSettings::panelChanged, this, &MainWindow::applyCustomSettings);

    // connect(windowHandle(), &QWindow::visibleChanged, this, &MainWindow::updateRegionMonitorWatch);

    connect(KWindowSystem::self(), &KWindowSystem::windowRemoved, [ this ](WId wId){
        if(fullscreenIds.remove(wId))
        {
            updateRegionMonitorWatch();
        }
    });

    connect(KWindowSystem::self(), qOverload<WId, NET::Properties, NET::Properties2>(&KWindowSystem::windowChanged), [this](WId wId, NET::Properties properties, NET::Properties2 properties2){
        if(QApplication::screens().count() > 1)
        {
            int index = XUtils::getWindowScreenNum(wId);


            if(index > -1 && QApplication::desktop()->screenNumber(this) != index)
                return;
        }

        if(properties.testFlag(NET::WMDesktop))
        {
            if(fullscreenIds.contains(wId))
                updateRegionMonitorWatch();
        }
        else if(properties.testFlag(NET::WMState))
        {
            KWindowInfo kwin(wId, NET::WMState);
            if(kwin.valid())
            {
                if(kwin.hasState(NET::Hidden))
                {
                    if(fullscreenIds.remove(wId))
                        updateRegionMonitorWatch();
                }
                else if(kwin.hasState(NET::FullScreen))
                {
                    if(!fullscreenIds.contains(wId))
                    {
                        fullscreenIds.insert(wId);
                            updateRegionMonitorWatch();
                    }
                }
                else if(fullscreenIds.remove(wId))
                {
                    updateRegionMonitorWatch();
                }
            }
        }
    });
    connect(KWindowSystem::self(), &KWindowSystem::currentDesktopChanged, this, &MainWindow::updateRegionMonitorWatch);
}

void MainWindow::mousePressEvent(QMouseEvent *e)
{
    e->ignore();
    if (e->button() == Qt::RightButton) {
        m_settings->showDockSettingsMenu();
    }
}

void MainWindow::moveToScreen(QScreen *screen) {
    hide();
    m_settings->moveToScreen(screen);
    this->resize(m_settings->windowSize());
    m_mainPanel->adjustSize();
    QThread::msleep(100);  // sleep for a short while to make sure the movement is successful
    this->move(m_settings->primaryRect().topLeft());
    this->setStrutPartial();
    show();
}

void MainWindow::applyCustomSettings() {
    CustomSettings *customSettings = CustomSettings::instance();
    disconnect(m_dockInter, &DBusDock::OpacityChanged, this, 0);

    if(customSettings->isPanelCustom())
    {
        this->setMaskColor(customSettings->getPanelBgColor());
        this->setMaskAlpha(customSettings->getPanelOpacity());
    }
    else
    {
        setMaskColor(AutoColor);
        setMaskAlpha(m_dockInter->opacity() * 255);

        connect(m_dockInter, &DBusDock::OpacityChanged, this, [ this ](double opacity) {
            setMaskAlpha(opacity * 255);
            this->m_platformWindowHandle.setShadowRadius(opacity < 0.2 ? 10 : 20);
        });
    }
    const QString bgImg = customSettings->getPanelBgImg();
    const bool repeat = customSettings->isPanelBgImgRepeat();
    if(!bgImg.isNull() && QFile::exists(bgImg))
        this->setStyleSheet(QString(" MainPanelControl { background-image: url(%1); %2 }").arg(bgImg).arg(repeat ? "" : "background-repeat: no-repeat; background-position: center center;"));
}

void MainWindow::updateRegionMonitorWatch()
{
    unRegisterRegion();

    if(m_showAnimation->state() == QVariantAnimation::Running)
        m_showAnimation->stop();

    if(windowFlags() & Qt::X11BypassWindowManagerHint)
    {
        if(m_leaveTimer->isActive())
            m_leaveTimer->stop();
        if(m_hideAnimation->state() == QVariantAnimation::Running)
            m_hideAnimation->stop();

        hide();
        setWindowFlag(Qt::X11BypassWindowManagerHint, false);
        setWindowFlag(Qt::Tool, false);
        // setWindowFlag(Qt::WindowDoesNotAcceptFocus);
        move(m_settings->windowRect().topLeft());
        m_xcbMisc->set_window_type(winId(), XcbMisc::Dock);
        setStrutPartial();
        show();
    }

    if(!fullscreenIds.isEmpty())
    {
        for(auto winId : fullscreenIds)
        {
            KWindowInfo kwin(winId, NET::WMDesktop);
            if(kwin.valid() && kwin.isOnCurrentDesktop())
            {
                const int flags = Motion | Button | Key;
                const QRect windowRect = m_settings->windowRect();
                const qreal scale = devicePixelRatioF();
                int x, y, w;

                x = windowRect.x();
                y = windowRect.y();
                w = windowRect.right();

                m_registerKey = m_eventInter->RegisterArea(x * scale, y * scale, w * scale, y * scale + 1, flags);

                connect(m_eventInter, &XEventMonitor::CursorInto, this, &MainWindow::onRegionMonitorChanged);
                break;
            }
        }
    }
}

void MainWindow::unRegisterRegion()
{
    if (!m_registerKey.isEmpty())
    {
        disconnect(m_eventInter, &XEventMonitor::CursorInto, this, &MainWindow::onRegionMonitorChanged);
        m_eventInter->UnregisterArea(m_registerKey);
        m_registerKey.clear();
    }
}

void MainWindow::onRegionMonitorChanged(int x, int y, const QString &key)
{
    if(key == m_registerKey)
    {
        unRegisterRegion();
        ActivateWindow();
    }
}

void MainWindow::ActivateWindow()
{
    if (!fullscreenIds.isEmpty() && !(windowFlags() & Qt::X11BypassWindowManagerHint))
    {
           setWindowFlag(Qt::X11BypassWindowManagerHint);
           setWindowFlag(Qt::Tool);
           m_showAnimation->start();
           show();
    }
}

void MainWindow::DeactivateWindow()
{
    if(windowFlags() & Qt::X11BypassWindowManagerHint)
    {
        m_hideAnimation->start();
    }
}

void MainWindow::ToggleMenu(int id)
{
    this->m_mainPanel->toggleMenu(id);
}

QStringList MainWindow::GetLoadedPlugins() {
    QStringList names;
    if(m_itemManager) {
        for (auto *p : m_itemManager->pluginList())
        {
            if (p->pluginIsAllowDisable() && !p->pluginDisplayName().isEmpty())
                names.append(p->pluginDisplayName());
        }
    }
    return names;
}

QString MainWindow::getPluginKey(QString pluginName) {
    if(m_itemManager) {
        for(auto *p : m_itemManager->pluginList())
            if(p->pluginDisplayName() == pluginName)
                return p->pluginName();
    }
    return QString();
}

bool MainWindow::getPluginVisible(QString pluginName) {
    if(m_itemManager) {
        for(auto *p : m_itemManager->pluginList())
            if(p->pluginDisplayName() == pluginName)
                return !p->pluginIsDisable();
    }
    return false;
}

void MainWindow::setPluginVisible(QString pluginName, bool visible) {
    if(m_itemManager) {
        for(auto *p : m_itemManager->pluginList())
            if(p->pluginDisplayName() == pluginName) {
                p->pluginStateSwitched();
                break;
            }
    }
}

void MainWindow::leaveEvent(QEvent *event)
{
    if(fullscreenIds.count() > 0 && windowFlags() & Qt::X11BypassWindowManagerHint && m_settings->autoHide() && !m_leaveTimer->isActive() && m_hideAnimation->state() != QVariantAnimation::Running)
        m_leaveTimer->start();
    DBlurEffectWidget::leaveEvent(event);
}

TopPanelLauncher::TopPanelLauncher()
    : primaryScreen(nullptr)
    , m_display(new DBusDisplay(this))
{
    connect(m_display, &DBusDisplay::MonitorsChanged, this, &TopPanelLauncher::rearrange);
    connect(m_display, &DBusDisplay::PrimaryChanged, this, &TopPanelLauncher::primaryChanged);
    this->rearrange();
}

void TopPanelLauncher::rearrange() {
    this->primaryChanged();

    for (auto p_screen : qApp->screens()) {
        if (mwMap.contains(p_screen)) {
            mwMap[p_screen]->moveToScreen(p_screen);
            continue;
        }

        qDebug() << "===========> create top panel on" << p_screen->name();
        MainWindow *mw = new MainWindow(p_screen, p_screen != qApp->primaryScreen());
        mwMap.insert(p_screen, mw);

        if(p_screen == qApp->primaryScreen())
        {
            new DBusTopPanelAdaptors (mw);
            QDBusConnection::sessionBus().registerObject("/me/imever/dde/TopPanel", "me.imever.dde.TopPanel", mw);
        }
    }

    for (auto screen : mwMap.keys()) {
        if (!qApp->screens().contains(screen)) {
            qDebug() << "===========> close top panel";
            MainWindow *window = mwMap.take(screen);
            window->close();
            window->deleteLater();
        }
    }
}

void TopPanelLauncher::primaryChanged() {
    QScreen *currPrimaryScreen = qApp->primaryScreen();
    if (currPrimaryScreen == primaryScreen) return;
    if (primaryScreen == nullptr) {
        primaryScreen = currPrimaryScreen;
        return;
    }

    // prevent raw primary screen is destroyed (unplugged)
    bool ifRawPrimaryExists = qApp->screens().contains(primaryScreen);
    bool ifCurrPrimaryExisted = mwMap.contains(currPrimaryScreen);
    bool ifRawPrimaryExisted = mwMap.contains(primaryScreen);

    MainWindow *pMw = nullptr;
    if (ifRawPrimaryExisted) {
        pMw = mwMap[primaryScreen];
        mwMap.remove(primaryScreen);
        pMw->hide();
    }

    if (ifCurrPrimaryExisted) {
        if (ifRawPrimaryExists) {
            mwMap[currPrimaryScreen]->moveToScreen(primaryScreen);
            mwMap[primaryScreen] = mwMap[currPrimaryScreen];
        } else {
            MainWindow *tmp = mwMap[currPrimaryScreen];
            mwMap[currPrimaryScreen]->hide();
            tmp->deleteLater();
        }
        mwMap.remove(currPrimaryScreen);
    }

    if (ifRawPrimaryExisted) {
        pMw->moveToScreen(currPrimaryScreen);
        mwMap.insert(currPrimaryScreen, pMw);
    }
    this->primaryScreen = currPrimaryScreen;
}
