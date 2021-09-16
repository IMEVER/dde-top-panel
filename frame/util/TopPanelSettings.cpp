//
// Created by septemberhx on 2020/5/23.
//

#include "TopPanelSettings.h"
#include <QScreen>
#include <QAction>

TopPanelSettings::TopPanelSettings(QScreen *screen, QWidget *parent)
    : QObject(parent)
    , m_autoHide(true)
{
    m_hideSubMenu = new QMenu("插件");
    m_settingsMenu.addMenu(m_hideSubMenu);
    m_settingsMenu.addAction("设置", this, &TopPanelSettings::settingActionClicked);

    connect(&m_settingsMenu, &QMenu::triggered, this, &TopPanelSettings::menuActionClicked);
    moveToScreen(screen);
}

void TopPanelSettings::moveToScreen(QScreen *screen)
{
    this->m_screen = screen;
    m_primaryRawRect = screen->geometry();
    m_primaryRawRect.setHeight(m_primaryRawRect.height() * screen->devicePixelRatio());
    m_primaryRawRect.setWidth(m_primaryRawRect.width() * screen->devicePixelRatio());
}

void TopPanelSettings::showDockSettingsMenu()
{
    setAutoHide(false);
    // create actions
    QList<QAction *> actions;
    for (auto *p : DockItemManager::instance()->pluginList()) {
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

    setAutoHide(true);
}

void TopPanelSettings::menuActionClicked(QAction *action)
{
    Q_ASSERT(action);

    // check plugin hide menu.
    const QString &data = action->data().toString();
    if (data.isEmpty())
        return;
    for (auto *p : DockItemManager::instance()->pluginList()) {
        if (p->pluginName() == data)
            return p->pluginStateSwitched();
    }
}

QSize TopPanelSettings::windowSize()
{
    return QSize(m_screen->geometry().width(), DEFAULT_HEIGHT);
}

const QRect TopPanelSettings::primaryRect() const
{
    return m_screen->geometry();
}

const QRect TopPanelSettings::windowRect() const
{
    return QRect(primaryRect().topLeft(), QSize(m_screen->geometry().width(), DEFAULT_HEIGHT));
}

qreal TopPanelSettings::dockRatio() const
{
    return m_screen->devicePixelRatio();
}

void TopPanelSettings::setAutoHide(bool autoHide)
{
    if(m_autoHide != autoHide)
    {
        m_autoHide = autoHide;
        emit autoHideChanged(m_autoHide);
    }
}