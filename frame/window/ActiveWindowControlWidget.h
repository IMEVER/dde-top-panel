//
// Created by septemberhx on 2020/5/26.
//

#ifndef DDE_TOP_PANEL_ACTIVEWINDOWCONTROLWIDGET_H
#define DDE_TOP_PANEL_ACTIVEWINDOWCONTROLWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QToolButton>
#include <DGuiApplicationHelper>
#include <com_deepin_dde_daemon_dock.h>
#include <com_deepin_wm.h>
#include <QMenuBar>
#include "../../appmenu/appmenumodel.h"
#include "QClickableLabel.h"
#include <com_deepin_wm.h>
#include "../util/CustomSettings.h"
#include "../item/appitem.h"

using DBusDock = com::deepin::dde::daemon::Dock;
using DBusWM = com::deepin::wm;
using Dtk::Gui::DGuiApplicationHelper;

class ActiveWindowControlWidget : public QWidget {

    Q_OBJECT

public:
    explicit ActiveWindowControlWidget(QWidget *parent = 0);
    // bool eventFilter(QObject *watched, QEvent *event) override;

public slots:
    void activeWindowInfoChanged();
    void maximizeWindow();
    void applyCustomSettings(const CustomSettings& settings);
    void toggleStartMenu();
    void toggleMenu();

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    void setButtonsVisible(bool visible);
    int currScreenNum();

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private slots:
    void maxButtonClicked();
    void minButtonClicked();
    void closeButtonClicked();
    void updateMenu();
    void windowChanged(WId, NET::Properties, NET::Properties2);
    void themeTypeChanged(DGuiApplicationHelper::ColorType themeType);
    void reloadAppItems();

private:
    QMenu *menu;
    QHBoxLayout *m_layout;
    QLabel *m_winTitleLabel;

    QStack<int> activeIdStack;
    int currActiveWinId;
    QString currActiveWinTitle;

    QWidget *m_buttonWidget;
    QHBoxLayout *m_buttonLayout;
    QToolButton *closeButton;
    QToolButton *minButton;
    QToolButton *maxButton;

    QLabel *m_iconLabel;
    QMenuBar *menuBar;
    AppMenuModel *m_appMenuModel;

    DBusDock *m_appInter;
    DBusWM  *m_wmInter;
    bool mouseClicked;

    QPropertyAnimation *m_buttonShowAnimation;
    QPropertyAnimation *m_buttonHideAnimation;
    QTimer *m_fixTimer;
    QMap<QString, AppItem *> m_appItemMap;
};

#endif //DDE_TOP_PANEL_ACTIVEWINDOWCONTROLWIDGET_H
