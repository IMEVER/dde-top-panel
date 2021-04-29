//
// Created by septemberhx on 2020/5/23.
//

#include "TopPanelSettings.h"
#include <QScreen>
#include <QAction>

TopPanelSettings::TopPanelSettings(DockItemManager *itemManager, QScreen *screen, QWidget *parent)
        : QObject(parent)
        , m_screen(screen)
        , m_itemManager(itemManager)
{
    m_primaryRawRect = screen->geometry();
    m_primaryRawRect.setHeight(m_primaryRawRect.height() * screen->devicePixelRatio());
    m_primaryRawRect.setWidth(m_primaryRawRect.width() * screen->devicePixelRatio());

    m_hideSubMenu = new QMenu("插件");
    m_settingsMenu.addMenu(m_hideSubMenu);
    m_settingsMenu.addAction("设置", this, &TopPanelSettings::settingActionClicked);

    connect(&m_settingsMenu, &QMenu::triggered, this, &TopPanelSettings::menuActionClicked);

    calculateWindowConfig();
}

void TopPanelSettings::moveToScreen(QScreen *screen) {
    this->m_screen = screen;
    m_primaryRawRect = screen->geometry();
    m_primaryRawRect.setHeight(m_primaryRawRect.height() * screen->devicePixelRatio());
    m_primaryRawRect.setWidth(m_primaryRawRect.width() * screen->devicePixelRatio());

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
    m_hideSubMenu->addActions(actions);

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
    m_mainWindowSize.setHeight(DEFAULT_HEIGHT);
    m_mainWindowSize.setWidth(m_screen->geometry().width());
}

const QRect TopPanelSettings::primaryRect() const
{
    return m_screen->geometry();
}

const QRect TopPanelSettings::windowRect() const
{
    return QRect(primaryRect().topLeft(), m_mainWindowSize);
}

qreal TopPanelSettings::dockRatio() const
{
    return m_screen->devicePixelRatio();
}
