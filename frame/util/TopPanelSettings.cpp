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

    QMenu *m_settingsMenu = new QMenu;
    m_settingsMenu->addAction("设置", this, &TopPanelSettings::settingActionClicked);
    m_settingsMenu->addAction("关于", this, &TopPanelSettings::aboutActionClicked);

    m_settingsMenu->exec(QCursor::pos());
    m_settingsMenu->deleteLater();
    setAutoHide(true);
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