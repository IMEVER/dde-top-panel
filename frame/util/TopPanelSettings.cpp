//
// Created by septemberhx on 2020/5/23.
//

#include "TopPanelSettings.h"
#include "utils.h"
#include <QApplication>
#include <QScreen>
#include <QAction>

#define EffICIENT_DEFAULT_HEIGHT 32
#define WINDOW_MAX_SIZE          100
#define WINDOW_MIN_SIZE          32

extern const QPoint rawXPosition(const QPoint &scaledPos);


TopPanelSettings::TopPanelSettings(DockItemManager *itemManager, QScreen *screen, QWidget *parent)
        : QObject(parent)
        , m_dockInter(new DBusDock("com.deepin.dde.daemon.Dock", "/com/deepin/dde/daemon/Dock", QDBusConnection::sessionBus(), this))
        , m_dockWindowSize(EffICIENT_DEFAULT_HEIGHT)
        , m_position(Top)
        , m_displayMode(Dock::Efficient)
        , m_displayInter(new DBusDisplay(this))
        , m_itemManager(itemManager)
        , m_screen(screen)
{
    m_primaryRawRect = screen->geometry();
    m_primaryRawRect.setHeight(m_primaryRawRect.height() * screen->devicePixelRatio());
    m_primaryRawRect.setWidth(m_primaryRawRect.width() * screen->devicePixelRatio());
    m_screenRawHeight = m_primaryRawRect.height();
    m_screenRawWidth = m_primaryRawRect.width();

    m_hideSubMenu = new QMenu(&m_settingsMenu);
    m_hideSubMenu->setAccessibleName("pluginsmenu");
    QAction *hideSubMenuAct = new QAction(tr("Plugins"), this);
    hideSubMenuAct->setMenu(m_hideSubMenu);

    m_settingsMenu.addAction(hideSubMenuAct);
    m_settingsMenu.setTitle("Settings Menu");

    QAction *settingAction = new QAction(tr("Settings"), this);
    m_settingsMenu.addAction(settingAction);

    connect(&m_settingsMenu, &QMenu::triggered, this, &TopPanelSettings::menuActionClicked);
    connect(settingAction, &QAction::triggered, this, &TopPanelSettings::settingActionClicked);

    calculateWindowConfig();
}

void TopPanelSettings::moveToScreen(QScreen *screen) {
    this->m_screen = screen;
    m_primaryRawRect = screen->geometry();
    m_primaryRawRect.setHeight(m_primaryRawRect.height() * screen->devicePixelRatio());
    m_primaryRawRect.setWidth(m_primaryRawRect.width() * screen->devicePixelRatio());
    m_screenRawWidth = m_primaryRawRect.width();
    m_screenRawHeight = m_primaryRawRect.height();

    calculateWindowConfig();
}

void TopPanelSettings::showDockSettingsMenu()
{
    // create actions
    QList<QAction *> actions;
    for (auto *p : m_itemManager->pluginList()) {
        if (!p->pluginIsAllowDisable())
            continue;

        const bool enable = !p->pluginIsDisable();
        const QString &name = p->pluginName();
        const QString &display = p->pluginDisplayName();

//        if (!m_trashPluginShow && name == "trash") {
//            continue;
//        }

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

    m_settingsMenu.exec(QCursor::pos());
}

void TopPanelSettings::menuActionClicked(QAction *action)
{
    Q_ASSERT(action);

    // check plugin hide menu.
    const QString &data = action->data().toString();
    if (data.isEmpty())
        return;
    for (auto *p : m_itemManager->pluginList()) {
        if (p->pluginName() == data)
            return p->pluginStateSwitched();
    }
}

void TopPanelSettings::calculateWindowConfig()
{
    m_mainWindowSize.setHeight(32);
    m_mainWindowSize.setWidth(primaryRect().width());
    resetFrontendGeometry();
}

const QRect TopPanelSettings::primaryRect() const
{
    QRect rect = m_primaryRawRect;
    qreal scale = this->m_screen->devicePixelRatio();

    rect.setWidth(std::round(qreal(rect.width()) / scale));
    rect.setHeight(std::round(qreal(rect.height()) / scale));

    return rect;
}

void TopPanelSettings::resetFrontendGeometry()
{
    const QRect r = this->windowRect();
    const qreal ratio = dockRatio();
    const QPoint p = rawXPosition(r.topLeft());
    const uint w = r.width() * ratio;
    const uint h = r.height() * ratio;

    m_frontendRect = QRect(p.x(), p.y(), w, h);
}

const QRect TopPanelSettings::windowRect() const
{
    QSize size = m_mainWindowSize;

    const QRect primaryRect = this->primaryRect();
    const int offsetX = (primaryRect.width() - size.width());
    const int offsetY = (primaryRect.height() - size.height());
    QPoint p = QPoint(offsetX, 0);

    return QRect(primaryRect.topLeft() + p, size);
}

qreal TopPanelSettings::dockRatio() const
{
    QScreen const *screen = Utils::screenAtByScaled(m_frontendRect.center());
    
    return screen ? screen->devicePixelRatio() : qApp->devicePixelRatio();
}
