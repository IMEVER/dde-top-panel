//
// Created by septemberhx on 2020/5/23.
//

#ifndef DDE_TOP_PANEL_MAINWINDOW_H
#define DDE_TOP_PANEL_MAINWINDOW_H

#include <QVariantAnimation>
#include <DBlurEffectWidget>
#include <DPlatformWindowHandle>
#include <com_deepin_dde_daemon_dock.h>
#include <com_deepin_api_xeventmonitor.h>

#include "MainPanelControl.h"
#include "statusnotifierwatcher_interface.h"
#include "util/TopPanelSettings.h"
#include "xcb/xcb_misc.h"
#include "util/CustomSettings.h"
#include "mainsettingwidget.h"
#include "../dbus/dbusdisplay.h"

DWIDGET_USE_NAMESPACE

using XEventMonitor = ::com::deepin::api::XEventMonitor;
using org::kde::StatusNotifierWatcher;
using namespace Dock;
using DBusDock = com::deepin::dde::daemon::Dock;

class DockItemManager;

class MainWindow : public DBlurEffectWidget
{
    Q_OBJECT

    enum Flag{
        Motion = 1 << 0,
        Button = 1 << 1,
        Key    = 1 << 2
    };

public:
    explicit MainWindow(QScreen *screen, bool disablePlugin, QWidget *parent = 0);
    ~MainWindow() override;

    void moveToScreen(QScreen *screen);
    void applyCustomSettings();

signals:
    void panelGeometryChanged();
    void pluginVisibleChanged(QString pluginName, bool visible);

private slots:
    void onDbusNameOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner);
    void onRegionMonitorChanged(int x, int y, const QString &key);
    void updateRegionMonitorWatch();
    void unRegisterRegion();

public Q_SLOTS:
    void setRadius(int radius) {};
    void ToggleMenu(int id);
    void ActivateWindow();
    void DeactivateWindow();
    QStringList GetLoadedPlugins();
    QString getPluginKey(QString pluginName);
    bool getPluginVisible(QString pluginName);
    void setPluginVisible(QString pluginName, bool visible);

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void leaveEvent(QEvent *event) override;

private:
    void setStrutPartial();
    void initConnections();
    void initSNIHost();

private:
    DockItemManager *m_itemManager;
    MainPanelControl *m_mainPanel;
    TopPanelSettings *m_settings;
    XcbMisc *m_xcbMisc;
    DPlatformWindowHandle m_platformWindowHandle;

    org::kde::StatusNotifierWatcher *m_sniWatcher;
    QString m_sniHostService;
    DBusDock *m_dockInter;
    XEventMonitor *m_eventInter;
    QString m_registerKey;
    QVariantAnimation *m_showAnimation;
    QVariantAnimation *m_hideAnimation;
    QTimer *m_leaveTimer;
    QSet<WId> fullscreenIds;
};

class TopPanelLauncher : public QObject {
    Q_OBJECT

public:
    explicit TopPanelLauncher();

private slots:
    void primaryChanged();
    void rearrange();

private:
    QScreen *primaryScreen;
    DBusDisplay *m_display;
    QMap<QScreen *, MainWindow *> mwMap;
};


#endif //DDE_TOP_PANEL_MAINWINDOW_H
