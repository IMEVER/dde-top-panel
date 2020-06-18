//
// Created by septemberhx on 2020/5/23.
//

#ifndef DDE_TOP_PANEL_MAINWINDOW_H
#define DDE_TOP_PANEL_MAINWINDOW_H

#include <DBlurEffectWidget>
#include <DPlatformWindowHandle>
#include "../panel/MainPanelControl.h"
#include "util/TopPanelSettings.h"
#include "xcb/xcb_misc.h"
#include "util/CustomSettings.h"
#include "../widgets/mainsettingwidget.h"
#include "../dbus/dbusdisplay.h"

DWIDGET_USE_NAMESPACE

using namespace Dock;
using DBusDock = com::deepin::dde::daemon::Dock;    // use dbus to get the height/width, position and hide mode of the dock

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
    void showOverFullscreen();

signals:
    void panelGeometryChanged();
    void settingActionClicked();

protected:
    bool event(QEvent *event) override;

private:
    void mousePressEvent(QMouseEvent *e) override;
    void clearStrutPartial();
    void setStrutPartial();
    void initConnections();

private:
    DockItemManager *m_itemManager;
    DBusDock *m_dockInter;
    MainPanelControl *m_mainPanel;
    TopPanelSettings *m_settings;
    XcbMisc *m_xcbMisc;
    DPlatformWindowHandle m_platformWindowHandle;
    QVBoxLayout *m_layout;
    Qt::WindowFlags oldFlags;
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
