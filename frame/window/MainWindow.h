//
// Created by septemberhx on 2020/5/23.
//

#ifndef DDE_TOP_PANEL_MAINWINDOW_H
#define DDE_TOP_PANEL_MAINWINDOW_H

#include <DBlurEffectWidget>
#include <DPlatformWindowHandle>
#include "MainPanelControl.h"
#include "statusnotifierwatcher_interface.h"
#include "util/TopPanelSettings.h"
#include "xcb/xcb_misc.h"
#include "util/CustomSettings.h"
#include "mainsettingwidget.h"
#include "../dbus/dbusdisplay.h"

DWIDGET_USE_NAMESPACE

using org::kde::StatusNotifierWatcher;
using namespace Dock;

class MainWindow : public DBlurEffectWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QScreen *screen, bool enableBlacklist, QWidget *parent = 0);
    ~MainWindow() override;

    void loadPlugins();
    void moveToScreen(QScreen *screen);
    void adjustPanelSize();
    void applyCustomSettings(const CustomSettings& customSettings);

signals:
    void panelGeometryChanged();
    void settingActionClicked();

private slots:
    void onDbusNameOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner);

public Q_SLOTS:
    void setRadius(int radius) {};
    void ToggleMenu(int id);
    void ActivateWindow();

protected:
    bool event(QEvent *event) override;

private:
    void mousePressEvent(QMouseEvent *e) override;
    void clearStrutPartial();
    void setStrutPartial();
    void initConnections();
    void initSNIHost();

private:
    DockItemManager *m_itemManager;
    MainPanelControl *m_mainPanel;
    TopPanelSettings *m_settings;
    XcbMisc *m_xcbMisc;
    DPlatformWindowHandle m_platformWindowHandle;
    Qt::WindowFlags oldFlags;

    QDBusConnectionInterface *m_dbusDaemonInterface;
    org::kde::StatusNotifierWatcher *m_sniWatcher;
    QString m_sniHostService;
};

class TopPanelLauncher : public QObject {
    Q_OBJECT

public:
    explicit TopPanelLauncher();

private slots:
    void monitorsChanged();
    void primaryChanged();

private:
    MainSettingWidget *m_settingWidget;

    QScreen *primaryScreen;
    DBusDisplay *m_display;
    QMap<QScreen *, MainWindow *> mwMap;
    void rearrange();
};


#endif //DDE_TOP_PANEL_MAINWINDOW_H
