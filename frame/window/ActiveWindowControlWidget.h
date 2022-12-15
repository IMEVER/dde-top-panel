//
// Created by septemberhx on 2020/5/26.
//

#ifndef DDE_TOP_PANEL_ACTIVEWINDOWCONTROLWIDGET_H
#define DDE_TOP_PANEL_ACTIVEWINDOWCONTROLWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <DGuiApplicationHelper>
// #include <QQmlApplicationEngine>
#include <QMenuBar>
#include <QWidgetAction>
#include "../../appmenu/appmenumodel.h"
#include <com_deepin_wm.h>
#include "../util/CustomSettings.h"
#include "../util/CustomizeMenubar.h"

using Dtk::Gui::DGuiApplicationHelper;

class ActiveWindowControlWidget : public QWidget {

    Q_OBJECT

public:
    explicit ActiveWindowControlWidget(QWidget *parent = 0);

public slots:
    void activeWindowInfoChanged(WId wId);
    void maximizeWindow();
    void toggleMenu(int id);

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    void initMenuBar();
    void setButtonsVisible(bool visible);

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private slots:
    void windowChanged(WId, NET::Properties, NET::Properties2);
    void themeTypeChanged(DGuiApplicationHelper::ColorType themeType);

private:
    // QQmlApplicationEngine *engine = nullptr;
    QStack<WId> activeIdStack;
    WId currActiveWinId;

    CustomizeMenubar *menuBar;
    AppMenuModel *m_appMenuModel;

    bool mouseClicked;
};

#endif //DDE_TOP_PANEL_ACTIVEWINDOWCONTROLWIDGET_H
