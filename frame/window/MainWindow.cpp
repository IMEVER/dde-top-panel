//
// Created by septemberhx on 2020/5/23.
//

#include <xcb/xcb_misc.h>
#include "MainWindow.h"
#include "controller/dockitemmanager.h"
#include "util/utils.h"

#define SNI_WATCHER_SERVICE "org.kde.StatusNotifierWatcher"
#define SNI_WATCHER_PATH "/StatusNotifierWatcher"

MainWindow::MainWindow(QScreen *screen, bool enableBlacklist, QWidget *parent)
    : DBlurEffectWidget(parent)
    , m_itemManager(new DockItemManager(this, enableBlacklist))
    , m_dockInter(new DBusDock("com.deepin.dde.daemon.Dock", "/com/deepin/dde/daemon/Dock", QDBusConnection::sessionBus(), this))
    , m_mainPanel(new MainPanelControl(this))
    , m_xcbMisc(XcbMisc::instance())
    , m_platformWindowHandle(this, this)
    , m_layout(new QVBoxLayout(this))
    , m_dbusDaemonInterface(QDBusConnection::sessionBus().interface())
    , m_sniWatcher(new org::kde::StatusNotifierWatcher(SNI_WATCHER_SERVICE, SNI_WATCHER_PATH, QDBusConnection::sessionBus(), this))
{
//    setWindowFlag(Qt::WindowDoesNotAcceptFocus);
    setAccessibleName("dock-top-panel-mainwindow");
    m_mainPanel->setAccessibleName("dock-top-panel-mainpanel");
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setAcceptDrops(true);

    this->setLayout(this->m_layout);
    this->m_layout->addWidget(m_mainPanel);
    this->setFixedHeight(32);
    this->setFixedWidth(screen->size().width());
    this->m_layout->setContentsMargins(0, 0, 0, 0);
    this->m_layout->setSpacing(0);
    this->m_layout->setMargin(0);

    m_settings = new TopPanelSettings(m_itemManager, screen, this);
    m_xcbMisc->set_window_type(winId(), XcbMisc::Dock);

    this->initSNIHost();
    this->initConnections();

    for (auto item : m_itemManager->itemList())
        m_mainPanel->insertItem(-1, item);

    setStrutPartial();

    this->move(m_settings->m_frontendRect.topLeft() / m_settings->m_screen->devicePixelRatio());

    setVisible(true);
    // platformwindowhandle only works when the widget is visible...
    DPlatformWindowHandle::enableDXcbForWindow(this, true);
    m_platformWindowHandle.setEnableBlurWindow(true);
    m_platformWindowHandle.setTranslucentBackground(true);
    m_platformWindowHandle.setWindowRadius(0);  // have no idea why it doesn't work :(
    m_platformWindowHandle.setShadowOffset(QPoint(0, 5));
    m_platformWindowHandle.setShadowColor(QColor(0, 0, 0, 0.3 * 255));
    m_platformWindowHandle.setBorderWidth(1);

    this->applyCustomSettings(*CustomSettings::instance());
}

MainWindow::~MainWindow() {
    delete m_xcbMisc;
}

const QPoint rawXPosition(const QPoint &scaledPos)
{
    QScreen const *screen = Utils::screenAtByScaled(scaledPos);

    return screen ? screen->geometry().topLeft() +
                    (scaledPos - screen->geometry().topLeft()) *
                    screen->devicePixelRatio()
                  : scaledPos;
}

void MainWindow::clearStrutPartial()
{
    m_xcbMisc->clear_strut_partial(winId());
}

void MainWindow::setStrutPartial()
{
    // first, clear old strut partial
    clearStrutPartial();

    const auto ratio = devicePixelRatioF();
    const int maxScreenHeight = m_settings->screenRawHeight();
    const int maxScreenWidth = m_settings->screenRawWidth();
    const QPoint &p = rawXPosition(m_settings->windowRect().topLeft());
    const QSize &s = m_settings->windowSize();
    const QRect &primaryRawRect = m_settings->primaryRawRect();

    XcbMisc::Orientation orientation = XcbMisc::OrientationTop;
    uint strut = 0;
    uint strutTop = 0;
    uint strutStart = 0;
    uint strutEnd = 0;

    QRect strutArea(0, 0, maxScreenWidth, maxScreenHeight);

    orientation = XcbMisc::OrientationTop;
    strut = p.y() + s.height() * ratio;
    strutTop = p.y();
    strutStart = p.x();
    strutEnd = qMin(qRound(p.x() + s.width() * ratio), primaryRawRect.right());
    strutArea.setTop(strutTop);
    strutArea.setLeft(strutStart);
    strutArea.setRight(strutEnd);
    strutArea.setBottom(strut);

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
        qWarning() << "strutArea is intersects with another screen.";
        qWarning() << maxScreenHeight << maxScreenWidth << p << s;
        return;
    }
    m_xcbMisc->set_strut_partial(winId(), orientation, strut, strutStart, strutEnd);
}

void MainWindow::initConnections() {
    connect(m_itemManager, &DockItemManager::itemInserted, m_mainPanel, &MainPanelControl::insertItem, Qt::DirectConnection);
    connect(m_itemManager, &DockItemManager::itemUpdated, m_mainPanel, &MainPanelControl::itemUpdated, Qt::DirectConnection);
    connect(m_itemManager, &DockItemManager::itemRemoved, m_mainPanel, &MainPanelControl::removeItem, Qt::DirectConnection);

    connect(m_mainPanel, &MainPanelControl::itemMoved, DockItemManager::instance(), &DockItemManager::itemMoved, Qt::DirectConnection);
    connect(m_mainPanel, &MainPanelControl::itemAdded, DockItemManager::instance(), &DockItemManager::itemAdded, Qt::DirectConnection);

    connect(m_dbusDaemonInterface, &QDBusConnectionInterface::serviceOwnerChanged, this, &MainWindow::onDbusNameOwnerChanged);
    connect(m_settings, &TopPanelSettings::settingActionClicked, this, &MainWindow::settingActionClicked);
    connect(CustomSettings::instance(), &CustomSettings::settingsChanged, this, [this]() {
        this->applyCustomSettings(*CustomSettings::instance());
    });
}

void MainWindow::mousePressEvent(QMouseEvent *e)
{
    e->ignore();
    if (e->button() == Qt::RightButton) {
        m_settings->showDockSettingsMenu();
        return;
    }
}

void MainWindow::loadPlugins() {
    this->m_itemManager->startLoadPlugins();
}

void MainWindow::moveToScreen(QScreen *screen) {
    m_settings->moveToScreen(screen);
    this->resize(m_settings->m_mainWindowSize);
    // m_mainPanel->resize(m_settings->m_mainWindowSize);
    m_mainPanel->adjustSize();
    QThread::msleep(100);  // sleep for a short while to make sure the movement is successful
    this->move(m_settings->m_frontendRect.topLeft() / m_settings->m_screen->devicePixelRatio());
    this->setStrutPartial();
}

void MainWindow::setRaidus(int radius) {
    m_platformWindowHandle.setWindowRadius(radius);  // have no idea why it doesn't work :(
}

void MainWindow::adjustPanelSize() {
    this->m_mainPanel->adjustSize();
}

void MainWindow::onDbusNameOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner) {
    Q_UNUSED(oldOwner)

    if (name == SNI_WATCHER_SERVICE && !newOwner.isEmpty()) {
        qDebug() << SNI_WATCHER_SERVICE << "SNI watcher daemon started, register dock to watcher as SNI Host";
        m_sniWatcher->RegisterStatusNotifierHost(m_sniHostService);
    }
}

void MainWindow::initSNIHost()
{
    // registor dock as SNI Host on dbus
    QDBusConnection dbusConn = QDBusConnection::sessionBus();
    m_sniHostService = QString("org.kde.StatusNotifierHost-") + QString::number(qApp->applicationPid());
    dbusConn.registerService(m_sniHostService);
    dbusConn.registerObject("/StatusNotifierHost", this);

    if (m_sniWatcher->isValid()) {
        m_sniWatcher->RegisterStatusNotifierHost(m_sniHostService);
    } else {
        qDebug() << SNI_WATCHER_SERVICE << "SNI watcher daemon is not exist for now!";
    }
}

void MainWindow::applyCustomSettings(const CustomSettings &customSettings) {
    this->setMaskAlpha(customSettings.getPanelOpacity());
    this->setMaskColor(customSettings.getPanelBgColor());
    this->m_mainPanel->applyCustomSettings(customSettings);
}

TopPanelLauncher::TopPanelLauncher()
    : m_display(new DBusDisplay(this))
{
    this->m_settingWidget = new MainSettingWidget();
    connect(m_display, &DBusDisplay::MonitorsChanged, this, &TopPanelLauncher::monitorsChanged);
//    connect(m_display, &DBusDisplay::PrimaryChanged, this, &TopPanelLauncher::primaryChanged);
    this->rearrange();
}

void TopPanelLauncher::monitorsChanged() {
    rearrange();
}

void TopPanelLauncher::rearrange() {
    this->primaryChanged();

    for (auto p_screen : qApp->screens()) {
        if (mwMap.contains(p_screen)) {
            // adjust size
            mwMap[p_screen]->hide();
            mwMap[p_screen]->moveToScreen(p_screen);
            mwMap[p_screen]->show();
            mwMap[p_screen]->setRaidus(0);
            continue;
        }

        qDebug() << "===========> create top panel on" << p_screen->name();
        MainWindow *mw = new MainWindow(p_screen, p_screen != qApp->primaryScreen());
        connect(mw, &MainWindow::settingActionClicked, this, [this]() {
            int screenNum = QApplication::desktop()->screenNumber(dynamic_cast<MainWindow*>(sender()));
            this->m_settingWidget->move(QApplication::desktop()->screen(screenNum)->rect().center() - this->m_settingWidget->rect().center());
            this->m_settingWidget->show();
        });
        if (p_screen == qApp->primaryScreen()) {
            mw->loadPlugins();
        }
        mwMap.insert(p_screen, mw);
    }

    for (auto screen : mwMap.keys()) {
        if (!qApp->screens().contains(screen)) {
            mwMap[screen]->close();
            qDebug() << "===========> close top panel";
            mwMap.remove(screen);
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
        pMw->hide();
    }

    if (ifCurrPrimaryExisted) {
        mwMap[currPrimaryScreen]->hide();
        if (ifRawPrimaryExists) {
            mwMap[currPrimaryScreen]->moveToScreen(primaryScreen);
            mwMap[primaryScreen] = mwMap[currPrimaryScreen];
        } else {
            mwMap[currPrimaryScreen]->close();
            mwMap.remove(primaryScreen);
        }
    }

    if (ifRawPrimaryExisted) {
        pMw->moveToScreen(currPrimaryScreen);
        if (mwMap.contains(currPrimaryScreen)) {
            mwMap[currPrimaryScreen] = pMw;
        } else {
            mwMap.insert(currPrimaryScreen, pMw);
        }
        pMw->show();
        pMw->setRaidus(0);

        if (ifRawPrimaryExists) {
            mwMap[primaryScreen]->show();
            mwMap[primaryScreen]->setRaidus(0);
        } else {
            mwMap.remove(primaryScreen);
        }

        if (!ifCurrPrimaryExisted) {
            mwMap.remove(primaryScreen);
        }
    }

    this->primaryScreen = currPrimaryScreen;
}
