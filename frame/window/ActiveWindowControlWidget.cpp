//
// Created by septemberhx on 2020/5/26.
//

#include "ActiveWindowControlWidget.h"
#include <QDebug>
#include <QMouseEvent>
#include <QtX11Extras/QX11Info>
#include <QProcess>
#include "../util/desktop_entry_stat.h"
#include "AboutWindow.h"
#include "controller/dockitemmanager.h"
#include "../../appmenu/appmenumodel.h"
#include "../util/CustomSettings.h"
#include "../util/CustomizeMenubar.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QGuiApplication>
#include <QScreen>
#include <DDBusSender>
#include <QToolButton>
#include <QStyle>
#include <QGSettings>
#include <QLineEdit>
#include <QRandomGenerator>

#include "util/XUtils.h"
#undef KeyPress

class ActiveWindowControlWidgetPrivate : public QObject {
    Q_OBJECT
public:
    ActiveWindowControlWidgetPrivate(ActiveWindowControlWidget *q) : q_ptr(q) {
        mouseClicked = false;

        this->m_appMenuModel = new AppMenuModel(this);
        connect(this->m_appMenuModel, &AppMenuModel::clearMenu, this, [this] {
            while (this->menuBar->actions().size() > 6)
            {
                QAction *action = this->menuBar->actions()[5];
                if(action->menu())
                {
                disconnect(action->menu(), &QMenu::aboutToShow, this, 0);
                disconnect(action->menu(), &QMenu::aboutToHide, this, 0);
                }

                this->menuBar->removeAction(action);
            }
            this->menuBar->actions().last()->setVisible(false);
        });
        connect(this->m_appMenuModel, &AppMenuModel::modelNeedsUpdate, this, [this] {
            if (QMenu *menu = m_appMenuModel->menu())
            {
                if(menu->actions().isEmpty()) return;

                this->menuBar->insertActions(this->menuBar->actions().last(), menu->actions());
                this->menuBar->actions().last()->setVisible(true);
                this->menuBar->adjustSize();
                for(auto item : menu->actions())
                {
                    if(item->menu())
                    {
                        connect(item->menu(), &QMenu::aboutToShow, this, []{ DockItemManager::instance()->requestWindowAutoHide(false); });
                        connect(item->menu(), &QMenu::aboutToHide, this, []{ DockItemManager::instance()->requestWindowAutoHide(true); });
                    }
                }
            }
        });
        connect(this->m_appMenuModel, &AppMenuModel::requestActivateIndex, this, [this](int index) {
            Q_Q(ActiveWindowControlWidget);
            if(this->menuBar->actions().size() - 6 > index && index >= 0){
                q->toggleMenu(index + 5);
            }
        });
        connect(this->m_appMenuModel, &AppMenuModel::actionRemoved, [this](QAction *action) {
            this->menuBar->removeAction(action);
            this->menuBar->actions().last()->setVisible(this->menuBar->actions().count() > 6);
            if(action->menu())
            {
                disconnect(action->menu(), &QMenu::aboutToShow, this, 0);
                disconnect(action->menu(), &QMenu::aboutToHide, this, 0);
            }
        });
        connect(this->m_appMenuModel, &AppMenuModel::actionAdded, [this](QAction *action, QAction *before) {
            if(!before || !this->menuBar->actions().contains(before))
                before = this->menuBar->actions().last();
            this->menuBar->insertAction(before, action);
            this->menuBar->actions().last()->setVisible(this->menuBar->actions().count() > 6);
            if(action->menu())
            {
                connect(action->menu(), &QMenu::aboutToShow, []{ DockItemManager::instance()->requestWindowAutoHide(false); });
                connect(action->menu(), &QMenu::aboutToHide, []{ DockItemManager::instance()->requestWindowAutoHide(true); });
            }
        });
    }

    void initMenuBar()
    {
        Q_Q(ActiveWindowControlWidget);
        this->menuBar = new CustomizeMenubar(q);
        this->menuBar->setFixedHeight(q->height());

        QString logo = CustomSettings::instance()->getLogoImg();

        QMenu *startMenu = this->menuBar->addMenu(!logo.isNull() ? QIcon(logo) : QIcon::fromTheme("app-launcher"), "开始");
        startMenu->menuAction()->setObjectName("logo");
        startMenu->addAction(QIcon::fromTheme("applications-other"), "关   于", [] {
            QProcess::startDetached("/usr/bin/dde-file-manager", {"-p", "computer:///"});
            // QProcess::startDetached("/usr/bin/dbus-send --session --print-reply --dest=com.deepin.SessionManager /com/deepin/StartManager com.deepin.StartManager.RunCommand string:\"/usr/bin/dde-file-manager\" array:string:\"-p\",\"computer:///\"");
            // QProcess::startDetached("/usr/bin/qdbus", {"com.deepin.SessionManager", "/com/deepin/StartManager", "com.deepin.StartManager.RunCommand", "dde-file-manager", "(", "-p", "computer:///", ")"});
        });

        startMenu->addAction(QIcon::fromTheme("preferences-system"), "设    置", [] {
            // QProcess::startDetached("/usr/bin/qdbus", {"com.deepin.SessionManager", "/com/deepin/StartManager", "com.deepin.StartManager.LaunchApp", "/usr/share/applications/dde-control-center.desktop", "0", "[]"});
            QProcess::startDetached("/usr/bin/dde-control-center", {"--show"});
        });

        startMenu->addSeparator();

        startMenu->addAction(QIcon::fromTheme("app-launcher"), "启动器", this, [] {
            QProcess::startDetached("/usr/bin/qdbus", {"com.deepin.dde.Launcher", "/com/deepin/dde/Launcher", "com.deepin.dde.Launcher.Toggle"});
        });

        startMenu->addAction(QIcon::fromTheme("deepin-app-store"), "应用商店", [] {
            // QProcess::startDetached("/usr/bin/qdbus", {"com.deepin.SessionManager", "/com/deepin/StartManager", "com.deepin.StartManager.LaunchApp", "/usr/share/applications/deepin-app-store.desktop", "0", "[]"});
            QProcess::startDetached("/usr/bin/deepin-home-appstore-client", {});
        });

        startMenu->addAction(QIcon::fromTheme("folder"), "文件管理器", [] {
            QProcess::startDetached("/usr/bin/dde-file-manager", {"-O"});
        });

        startMenu->addAction(QIcon::fromTheme("deepin-system-monitor"), "系统监视器", [] {
            // QProcess::startDetached("/usr/bin/qdbus", {"com.deepin.SessionManager", "/com/deepin/StartManager", "com.deepin.StartManager.LaunchApp", "/usr/share/applications/deepin-system-monitor.desktop", "0", "[]"});
            QProcess::startDetached("/usr/bin/deepin-system-monitor", {});
        });

        startMenu->addAction(QIcon::fromTheme("deepin-devicemanager"), "设备管理器", [] {
            // QProcess::startDetached("/usr/bin/qdbus", {"com.deepin.SessionManager", "/com/deepin/StartManager", "com.deepin.StartManager.LaunchApp", "/usr/share/applications/deepin-system-monitor.desktop", "0", "[]"});
            QProcess::startDetached("/usr/bin/deepin-devicemanager", {});
        });

        startMenu->addSeparator();
        startMenu->addAction(QIcon::fromTheme("gnome-panel-force-quit"), "强制关闭窗口", [] {
            QProcess::startDetached("/usr/bin/xkill", {});
        });

        startMenu->addSeparator();

        QMenu *systemMenu = startMenu->addMenu(QIcon::fromTheme("preferences-theme"), "系统主题");
        QActionGroup *group = new QActionGroup(systemMenu);

        QAction *autoAction = systemMenu->addAction("自动");
        autoAction->setCheckable(true);
        autoAction->setData("deepin-auto");
        QAction *lightAction = systemMenu->addAction("浅色");
        lightAction->setCheckable(true);
        lightAction->setData("deepin");
        QAction *darkAction = systemMenu->addAction("深色");
        darkAction->setCheckable(true);
        darkAction->setData("deepin-dark");

        group->addAction(autoAction);
        group->addAction(lightAction);
        group->addAction(darkAction);

        connect(group, &QActionGroup::triggered, this, [](QAction *action) {
            QGSettings iconThemeSettings("com.deepin.dde.appearance");
            iconThemeSettings.set("gtk-theme", action->data().toString());
        });

        auto checkTheme = [group]
        {
            QGSettings iconThemeSettings("com.deepin.dde.appearance");
            QString theme = iconThemeSettings.get("gtk-theme").toString();
            for (auto action : group->actions())
            {
                if (action->data() == theme)
                {
                    action->setChecked(true);
                    break;
                }
            }
        };

        connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, checkTheme);
        QTimer::singleShot(100, checkTheme);

        QMenu *shutdownMenu = startMenu->addMenu(QIcon::fromTheme("system-shutdown-symbolic"), "关机");
        shutdownMenu->addAction(QIcon::fromTheme("system-log-out-symbolic"), "注销", [] {
            DDBusSender().service("com.deepin.dde.shutdownFront").path("/com/deepin/dde/shutdownFront").interface("com.deepin.dde.shutdownFront").method("Logout").call();
        });
        shutdownMenu->addAction(QIcon::fromTheme("system-lock-screen-symbolic"), "锁定", [] {
            DDBusSender().service("com.deepin.dde.lockFront").path("/com/deepin/dde/lockFront").interface("com.deepin.dde.lockFront").method("Show").call();
        });
        shutdownMenu->addAction(QIcon::fromTheme("system-switch-user-symbolic"), "切换用户", [] {
            DDBusSender().service("com.deepin.dde.shutdownFront").path("/com/deepin/dde/shutdownFront").interface("com.deepin.dde.shutdownFront").method("SwitchUser").call();
        });
        shutdownMenu->addAction(QIcon::fromTheme("system-suspend-symbolic"), "待机", [] {
            DDBusSender().service("com.deepin.dde.shutdownFront").path("/com/deepin/dde/shutdownFront").interface("com.deepin.dde.shutdownFront").method("Suspend").call();
        });
        shutdownMenu->addAction(QIcon::fromTheme("system-reboot-symbolic"), "重启", [] {
            DDBusSender().service("com.deepin.dde.shutdownFront").path("/com/deepin/dde/shutdownFront").interface("com.deepin.dde.shutdownFront").method("Restart").call();
        });
        shutdownMenu->addAction(QIcon::fromTheme("system-shutdown-symbolic"), "关机", [] {
            DDBusSender().service("com.deepin.dde.shutdownFront").path("/com/deepin/dde/shutdownFront").interface("com.deepin.dde.shutdownFront").method("Shutdown").call();
        });

        this->menuBar->addAction("关闭", [this] {
            if(currActiveWinId > 0)
            {
                NETRootInfo ri( QX11Info::connection(), NET::CloseWindow );
                ri.closeWindowRequest( currActiveWinId );
            }
        });

        this->menuBar->addAction("还原", this, &ActiveWindowControlWidgetPrivate::maximizeWindow);
        this->menuBar->addAction("最小化", this, &ActiveWindowControlWidgetPrivate::minimizeWindow);

        QMenu *appMenu = this->menuBar->addMenu("桌面");
        appMenu->menuAction()->setObjectName("appMenu");
        connect(appMenu, &QMenu::aboutToShow, [this, appMenu] {
            KWindowInfo kwin(currActiveWinId, NET::WMState | NET::WMWindowType);
            if(kwin.valid() && kwin.windowType(NET::DesktopMask) != NET::Desktop)
            {
                appMenu->actions().at(1)->setEnabled(this->m_appMenuModel->getAction(QAction::AboutRole));
                appMenu->actions().at(3)->setEnabled(this->m_appMenuModel->getAction(QAction::PreferencesRole));

                appMenu->actions().at(4)->setEnabled(true);
                appMenu->actions().at(6)->setEnabled(true);
                appMenu->actions().at(7)->setEnabled(true);

                appMenu->actions().at(4)->setChecked(kwin.hasState(NET::KeepAbove));
            } else {
                int index=1;
                while(index < 8)
                    appMenu->actions().at(index++)->setDisabled(true);
                appMenu->actions().at(4)->setChecked(false);
            }
        });

        appMenu->addAction(QIcon::fromTheme("applications-other"), "关于包(&P)", [this] {
            KWindowInfo kwin(currActiveWinId, NET::WMName|NET::WMDesktop|NET::WMPid, NET::WM2DesktopFileName|NET::WM2ClientMachine|NET::WM2WindowClass);
            if(kwin.valid())
            {
                // engine = new QQmlApplicationEngine;

                // engine->rootContext()->setContextProperty("appInfo", appInfo);
                // engine->load(QUrl(QStringLiteral("qrc:/qml/about.qml")));
                AboutWindow *about = new AboutWindow(kwin);
                about->show();
                connect(about, &AboutWindow::finished, about, &AboutWindow::deleteLater);
            }
        });

        appMenu->addAction(QIcon(), "关于(&A)", [this] {
            if(QAction *action = this->m_appMenuModel->getAction(QAction::AboutRole))
                action->trigger();
        });

        appMenu->addSeparator();
        appMenu->addAction(QIcon(), "偏好设置(&S)", [this] {
            if(QAction *action = this->m_appMenuModel->getAction(QAction::PreferencesRole))
                action->trigger();
        });

        QAction *keepTopAction = appMenu->addAction(/*QIcon::fromTheme("top"),*/ "置顶");
        keepTopAction->setCheckable(true);
        connect(keepTopAction, &QAction::triggered, [this](bool checked) {
            KWindowInfo kwin(currActiveWinId, NET::WMState);
            if(kwin.valid() && checked != kwin.hasState(NET::KeepAbove))
            {
                if (checked)
                    KWindowSystem::setState(currActiveWinId, NET::KeepAbove);
                else
                    KWindowSystem::clearState(currActiveWinId, NET::KeepAbove);
            }
        });

        appMenu->addSeparator();
        appMenu->addAction(/*QIcon::fromTheme("window-close-symbolic"),*/ "关闭(&C)", this->menuBar->actions().at(1), &QAction::trigger);
        appMenu->addAction(/*QIcon::fromTheme("application-exit"),*/ "退出(&Q)", [this] {
            if(QAction *action = this->m_appMenuModel->getAction(QAction::QuitRole))
                action->trigger();
            else if (currActiveWinId > 0)
            {
                KWindowInfo info(currActiveWinId, NET::WMPid);
                int pid = info.valid() ? info.pid() : 0;
                if(pid > 0)
                    QProcess::startDetached("/usr/bin/kill", {"-9", QString::number(pid)});
            }
        });

        QMenu *searchMenu = this->menuBar->addMenu("搜索(&S)");
        m_searchEdit = new QLineEdit(searchMenu);
        m_searchEdit->setAttribute(Qt::WA_TranslucentBackground);
        m_searchEdit->setMinimumWidth(240);
        m_searchEdit->setFixedHeight(24);
        m_searchEdit->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        m_searchEdit->setClearButtonEnabled(true);
        m_searchEdit->setAlignment(Qt::AlignCenter);
        m_searchEdit->setFrame(false);
        m_searchEdit->setContentsMargins(30, 2, 30, 2);
        m_searchEdit->setPlaceholderText("搜索");
        m_searchEdit->setStyleSheet("QLineEdit{ border-with: 0; }");
        m_searchEdit->installEventFilter(q);
        QTimer *searchTimer = new QTimer(this);
        searchTimer->setSingleShot(true);
        searchTimer->setInterval(100);
        connect(searchTimer, &QTimer::timeout, [searchMenu, this] {
            while (searchMenu->actions().count() > 1)
                searchMenu->removeAction(searchMenu->actions()[1]);

            QString searchTxt = m_searchEdit->text().toLower();
            if(searchTxt.length() > 0 && menuBar->actions().count() > 6)
            {
                QList<QAction *> list;
                std::function<void(QAction *)> searchFunc = [ &searchFunc, searchTxt, &list ](QAction *action) {
                    if(action->menu())
                        for(auto *subAction : action->menu()->actions())
                            searchFunc(subAction);
                    else if(action->text().toLower().contains(searchTxt))
                        list.append(action);
                };

                for(int i=5; i < menuBar->actions().count() - 1; i++)
                    searchFunc(menuBar->actions().at(i));

                if(list.count() > 0)
                    searchMenu->addActions(list);
            }
        });
        connect(searchMenu, &QMenu::aboutToHide, m_searchEdit, &QLineEdit::clear);
        // connect(searchMenu, &QMenu::aboutToShow, m_searchEdit, qOverload<>(&QLineEdit::setFocus));
        connect(m_searchEdit, &QLineEdit::textChanged, searchTimer, qOverload<>(&QTimer::start));
        QWidgetAction *searchAction = new QWidgetAction(searchMenu);
        searchAction->setDefaultWidget(m_searchEdit);
        searchMenu->addAction(searchAction);

        for (auto item : this->menuBar->actions())
        {
            if (item->menu())
            {
                connect(item->menu(), &QMenu::aboutToShow, [] { DockItemManager::instance()->requestWindowAutoHide(false); });
                connect(item->menu(), &QMenu::aboutToHide, [] { DockItemManager::instance()->requestWindowAutoHide(true); });
            }
        }

        if (CustomSettings::instance()->getPopupHover())
            connect(this->menuBar, &CustomizeMenubar::hovered, this, [this](QAction *action) {
                if(action->menu() && this->menuBar->actions().contains(action)) this->menuBar->setActiveAction(action);
            });

        // menu settings
        if (!CustomSettings::instance()->isShowMenuLaunch())
            startMenu->actions().at(3)->setVisible(false);
        if (!CustomSettings::instance()->isShowMenuFileManager())
            startMenu->actions().at(5)->setVisible(false);
        if (!CustomSettings::instance()->isShowMenuTheme())
            startMenu->actions().at(11)->setVisible(false);
        if (!CustomSettings::instance()->isShowMenuAboutPackage())
            appMenu->actions().first()->setVisible(false);
        if (!CustomSettings::instance()->isShowMenuSearch())
            searchMenu->menuAction()->setVisible(false);

        connect(CustomSettings::instance(), &CustomSettings::menuChanged, [startMenu, appMenu, searchMenu](const QString menu, const bool show) {
            if(menu == "launch")
                startMenu->actions().at(3)->setVisible(show);
            else if(menu == "fileManager")
                startMenu->actions().at(5)->setVisible(show);
            else if(menu == "theme")
                startMenu->actions().at(11)->setVisible(show);
            else if(menu == "aboutPackage")
                appMenu->actions().at(0)->setVisible(show);
            else if(menu == "search")
                searchMenu->menuAction()->setVisible(show);
        });
    }

    void popupHoverChanged(const bool &on) {
        if(on)
            connect(menuBar, &CustomizeMenubar::hovered, this, [ this ](QAction *action){
                if(action->menu() && menuBar->actions().contains(action))
                    this->menuBar->setActiveAction(action);
            });
        else
            disconnect(menuBar, &CustomizeMenubar::hovered, this, 0);
    }

    void setButtonsVisible(const bool visible)
    {
        if (visible != menuBar->actions().at(1)->isVisible())
        {
            int i = 1;
            while (i < 4)
            {
                QTimer::singleShot(100 * i, [this, visible, i] {
                    this->menuBar->actions().at(i)->setVisible(visible);
                });
                i++;
            }
        }
    }

    void maximizeWindow()
    {
        XUtils::toggleWindow(currActiveWinId);
        // if (XUtils::checkIfWinMaximum(currActiveWinId))
        // {
        //     XUtils::unmaximizeWindow(currActiveWinId);

        //     // checkIfWinMaximum is MUST needed here.
        //     //   I don't know whether XSendEvent is designed like this,
        //     //   my test shows unmaximizeWindow by XSendEvent will work when others try to fetch its properties.
        //     //   i.e., checkIfWinMaximum
        //     // XUtils::checkIfWinMaximum(currActiveWinId);

        //     // this->m_appInter->MinimizeWindow(this->currActiveWinId);
        // }
        // else
        // {
        //     // sadly the dbus maximizeWindow cannot unmaximize window :(
        //     // this->m_appInter->MaximizeWindow(this->currActiveWinId);
        //     XUtils::maximizeWindow(currActiveWinId);
        //     // XUtils::checkIfWinMaximum(currActiveWinId);
        // }
    }

    void minimizeWindow() {
        KWindowSystem::minimizeWindow(currActiveWinId);
    }

    void themeTypeChanged() {
        themeTypeChanged(DGuiApplicationHelper::instance()->themeType());
    }

    void themeTypeChanged(const DGuiApplicationHelper::ColorType themeType)
    {
        Q_Q(ActiveWindowControlWidget);
        CustomSettings *settings = CustomSettings::instance();

        if (settings->isButtonCustom())
        {
            menuBar->actions().at(1)->setIcon(QIcon(settings->getActiveCloseIconPath()));
            menuBar->actions().at(2)->setIcon(QIcon(settings->getActiveUnmaximizedIconPath()));
            menuBar->actions().at(3)->setIcon(QIcon(settings->getActiveMinimizedIconPath()));
        }
        else
        {
            // QString subfix = themeType == DGuiApplicationHelper::LightType ? "" : "-white";
            // this->closeButton->setIcon(QIcon(QString(":/icons/close%1.svg").arg(subfix)));
            // this->maxButton->setIcon(QIcon(QString(":/icons/maximum%1.svg").arg(subfix)));
            // this->minButton->setIcon(QIcon(QString(":/icons/minimum%1.svg").arg(subfix)));

            auto findIcon = [q](QStyle::StandardPixmap button)
            {
                QIcon icon, retIcon;
                QPixmap pixmap;

                if (true)
                {
                    switch (button)
                    {
                    case QStyle::SP_TitleBarCloseButton:
                        icon = QIcon::fromTheme("window-close-symbolic");
                        break;
                    case QStyle::SP_TitleBarNormalButton:
                        icon = QIcon::fromTheme("window-restore-symbolic");
                        break;
                    case QStyle::SP_TitleBarMinButton:
                        icon = QIcon::fromTheme("window-minimize-symbolic");
                        break;
                    default:
                        break;
                    }

                    pixmap = icon.pixmap(16).copy(2, 2, 12, 12).scaled(24, 24);
                    retIcon = QIcon(pixmap);
                }
                else
                {
                    icon = q->style()->standardIcon(button);
                    pixmap = icon.pixmap(40, QIcon::Normal, QIcon::On).copy(13, 13, 14, 14).scaled(24, 24);
                    retIcon.addPixmap(pixmap, QIcon::Normal, QIcon::On);
                    pixmap = icon.pixmap(40, QIcon::Active, QIcon::On).copy(13, 13, 14, 14).scaled(24, 24);
                    retIcon.addPixmap(pixmap, QIcon::Active, QIcon::On);
                    pixmap = icon.pixmap(40, QIcon::Selected, QIcon::On).copy(13, 13, 14, 14).scaled(24, 24);
                    retIcon.addPixmap(pixmap, QIcon::Selected, QIcon::On);
                }
                return retIcon;
            };

            menuBar->actions().at(1)->setIcon(findIcon(QStyle::SP_TitleBarCloseButton));
            menuBar->actions().at(2)->setIcon(findIcon(QStyle::SP_TitleBarNormalButton));
            menuBar->actions().at(3)->setIcon(findIcon(QStyle::SP_TitleBarMinButton));
        }

        QGSettings iconThemeSettings("com.deepin.dde.appearance");
        if (CustomSettings::instance()->isIconThemeFollowSystem())
            iconThemeSettings.set("icon-theme", themeType == DGuiApplicationHelper::LightType ? "bloom" : "bloom-dark");

        QString color;
        if (settings->isPanelCustom())
            color = settings->getActiveFontColor().name();
        else
            color = themeType == DGuiApplicationHelper::LightType ? "black" : "white";
        int padding = QRandomGenerator::system()->bounded(1);
        menuBar->setStyleSheet(QString("QMenuBar {padding-left: %1; font-size: 15px; color: %2; background-color: rgba(0, 0, 0, 0); spacing: 1px; }\
            QMenuBar::item { padding-top: 3px; padding-bottom: 3px; padding-left: 5px; padding-right: 5px; }\
            QMenuBar::item:selected { border-radius: 4px; color: palette(highlighted-text); background-color: palette(highlight); }")
                                        .arg(padding)
                                        .arg(color));
    }

    void windowChanged(WId wId, NET::Properties properties, NET::Properties2 properties2)
    {
        Q_Q(ActiveWindowControlWidget);
        if (KWindowSystem::activeWindow() == wId && properties.testFlag(NET::WMGeometry) && QApplication::screens().size() > 1)
        {
            int currScreenNum = QApplication::desktop()->screenNumber(qobject_cast<QWidget *>(q->parent()));
            int activeWinScreenNum = XUtils::getWindowScreenNum(wId);
            if (activeWinScreenNum >= 0 && activeWinScreenNum != currScreenNum)
            {
                if (currActiveWinId == wId)
                    activeWindowInfoChanged(wId);
            }
            else if (wId != currActiveWinId)
                activeWindowInfoChanged(wId);

            return;
        }

        if (wId == currActiveWinId && properties.testFlag(NET::WMState))
        {
            KWindowInfo kwin(currActiveWinId, NET::WMState);
            if (kwin.valid())
                setButtonsVisible(kwin.hasState(NET::Max));
        }
    }

    void activeWindowInfoChanged(WId activeWinId)
    {
        Q_Q(ActiveWindowControlWidget);
        if (activeWinId == q->winId() || activeWinId == currActiveWinId)
            return;

        KWindowInfo kwindow(activeWinId, NET::WMPid | NET::WMState, NET::WM2WindowClass);
        if (kwindow.valid() && (kwindow.pid() == QCoreApplication::applicationPid() || kwindow.hasState(NET::SkipTaskbar) || kwindow.windowClassName() == "dde-osd"))
            return;

        const int currScreenNum = QApplication::desktop()->screenNumber(q);

        if (activeWinId == 0)
        {
            const bool moreScreen = QApplication::screens().size() > 1;
            while (!activeIdStack.isEmpty())
            {
                const WId prevActiveId = activeIdStack.pop();
                if (XUtils::checkIfBadWindow(prevActiveId) || XUtils::checkIfWinMinimun(prevActiveId) || (moreScreen && XUtils::getWindowScreenNum(prevActiveId) != currScreenNum))
                    continue;

                activeWinId = prevActiveId;
                break;
            }
        }

        activeIdStack.removeAll(activeWinId);
        int activeWinScreenNum = QApplication::screens().size() > 1 ? XUtils::getWindowScreenNum(activeWinId) : -1;
        if (activeWinId > 0 && activeWinScreenNum >= 0 && activeWinScreenNum != currScreenNum)
        {
            if (XUtils::checkIfBadWindow(currActiveWinId) || currActiveWinId == activeWinId || XUtils::checkIfWinMinimun(currActiveWinId))
            {
                activeWinId = 0;
                while (!activeIdStack.isEmpty())
                {
                    const WId prevActiveId = activeIdStack.pop();
                    if (XUtils::checkIfBadWindow(prevActiveId) || currActiveWinId == prevActiveId || XUtils::checkIfWinMinimun(prevActiveId) || XUtils::getWindowScreenNum(prevActiveId) != currScreenNum)
                        continue;

                    activeWinId = prevActiveId;
                    break;
                }
            }
            else
                return;
        }

        QString activeWinTitle;
        QIcon appIcon;

        KWindowInfo kwin(activeWinId, NET::WMName | NET::WMPid | NET::WMWindowType, NET::WM2WindowClass | NET::WM2TransientFor);
        while (kwin.valid() && kwin.transientFor())
        {
            activeWinId = kwin.transientFor();
            kwin = KWindowInfo(activeWinId, NET::WMName | NET::WMPid | NET::WMWindowType, NET::WM2WindowClass | NET::WM2TransientFor);
        }

        if (activeWinId == currActiveWinId)
            return;

        if (kwin.valid())
        {
            if (kwin.windowType(NET::DesktopMask) == NET::Desktop /*kwin.windowClassName() == "dde-desktop"*/)
            {
                activeWinId = 0;
            }
            else
            {
                DesktopEntry entry;
                if (!kwin.windowClassName().isEmpty())
                    entry = DesktopEntryStat::instance()->getDesktopEntryByName(kwin.windowClassName());

                if (!entry && kwin.pid() > 0)
                    entry = DesktopEntryStat::instance()->getDesktopEntryByPid(kwin.pid());

                if (entry)
                {
                    activeWinTitle = entry->displayName;
                    if (!entry->icon.isNull())
                    {
                        if (entry->icon.startsWith("/"))
                            appIcon = QIcon(entry->icon);
                        else
                            appIcon = QIcon::fromTheme(entry->icon);
                    }
                }

                if (activeWinTitle.isEmpty())
                {
                    activeWinTitle = kwin.name();
                    if (activeWinTitle.contains(QRegExp("[–—-]")))
                    {
                        activeWinTitle = activeWinTitle.split(QRegExp("[–—-]")).last().trimmed();
                    }
                }
            }
        }
        else
            activeWinId = 0;

        menuBar->actions().at(4)->setText(activeWinId == 0 ? "桌面" : activeWinTitle);

        if (activeWinId == 0)
            appIcon = QIcon::fromTheme("dde-file-manager");
        else
            activeIdStack.push(activeWinId);

        menuBar->actions()[4]->menu()->actions().at(0)->setIcon(appIcon);
        currActiveWinId = activeWinId;
        setButtonsVisible(activeWinId != 0 && XUtils::checkIfWinMaximum(currActiveWinId));
        m_appMenuModel->setWinId(currActiveWinId, activeWinTitle);
    }

private:
    WId currActiveWinId;
    bool mouseClicked;
    QStack<WId> activeIdStack;

    AppMenuModel *m_appMenuModel;
    CustomizeMenubar *menuBar;
    QLineEdit *m_searchEdit;

    ActiveWindowControlWidget *q_ptr;
    Q_DECLARE_PUBLIC(ActiveWindowControlWidget)
};

ActiveWindowControlWidget::ActiveWindowControlWidget(QWidget *parent)
    : QWidget(parent), d_ptr(new ActiveWindowControlWidgetPrivate(this))
{
    Q_D(ActiveWindowControlWidget);
    QPalette palette = this->palette();
    palette.setColor(QPalette::Background, Qt::transparent);
    this->setPalette(palette);

    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);

    d->initMenuBar();

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    layout->setSpacing(10);
    layout->setMargin(0);
    layout->addWidget(d->menuBar, 0, Qt::AlignLeft | Qt::AlignVCenter);

    // detect whether active window maximized signal
    connect(KWindowSystem::self(), qOverload<WId, NET::Properties, NET::Properties2>(&KWindowSystem::windowChanged), d, &ActiveWindowControlWidgetPrivate::windowChanged);
    connect(KWindowSystem::self(), &KWindowSystem::activeWindowChanged, d, &ActiveWindowControlWidgetPrivate::activeWindowInfoChanged);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, d, qOverload<DGuiApplicationHelper::ColorType>(&ActiveWindowControlWidgetPrivate::themeTypeChanged));
    connect(CustomSettings::instance(), &CustomSettings::settingsChanged, d, qOverload<>(&ActiveWindowControlWidgetPrivate::themeTypeChanged));
    connect(CustomSettings::instance(), &CustomSettings::popupHoverChanged, d, &ActiveWindowControlWidgetPrivate::popupHoverChanged);

    QTimer::singleShot(100, this, [d] {
        d->themeTypeChanged();
        d->activeWindowInfoChanged(KWindowSystem::activeWindow());
    });
}

void ActiveWindowControlWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_D(ActiveWindowControlWidget);
    if (this->childAt(event->pos()) != d->menuBar)
        d->maximizeWindow();

    QWidget::mouseDoubleClickEvent(event);
}

void ActiveWindowControlWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        QWidget *pressedWidget = childAt(event->pos());
        if (pressedWidget == nullptr) {
            Q_D(ActiveWindowControlWidget);
            d->mouseClicked = true;
        }
    }
    QWidget::mousePressEvent(event);
}

void ActiveWindowControlWidget::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(ActiveWindowControlWidget);
    d->mouseClicked = false;
    QWidget::mouseReleaseEvent(event);
}

void ActiveWindowControlWidget::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(ActiveWindowControlWidget);
    if (d->mouseClicked)
    {
        d->mouseClicked = false;
        if (XUtils::checkIfWinMaximum(d->currActiveWinId))
        {
            #ifndef ENABLE_KDE_SUPPORT
                static const Atom constNetMoveResize = XInternAtom(QX11Info::display(), "_NET_WM_MOVERESIZE", False);
                XEvent xev;
                xev.xclient.type = ClientMessage;
                xev.xclient.message_type = constNetMoveResize;
                xev.xclient.display = QX11Info::display();
                xev.xclient.window = d->currActiveWinId;
                xev.xclient.format = 32;
                xev.xclient.data.l[0] = event->globalX() * this->devicePixelRatio();
                xev.xclient.data.l[1] = (this->height() + event->globalY()) * this->devicePixelRatio();
                xev.xclient.data.l[2] = 8; // NET::Move
                xev.xclient.data.l[3] = Button1;
                xev.xclient.data.l[4] = 0;
                //after ungrab, top-panel will not accept mouse event until you click the panel again
                // releaseMouse();
                XUngrabPointer(QX11Info::display(), QX11Info::appTime());
                XSendEvent(QX11Info::display(), QX11Info::appRootWindow(QX11Info::appScreen()), False,
                        SubstructureRedirectMask | SubstructureNotifyMask, &xev);
                XFlush(QX11Info::display());
                // XGrabPointer(QX11Info::display(), winId(), 1, PointerMotionMask | ButtonPressMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync, winId(), None, QX11Info::appTime());
            #else
                // XUngrabPointer(QX11Info::display(), QX11Info::appTime());
                NETRootInfo ri(QX11Info::connection(), NET::WMMoveResize);
                // NETRootInfo ri(QX11Info::display(), NET::WMMoveResize);
                ri.moveResizeRequest(
                    d->currActiveWinId,
                    event->globalX() * this->devicePixelRatio(),
                    (this->height() + event->globalY()) * this->devicePixelRatio(),
                    NET::Move);
            #endif
        }
    }
    QWidget::mouseMoveEvent(event);
}

bool ActiveWindowControlWidget::eventFilter(QObject *watched, QEvent *event) {
    Q_D(ActiveWindowControlWidget);
    if(watched == d->m_searchEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *e = static_cast<QKeyEvent*>(event);
        if(e->key() == Qt::Key_Left && d->m_searchEdit->cursorPosition() == 0) {
            d->menuBar->setActiveAction(d->menuBar->actions().at(d->menuBar->actions().count() -2));
            return true;
        } else if(e->key() == Qt::Key_Right && d->m_searchEdit->cursorPosition() == d->m_searchEdit->text().length()) {
            d->menuBar->setActiveAction(d->menuBar->actions().first());
            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}

void ActiveWindowControlWidget::toggleMenu(int id)
{
    Q_D(ActiveWindowControlWidget);
    if (id < -1 || id >= d->menuBar->actions().count())
        id = 0;

    QThread::msleep(150);
    if (id == -1)
        d->menuBar->setActiveAction(d->menuBar->actions().last());
    else
        d->menuBar->setActiveAction(d->menuBar->actions()[id]);
}

#include "ActiveWindowControlWidget.moc"