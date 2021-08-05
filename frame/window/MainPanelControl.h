//
// Created by septemberhx on 2020/5/23.
//

#ifndef DDE_TOP_PANEL_MAINPANELCONTROL_H
#define DDE_TOP_PANEL_MAINPANELCONTROL_H

#include <QWidget>
#include <QBoxLayout>
#include "item/traypluginitem.h"
#include "ActiveWindowControlWidget.h"
#include "util/CustomSettings.h"

// using DockEntryInter = com::deepin::dde::daemon::dock::Entry;

class MainPanelControl : public QWidget
{
    Q_OBJECT

public:
    explicit MainPanelControl(QWidget *parent = 0);

    // Tray area
    void addTrayAreaItem(int index, QWidget *wdg);
    void removeTrayAreaItem(QWidget *wdg);
    void addFixedAreaItem(int index, QWidget *wdg);
    void removePluginAreaItem(QWidget *wdg);
    void addPluginAreaItem(int index, QWidget *wdg);
    void addFixedPluginAreaItem(int index, QWidget *wdg);
    void removeFixedPluginAreaItem(QWidget *wdg);
    void toggleMenu(int id);

    void applyCustomSettings(const CustomSettings& customSettings);

public slots:
    void insertItem(const int index, DockItem *item);
    void removeItem(DockItem *item);
    void itemUpdated(DockItem *item);

signals:
    void itemMoved(DockItem *sourceItem, DockItem *targetItem);


protected:
    void dragMoveEvent(QDragMoveEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;

public:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void init();
    void startDrag(DockItem *);
    void handleDragMove(QDragMoveEvent *e);
    DockItem *dropTargetItem(DockItem *sourceItem, QPoint point);
    void moveItem(DockItem *sourceItem, DockItem *targetItem);

private:
    QBoxLayout *m_mainPanelLayout;
    QWidget *m_trayAreaWidget;
    QBoxLayout *m_trayAreaLayout;
    QWidget *m_pluginAreaWidget;
    QBoxLayout *m_pluginLayout;
    QWidget *m_fixedPluginWidget;
    QHBoxLayout *m_fixedPluginLayout;

    TrayPluginItem *m_tray = nullptr;
    ActiveWindowControlWidget *activeWindowControlWidget;
    QPoint m_mousePressPos;
};


#endif //DDE_TOP_PANEL_MAINPANELCONTROL_H
