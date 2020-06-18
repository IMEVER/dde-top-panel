//
// Created by septemberhx on 2020/5/23.
//

#ifndef DDE_TOP_PANEL_TOPPANELSETTINGS_H
#define DDE_TOP_PANEL_TOPPANELSETTINGS_H

#include "constants.h"
#include <QObject>
#include <QWidget>
#include <QMenu>
#include <controller/dockitemmanager.h>

class TopPanelSettings : public QObject
{
    Q_OBJECT

public:
    explicit TopPanelSettings(DockItemManager *itemManager, QScreen *screen, QWidget *parent = 0);

    inline const QRect primaryRawRect() const { return m_primaryRawRect; }
    inline const QSize windowSize() const { return m_mainWindowSize; }

    const QRect windowRect() const;

    const QRect primaryRect() const;

    void showDockSettingsMenu();
    void calculateWindowConfig();

    QSize m_mainWindowSize;
    QScreen *m_screen;

    void moveToScreen(QScreen *screen);

signals:
    void settingActionClicked();

private slots:
    void menuActionClicked(QAction *action);

private:
    TopPanelSettings(TopPanelSettings const &) = delete;
    TopPanelSettings operator =(TopPanelSettings const &) = delete;
    qreal dockRatio() const;

private:
    QRect m_primaryRawRect;
    QMenu m_settingsMenu;
    QMenu *m_hideSubMenu;
    DockItemManager *m_itemManager;
};

#endif //DDE_TOP_PANEL_TOPPANELSETTINGS_H
