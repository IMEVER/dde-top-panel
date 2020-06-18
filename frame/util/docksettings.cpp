/*
 * Copyright (C) 2011 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     sbw <sbw@sbw.so>
 *
 * Maintainer: sbw <sbw@sbw.so>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "docksettings.h"
#include "util/utils.h"

#include <QDebug>
#include <QX11Info>
#include <QGSettings>

#include <DApplication>
#include <QScreen>
#include <QGSettings>

#define FASHION_MODE_PADDING    30
#define MAINWINDOW_MARGIN       10

#define FASHION_DEFAULT_HEIGHT 72
#define EffICIENT_DEFAULT_HEIGHT 30
#define WINDOW_MAX_SIZE          100
#define WINDOW_MIN_SIZE          30

DWIDGET_USE_NAMESPACE

extern const QPoint rawXPosition(const QPoint &scaledPos);

static QGSettings *GSettingsByMenu()
{
    static QGSettings settings("com.deepin.dde.dock.module.menu");
    return &settings;
}

static QGSettings *GSettingsByTrash()
{
    static QGSettings settings("com.deepin.dde.dock.module.trash");
    return &settings;
}

DockSettings::DockSettings(QWidget *parent)
    : QObject(parent)
    , m_dockInter(new DBusDock("com.deepin.dde.daemon.Dock", "/com/deepin/dde/daemon/Dock", QDBusConnection::sessionBus(), this))
    , m_menuVisible(true)
    , m_autoHide(true)
    , m_opacity(0.4)
    , m_keepShownAct(tr("Keep Shown"), this)
    , m_keepHiddenAct(tr("Keep Hidden"), this)
    , m_smartHideAct(tr("Smart Hide"), this)
    , m_displayInter(new DBusDisplay(this))
    , m_itemManager(DockItemManager::instance(this))
    , m_trashPluginShow(true)
{
    m_settingsMenu.setAccessibleName("dock-settingsmenu");
    checkService();

    m_primaryRawRect = m_displayInter->primaryRawRect();
    m_screenRawHeight = m_displayInter->screenRawHeight();
    m_screenRawWidth = m_displayInter->screenRawWidth();

    m_hideMode = Dock::HideMode(m_dockInter->hideMode());
    m_hideState = Dock::HideState(m_dockInter->hideState());

    m_fashionModeAct.setCheckable(true);
    m_efficientModeAct.setCheckable(true);
    m_topPosAct.setCheckable(true);
    m_bottomPosAct.setCheckable(true);
    m_leftPosAct.setCheckable(true);
    m_rightPosAct.setCheckable(true);
    m_keepShownAct.setCheckable(true);
    m_keepHiddenAct.setCheckable(true);
    m_smartHideAct.setCheckable(true);

    QMenu *modeSubMenu = new QMenu(&m_settingsMenu);
    modeSubMenu->setAccessibleName("modesubmenu");
    modeSubMenu->addAction(&m_fashionModeAct);
    modeSubMenu->addAction(&m_efficientModeAct);
    QAction *modeSubMenuAct = new QAction(tr("Mode"), this);
    modeSubMenuAct->setMenu(modeSubMenu);

    QMenu *locationSubMenu = new QMenu(&m_settingsMenu);
    locationSubMenu->setAccessibleName("locationsubmenu");
    locationSubMenu->addAction(&m_topPosAct);
    locationSubMenu->addAction(&m_bottomPosAct);
    locationSubMenu->addAction(&m_leftPosAct);
    locationSubMenu->addAction(&m_rightPosAct);
    QAction *locationSubMenuAct = new QAction(tr("Location"), this);
    locationSubMenuAct->setMenu(locationSubMenu);

    QMenu *statusSubMenu = new QMenu(&m_settingsMenu);
    statusSubMenu->setAccessibleName("statussubmenu");
    statusSubMenu->addAction(&m_keepShownAct);
    statusSubMenu->addAction(&m_keepHiddenAct);
    statusSubMenu->addAction(&m_smartHideAct);
    QAction *statusSubMenuAct = new QAction(tr("Status"), this);
    statusSubMenuAct->setMenu(statusSubMenu);

    m_hideSubMenu = new QMenu(&m_settingsMenu);
    m_hideSubMenu->setAccessibleName("pluginsmenu");
    QAction *hideSubMenuAct = new QAction(tr("Plugins"), this);
    hideSubMenuAct->setMenu(m_hideSubMenu);

    m_settingsMenu.addAction(modeSubMenuAct);
    m_settingsMenu.addAction(locationSubMenuAct);
    m_settingsMenu.addAction(statusSubMenuAct);
    m_settingsMenu.addAction(hideSubMenuAct);
    m_settingsMenu.setTitle("Settings Menu");

    connect(&m_settingsMenu, &QMenu::triggered, this, &DockSettings::menuActionClicked);
    connect(GSettingsByMenu(), &QGSettings::changed, this, &DockSettings::onGSettingsChanged);
    connect(m_dockInter, &DBusDock::HideModeChanged, this, &DockSettings::hideModeChanged, Qt::QueuedConnection);
    connect(m_dockInter, &DBusDock::HideStateChanged, this, &DockSettings::hideStateChanged);
    connect(m_dockInter, &DBusDock::ServiceRestarted, this, &DockSettings::resetFrontendGeometry);
    connect(m_dockInter, &DBusDock::OpacityChanged, this, &DockSettings::onOpacityChanged);

    connect(m_displayInter, &DBusDisplay::PrimaryRectChanged, this, &DockSettings::primaryScreenChanged, Qt::QueuedConnection);
    connect(m_displayInter, &DBusDisplay::ScreenHeightChanged, this, &DockSettings::primaryScreenChanged, Qt::QueuedConnection);
    connect(m_displayInter, &DBusDisplay::ScreenWidthChanged, this, &DockSettings::primaryScreenChanged, Qt::QueuedConnection);
    connect(m_displayInter, &DBusDisplay::PrimaryChanged, this, &DockSettings::primaryScreenChanged, Qt::QueuedConnection);
    connect(GSettingsByTrash(), &QGSettings::changed, this, &DockSettings::onTrashGSettingsChanged);
    QTimer::singleShot(0, this, [=] {onGSettingsChanged("enable");});

    DApplication *app = qobject_cast<DApplication *>(qApp);
    if (app) {
        connect(app, &DApplication::iconThemeChanged, this, &DockSettings::gtkIconThemeChanged);
    }

    calculateWindowConfig();
    resetFrontendGeometry();

    QTimer::singleShot(0, this, [ = ] {onOpacityChanged(m_dockInter->opacity());});
    QTimer::singleShot(0, this, [=] {
        onGSettingsChanged("enable");
    });
}

DockSettings &DockSettings::Instance()
{
    static DockSettings settings;
    return settings;
}

const QRect DockSettings::primaryRect() const
{
    QRect rect = m_primaryRawRect;
    qreal scale = qApp->primaryScreen()->devicePixelRatio();

    rect.setWidth(std::round(qreal(rect.width()) / scale));
    rect.setHeight(std::round(qreal(rect.height()) / scale));

    return rect;
}

const QSize DockSettings::panelSize() const
{
    return m_mainWindowSize;
}

const QRect DockSettings::windowRect( const bool hide) const
{
    QSize size = m_mainWindowSize;
    if (hide) {
        size.setHeight(2);
    }

    const QRect primaryRect = this->primaryRect();
    const int offsetX = (primaryRect.width() - size.width()) / 2;
    const int offsetY = (primaryRect.height() - size.height()) / 2;
    QPoint p(0, 0);

    p = QPoint(offsetX, 0);


    return QRect(primaryRect.topLeft() + p, size);
}

void DockSettings::showDockSettingsMenu()
{
    QGSettings gsettings("com.deepin.dde.dock.module.menu");
    if (gsettings.keys().contains("enable") && !gsettings.get("enable").toBool()) {
        return;
    }

    m_autoHide = false;

    // create actions
    QList<QAction *> actions;
    for (auto *p : m_itemManager->pluginList()) {
        if (!p->pluginIsAllowDisable())
            continue;

        const bool enable = !p->pluginIsDisable();
        const QString &name = p->pluginName();
        const QString &display = p->pluginDisplayName();

        if (!m_trashPluginShow && name == "trash") {
            continue;
        }

        QAction *act = new QAction(display, this);
        act->setCheckable(true);
        act->setChecked(enable);
        act->setData(name);

        actions << act;
    }

    // sort by name
    std::sort(actions.begin(), actions.end(), [](QAction * a, QAction * b) -> bool {
        return a->data().toString() > b->data().toString();
    });

    // add actions
    qDeleteAll(m_hideSubMenu->actions());
    for (auto act : actions)
        m_hideSubMenu->addAction(act);

    m_keepShownAct.setChecked(m_hideMode == KeepShowing);
    m_keepHiddenAct.setChecked(m_hideMode == KeepHidden);
    m_smartHideAct.setChecked(m_hideMode == SmartHide);

    m_settingsMenu.exec(QCursor::pos());

    setAutoHide(true);
}

void DockSettings::updateGeometry()
{

}

void DockSettings::setAutoHide(const bool autoHide)
{
    if (m_autoHide == autoHide)
        return;

    m_autoHide = autoHide;
    emit autoHideChanged(m_autoHide);
}

void DockSettings::menuActionClicked(QAction *action)
{
    Q_ASSERT(action);

    if (action == &m_keepShownAct)
        return m_dockInter->setHideMode(KeepShowing);
    if (action == &m_keepHiddenAct)
        return m_dockInter->setHideMode(KeepHidden);
    if (action == &m_smartHideAct)
        return m_dockInter->setHideMode(SmartHide);

    // check plugin hide menu.
    const QString &data = action->data().toString();
    if (data.isEmpty())
        return;
    for (auto *p : m_itemManager->pluginList()) {
        if (p->pluginName() == data)
            return p->pluginStateSwitched();
    }
}

void DockSettings::onGSettingsChanged(const QString &key)
{
    if (key != "enable") {
        return;
    }

    QGSettings *setting = GSettingsByMenu();

    if (setting->keys().contains("enable")) {
        const bool isEnable = GSettingsByMenu()->keys().contains("enable") && GSettingsByMenu()->get("enable").toBool();
        m_menuVisible=isEnable && setting->get("enable").toBool();
    }
}

void DockSettings::hideModeChanged()
{
//    qDebug() << Q_FUNC_INFO;
    m_hideMode = Dock::HideMode(m_dockInter->hideMode());

    emit windowHideModeChanged();
}

void DockSettings::hideStateChanged()
{
//    qDebug() << Q_FUNC_INFO;
    const Dock::HideState state = Dock::HideState(m_dockInter->hideState());

    if (state == Dock::Unknown)
        return;

    m_hideState = state;

    emit windowVisibleChanged();
}

void DockSettings::primaryScreenChanged()
{
//    qDebug() << Q_FUNC_INFO;
    m_primaryRawRect = m_displayInter->primaryRawRect();
    m_screenRawHeight = m_displayInter->screenRawHeight();
    m_screenRawWidth = m_displayInter->screenRawWidth();

    emit dataChanged();
    calculateWindowConfig();

    // 主屏切换时，如果缩放比例不一样，需要刷新item的图标(bug:3176)
    m_itemManager->refershItemsIcon();
}

void DockSettings::resetFrontendGeometry()
{
    const QRect r = windowRect();
    const qreal ratio = dockRatio();
    const QPoint p = rawXPosition(r.topLeft());
    const uint w = r.width() * ratio;
    const uint h = r.height() * ratio;

    m_frontendRect = QRect(p.x(), p.y(), w, h);
    m_dockInter->SetFrontendWindowRect(p.x(), p.y(), w, h);
}

void DockSettings::updateFrontendGeometry()
{
    resetFrontendGeometry();
}

bool DockSettings::test(const QList<QRect> &otherScreens) const
{
    QRect maxStrut(0, 0, m_screenRawWidth, m_screenRawHeight);
        maxStrut.setBottom(m_primaryRawRect.top() - 1);
        maxStrut.setLeft(m_primaryRawRect.left());
        maxStrut.setRight(m_primaryRawRect.right());


    if (maxStrut.width() == 0 || maxStrut.height() == 0)
        return true;

    for (const auto &r : otherScreens)
        if (maxStrut.intersects(r))
            return false;

    return true;
}


void DockSettings::onOpacityChanged(const double value)
{
    if (m_opacity == value) return;

    m_opacity = value;

    emit opacityChanged(value * 255);
}

void DockSettings::calculateWindowConfig()
{
        m_dockWindowSize = m_dockInter->windowSizeEfficient();
        if (m_dockWindowSize > WINDOW_MAX_SIZE || m_dockWindowSize < WINDOW_MIN_SIZE) {
            m_dockWindowSize = EffICIENT_DEFAULT_HEIGHT;
            m_dockInter->setWindowSize(EffICIENT_DEFAULT_HEIGHT);
        }

            m_mainWindowSize.setHeight(m_dockWindowSize);
            m_mainWindowSize.setWidth(primaryRect().width());


    resetFrontendGeometry();
}

void DockSettings::gtkIconThemeChanged()
{
    qDebug() << Q_FUNC_INFO;
    m_itemManager->refershItemsIcon();
}

qreal DockSettings::dockRatio() const
{
    QScreen const *screen = Utils::screenAtByScaled(m_frontendRect.center());

    return screen ? screen->devicePixelRatio() : qApp->devicePixelRatio();
}

void DockSettings::onWindowSizeChanged()
{
    calculateWindowConfig();
}

void DockSettings::checkService()
{
    // com.deepin.dde.daemon.Dock服务比dock晚启动，导致dock启动后的状态错误

    QString serverName = "com.deepin.dde.daemon.Dock";
    QDBusConnectionInterface *ifc = QDBusConnection::sessionBus().interface();

    if (!ifc->isServiceRegistered(serverName)) {
        connect(ifc, &QDBusConnectionInterface::serviceOwnerChanged, this, [ = ](const QString & name, const QString & oldOwner, const QString & newOwner) {
            if (name == serverName && !newOwner.isEmpty()) {

                m_dockInter = new DBusDock(serverName, "/com/deepin/dde/daemon/Dock", QDBusConnection::sessionBus(), this);
                hideModeChanged();
                hideStateChanged();
                onOpacityChanged(m_dockInter->opacity());
                onWindowSizeChanged();

                disconnect(ifc);
            }
        });
    }
}

void DockSettings::onTrashGSettingsChanged(const QString &key)
{
    if (key != "enable") {
        return ;
    }

    QGSettings *setting = GSettingsByTrash();

    if (setting->keys().contains("enable")) {
         m_trashPluginShow = GSettingsByTrash()->keys().contains("enable") && GSettingsByTrash()->get("enable").toBool();
    }
}
