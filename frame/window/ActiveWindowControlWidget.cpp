//
// Created by septemberhx on 2020/5/26.
//

#include <QDebug>
#include "ActiveWindowControlWidget.h"
#include "util/XUtils.h"
#include <QMouseEvent>
#include <NETWM>
#include <QtX11Extras/QX11Info>
#include <QProcess>
#include "../util/desktop_entry_stat.h"
#include "AboutWindow.h"
#include "util/TopPanelSettings.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QGuiApplication>
#include <QScreen>
#include <QEvent>
#include <DDBusSender>
#include <QToolButton>
#include <QStyle>
#include <QGSettings>
#include <QLineEdit>
#include <QRandomGenerator>

ActiveWindowControlWidget::ActiveWindowControlWidget(QWidget *parent)
    : QWidget(parent)
    , mouseClicked(false)
{
    QPalette palette = this->palette();
    palette.setColor(QPalette::Background, Qt::transparent);
    this->setPalette(palette);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    layout->setSpacing(10);
    layout->setMargin(0);

    this->initMenuBar();

    layout->addWidget(this->menuBar);
    layout->addStretch();

    this->m_appMenuModel = new AppMenuModel(this);
    connect(this->m_appMenuModel, &AppMenuModel::clearMenu, this, [ this ]{
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
    connect(this->m_appMenuModel, &AppMenuModel::modelNeedsUpdate, this, &ActiveWindowControlWidget::updateMenu);
    connect(this->m_appMenuModel, &AppMenuModel::requestActivateIndex, this, [ this ](int index){
        if(this->menuBar->actions().size() - 6 > index && index >= 0){
            this->toggleMenu(index + 5);
        }
    });
    connect(this->m_appMenuModel, &AppMenuModel::actionRemoved, [ this ](QAction *action){
        this->menuBar->removeAction(action);
            if(action->menu())
            {
               disconnect(action->menu(), &QMenu::aboutToShow, this, 0);
               disconnect(action->menu(), &QMenu::aboutToHide, this, 0);
            }
    });
    connect(this->m_appMenuModel, &AppMenuModel::actionAdded, [ this ](QAction *action, QAction *before){
        if(!this->menuBar->actions().contains(before))
            before = this->menuBar->actions().last();
        this->menuBar->insertAction(before, action);
        if(action->menu())
        {
            connect(action->menu(), &QMenu::aboutToShow, []{ DockItemManager::instance()->requestWindowAutoHide(false); });
            connect(action->menu(), &QMenu::aboutToHide, []{ DockItemManager::instance()->requestWindowAutoHide(true); });
        }
    });

    // layout->addStretch();

    // detect whether active window maximized signal
    connect(KWindowSystem::self(), qOverload<WId, NET::Properties, NET::Properties2>(&KWindowSystem::windowChanged), this, &ActiveWindowControlWidget::windowChanged);

    connect(KWindowSystem::self(), &KWindowSystem::activeWindowChanged, this, &ActiveWindowControlWidget::activeWindowInfoChanged);

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &ActiveWindowControlWidget::themeTypeChanged);
    connect(CustomSettings::instance(), &CustomSettings::settingsChanged, this, [ this ] { themeTypeChanged(DGuiApplicationHelper::instance()->themeType()); });
    themeTypeChanged(DGuiApplicationHelper::instance()->themeType());
    QTimer::singleShot(1000, this, [this] { this->activeWindowInfoChanged(KWindowSystem::activeWindow()); });

    connect(CustomSettings::instance(), &CustomSettings::popupHoverChanged, this, [ this ](bool popupHover){
        if(popupHover)
            connect(this->menuBar, &CustomizeMenubar::hovered, this, [ this ](QAction *action){ if(action->menu() && this->menuBar->actions().contains(action)) this->menuBar->setActiveAction(action); });
        else
            disconnect(this->menuBar, &CustomizeMenubar::hovered, this, 0);
     });
}

void ActiveWindowControlWidget::initMenuBar()
{
    this->menuBar = new CustomizeMenubar(this);
    this->menuBar->setFixedHeight(height());

    QString logo = CustomSettings::instance()->getLogoImg();

    QMenu *startMenu = this->menuBar->addMenu(!logo.isNull() ? QIcon(logo) : QIcon::fromTheme("app-launcher"), "??????");
    startMenu->menuAction()->setObjectName("logo");
        startMenu->addAction(QIcon::fromTheme("applications-other"), "???   ???", []{
            QProcess::startDetached("/usr/bin/dde-file-manager", {"-p", "computer:///"});
            // QProcess::startDetached("/usr/bin/dbus-send --session --print-reply --dest=com.deepin.SessionManager /com/deepin/StartManager com.deepin.StartManager.RunCommand string:\"/usr/bin/dde-file-manager\" array:string:\"-p\",\"computer:///\"");
            // QProcess::startDetached("/usr/bin/qdbus", {"com.deepin.SessionManager", "/com/deepin/StartManager", "com.deepin.StartManager.RunCommand", "dde-file-manager", "(", "-p", "computer:///", ")"});
        });
        startMenu->addAction(QIcon::fromTheme("app-launcher"), "?????????", this, []{
            QProcess::startDetached("/usr/bin/qdbus", {"com.deepin.dde.Launcher", "/com/deepin/dde/Launcher", "com.deepin.dde.Launcher.Toggle"});
        });

        startMenu->addAction(QIcon::fromTheme("deepin-app-store"), "????????????", []{
            // QProcess::startDetached("/usr/bin/qdbus", {"com.deepin.SessionManager", "/com/deepin/StartManager", "com.deepin.StartManager.LaunchApp", "/usr/share/applications/deepin-app-store.desktop", "0", "[]"});
            QProcess::startDetached("/usr/bin/deepin-home-appstore-client", {});
        });

        startMenu->addAction(QIcon::fromTheme("preferences-system"), "???    ???", []{
            // QProcess::startDetached("/usr/bin/qdbus", {"com.deepin.SessionManager", "/com/deepin/StartManager", "com.deepin.StartManager.LaunchApp", "/usr/share/applications/dde-control-center.desktop", "0", "[]"});
            QProcess::startDetached("/usr/bin/dde-control-center", {"--show"});
        });

        startMenu->addAction(QIcon::fromTheme("deepin-system-monitor"), "???????????????", []{
            // QProcess::startDetached("/usr/bin/qdbus", {"com.deepin.SessionManager", "/com/deepin/StartManager", "com.deepin.StartManager.LaunchApp", "/usr/share/applications/deepin-system-monitor.desktop", "0", "[]"});
            QProcess::startDetached("/usr/bin/deepin-system-monitor", {});
        });

        startMenu->addAction(QIcon::fromTheme("folder"), "???????????????", []{
            QProcess::startDetached("/usr/bin/dde-file-manager", {"-O"});
        });

        startMenu->addAction(QIcon::fromTheme("gnome-panel-force-quit"), "??????????????????", []{
            QProcess::startDetached("/usr/bin/xkill", {});
        });

        QMenu *systemMenu = startMenu->addMenu(QIcon::fromTheme("preferences-theme"), "????????????");
        QActionGroup *group = new QActionGroup(systemMenu);

        QAction *autoAction = systemMenu->addAction("??????");
        autoAction->setCheckable(true);autoAction->setData("deepin-auto");
        QAction *lightAction = systemMenu->addAction("??????");
        lightAction->setCheckable(true);lightAction->setData("deepin");
        QAction *darkAction = systemMenu->addAction("??????");
        darkAction->setCheckable(true);darkAction->setData("deepin-dark");

        group->addAction(autoAction);
        group->addAction(lightAction);
        group->addAction(darkAction);

        connect(group, &QActionGroup::triggered, this, [](QAction *action){
            QGSettings iconThemeSettings("com.deepin.dde.appearance");
            iconThemeSettings.set("gtk-theme", action->data().toString());
         });

        connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, [group]{
            QGSettings iconThemeSettings("com.deepin.dde.appearance");
            QString theme = iconThemeSettings.get("gtk-theme").toString();
            for(auto action : group->actions())
            {
                if(action->data() == theme)
                {
                    action->setChecked(true);
                    break;
                }
            }
        });

        QMenu *shutdownMenu = startMenu->addMenu(QIcon::fromTheme("system-shutdown-symbolic"), "??????");
        shutdownMenu->addAction(QIcon::fromTheme("system-log-out-symbolic"), "??????", []{
            DDBusSender().service("com.deepin.dde.shutdownFront").path("/com/deepin/dde/shutdownFront").interface("com.deepin.dde.shutdownFront").method("Logout").call();
        });
        shutdownMenu->addAction(QIcon::fromTheme("system-lock-screen-symbolic"), "??????", []{
            DDBusSender().service("com.deepin.dde.lockFront").path("/com/deepin/dde/lockFront").interface("com.deepin.dde.lockFront").method("Show").call();
        });
        shutdownMenu->addAction(QIcon::fromTheme("system-switch-user-symbolic"), "????????????", []{
            DDBusSender().service("com.deepin.dde.shutdownFront").path("/com/deepin/dde/shutdownFront").interface("com.deepin.dde.shutdownFront").method("SwitchUser").call();
        });
        shutdownMenu->addAction(QIcon::fromTheme("system-suspend-symbolic"), "??????", []{
            DDBusSender().service("com.deepin.dde.shutdownFront").path("/com/deepin/dde/shutdownFront").interface("com.deepin.dde.shutdownFront").method("Suspend").call();
        });
        shutdownMenu->addAction(QIcon::fromTheme("system-reboot-symbolic"), "??????", []{
            DDBusSender().service("com.deepin.dde.shutdownFront").path("/com/deepin/dde/shutdownFront").interface("com.deepin.dde.shutdownFront").method("Restart").call();
        });
        shutdownMenu->addAction(QIcon::fromTheme("system-shutdown-symbolic"), "??????", []{
            DDBusSender().service("com.deepin.dde.shutdownFront").path("/com/deepin/dde/shutdownFront").interface("com.deepin.dde.shutdownFront").method("Shutdown").call();
        });

    this->menuBar->addAction("??????", [ this ]{
        if(this->currActiveWinId > 0)
        {
            NETRootInfo ri( QX11Info::connection(), NET::CloseWindow );
            ri.closeWindowRequest( this->currActiveWinId );
        }
    });

    this->menuBar->addAction( "??????", this, &ActiveWindowControlWidget::maximizeWindow);

    this->menuBar->addAction("?????????", [ this ]{
        // this->m_appInter->MinimizeWindow(this->currActiveWinId);
        KWindowSystem::minimizeWindow(this->currActiveWinId);
    });

    QMenu *appMenu = this->menuBar->addMenu("??????");
    appMenu->menuAction()->setObjectName("appMenu");
    connect(appMenu, &QMenu::aboutToShow, [ this, appMenu ]{
        KWindowInfo kwin(this->currActiveWinId, NET::WMState, NET::WM2WindowClass);
        if(kwin.valid())
        {
            int index = 1;
            if(kwin.windowClassName() == "dde-desktop")
            {
                while(index < 6)
                {
                    appMenu->actions().at(index++)->setDisabled(true);
                }
                appMenu->actions().at(3)->setChecked(false);
            }
            else
            {
                for(auto role : {QAction::AboutRole, QAction::PreferencesRole})
                {
                    appMenu->actions().at(index++)->setEnabled(this->m_appMenuModel->getAction(role));
                }
                while(index < 6)
                {
                    appMenu->actions().at(index++)->setEnabled(true);
                }
                appMenu->actions().at(3)->setChecked(kwin.hasState(NET::KeepAbove));
            }
        }
    });

    appMenu->addAction(QIcon::fromTheme("applications-other"), "?????????(&P)", [ this ]{
        KWindowInfo kwin(this->currActiveWinId, NET::WMName|NET::WMDesktop|NET::WMPid, NET::WM2DesktopFileName|NET::WM2ClientMachine|NET::WM2WindowClass);
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

    appMenu->addAction(QIcon(), "??????(&A)", [ this ]{
        if(QAction *action = this->m_appMenuModel->getAction(QAction::AboutRole))
        {
            action->trigger();
        }
    });

    appMenu->addAction(QIcon(), "????????????(&S)", [ this ]{
        if(QAction *action = this->m_appMenuModel->getAction(QAction::PreferencesRole))
        {
            action->trigger();
        }
    });

    QAction *keepTopAction = appMenu->addAction(/*QIcon::fromTheme("top"),*/ "??????");
    keepTopAction->setCheckable(true);
    connect(keepTopAction, &QAction::triggered, [ this ](bool checked){
        KWindowInfo kwin(this->currActiveWinId, NET::WMState);
        if(kwin.valid() && checked != kwin.hasState(NET::KeepAbove))
        {
            if (checked)
                KWindowSystem::setState(this->currActiveWinId, NET::KeepAbove);
            else
                KWindowSystem::clearState(this->currActiveWinId, NET::KeepAbove);
        }
    });

    appMenu->addAction(/*QIcon::fromTheme("window-close-symbolic"),*/ "??????(&C)", this->menuBar->actions().at(1), &QAction::trigger);
    appMenu->addAction(/*QIcon::fromTheme("application-exit"),*/ "??????(&Q)", [ this ]{
        if(QAction *action = this->m_appMenuModel->getAction(QAction::QuitRole))
        {
            action->trigger();
        }
        else if (this->currActiveWinId > 0)
        {
            KWindowInfo info(this->currActiveWinId, NET::WMPid);
            int pid = info.valid() ? info.pid() : 0;
            if(pid > 0)
                QProcess::startDetached("/usr/bin/kill", {"-9", QString::number(pid)});
        }
    });

    QMenu *searchMenu = this->menuBar->addMenu("??????(&S)");
    QLineEdit *searchEdit = new QLineEdit(searchMenu);
    searchEdit->setAttribute(Qt::WA_TranslucentBackground);
    searchEdit->setFixedWidth(240);
    QTimer *searchTimer = new QTimer(this);
    searchTimer->setSingleShot(true);
    searchTimer->setInterval(100);
    connect(searchTimer, &QTimer::timeout, [searchMenu, searchEdit, this]{
        while (searchMenu->actions().count() > 1)
        {
            searchMenu->removeAction(searchMenu->actions()[1]);
        }

        QString searchTxt = searchEdit->text().toLower();

        if(searchTxt.length() > 0 && menuBar->actions().count() > 6)
        {
            QList<QAction *> list;
            std::function<void(QAction *)> searchFunc = [ &searchFunc, searchTxt, &list ](QAction *action) {
                if(action->menu())
                {
                    for(auto *subAction : action->menu()->actions())
                        searchFunc(subAction);
                }
                else if(action->text().toLower().contains(searchTxt))
                {
                    list.append(action);
                }
            };

            for(int i=5; i < menuBar->actions().count() - 1; i++)
            {
                searchFunc(menuBar->actions().at(i));
            }

            if(list.count() > 0)
            {
                searchMenu->addActions(list);
            }
        }
    });
    connect(searchMenu, &QMenu::aboutToHide, searchEdit, &QLineEdit::clear);
    // connect(searchMenu, &QMenu::aboutToShow, [searchEdit]{ searchEdit->setFocus(); });
    connect(searchEdit, &QLineEdit::textChanged, [searchTimer] { searchTimer->start(); });
    QWidgetAction *searchAction = new QWidgetAction(searchMenu);
    searchAction->setDefaultWidget(searchEdit);
    searchMenu->addAction(searchAction);

    for(auto item : this->menuBar->actions())
    {
        if(item->menu())
        {
            connect(item->menu(), &QMenu::aboutToShow, []{ DockItemManager::instance()->requestWindowAutoHide(false); });
            connect(item->menu(), &QMenu::aboutToHide, []{ DockItemManager::instance()->requestWindowAutoHide(true); });
        }
    }

    if(CustomSettings::instance()->getPopupHover())
        connect(this->menuBar, &CustomizeMenubar::hovered, this, [ this ](QAction *action){ if(action->menu() && this->menuBar->actions().contains(action)) this->menuBar->setActiveAction(action); });
}

void ActiveWindowControlWidget::activeWindowInfoChanged(WId activeWinId) {
    bool isDesktop = false;
    if (activeWinId == 0 || activeWinId == this->winId() || activeWinId == this->currActiveWinId) {
        return;
    }
    KWindowInfo kwin(activeWinId, NET::WMPid | NET::WMState);
    if (kwin.valid() && (kwin.pid() == QCoreApplication::applicationPid() || kwin.hasState(NET::SkipTaskbar)))
        return;

    this->activeIdStack.removeAll(activeWinId);

    int currScreenNum = QApplication::desktop()->screenNumber(this);
    int activeWinScreenNum = QApplication::screens().size() > 1 ? XUtils::getWindowScreenNum(activeWinId) : -1;
    if (activeWinScreenNum >= 0 && activeWinScreenNum != currScreenNum) {
        if (XUtils::checkIfBadWindow(this->currActiveWinId) || this->currActiveWinId == activeWinId || XUtils::checkIfWinMinimun(this->currActiveWinId)) {
            WId newCurActiveWinId = -1;
            while (!this->activeIdStack.isEmpty()) {
                WId prevActiveId = this->activeIdStack.pop();
                if (XUtils::checkIfBadWindow(prevActiveId) || this->currActiveWinId == prevActiveId || XUtils::checkIfWinMinimun(prevActiveId)) {
                    continue;
                }

                int sNum = XUtils::getWindowScreenNum(prevActiveId);
                if (sNum != currScreenNum) {
                    continue;
                }

                newCurActiveWinId = prevActiveId;
                break;
            }

            if (newCurActiveWinId < 0) {
                isDesktop = true;
                activeWinId = 0;
            } else {
                activeWinId = newCurActiveWinId;
            }
        }
        else
        {
            return;
        }
    }


    QString activeWinTitle;
    QIcon appIcon;
    if(!isDesktop)
    {
        KWindowInfo kwin(activeWinId, NET::WMName | NET::WMPid, NET::WM2WindowClass);
        if (kwin.valid())
        {
            if(kwin.windowClassName() == "dde-osd")
            {
                return ;
            } else if (kwin.windowClassName() == "dde-desktop")
            {
                isDesktop = true;
            }
            else
            {
                DesktopEntry entry;
                if(!kwin.windowClassName().isEmpty())
                    entry = DesktopEntryStat::instance()->getDesktopEntryByName(kwin.windowClassName());

                if (!entry && kwin.pid() > 0)
                    entry = DesktopEntryStat::instance()->getDesktopEntryByPid(kwin.pid());

                if (entry)
                {
                    activeWinTitle = entry->displayName;
                    if(!entry->icon.isNull())
                    {
                        if(entry->icon.startsWith("/"))
                            appIcon = QIcon(entry->icon);
                        else
                            appIcon = QIcon::fromTheme(entry->icon);
                    }
                }

                if (activeWinTitle.isEmpty())
                {
                    activeWinTitle = kwin.name();
                    if (activeWinTitle.contains(QRegExp("[??????-]"))) {
                        activeWinTitle = activeWinTitle.split(QRegExp("[??????-]")).last().trimmed();
                    }
                }
            }
        }
    }


    this->menuBar->actions().at(4)->setText(isDesktop ? "??????" : activeWinTitle);

    if (isDesktop)
    {
        appIcon = QIcon::fromTheme("dde-file-manager");
    } else {
        // if(appIcon.isNull())
            // appIcon.addPixmap(XUtils::getWindowIconNameX11(activeWinId));
    }

    this->activeIdStack.push(activeWinId);
    this->menuBar->actions()[4]->menu()->actions().at(0)->setIcon(appIcon);

    this->currActiveWinId = activeWinId;
    this->setButtonsVisible(!isDesktop && XUtils::checkIfWinMaximum(this->currActiveWinId));

    this->m_appMenuModel->setWinId(this->currActiveWinId, isDesktop, activeWinTitle);
}

void ActiveWindowControlWidget::setButtonsVisible(bool visible) {
    if (visible != this->menuBar->actions().at(1)->isVisible())
    {
        int i = 1;
        while (i < 4)
        {
            QTimer::singleShot(100 * i, [ this, visible, i ] {
                this->menuBar->actions().at(i)->setVisible(visible);
            });
            i++;
        }
    }
}

void ActiveWindowControlWidget::maximizeWindow() {
    if (XUtils::checkIfWinMaximum(this->currActiveWinId)) {
        XUtils::unmaximizeWindow(this->currActiveWinId);

        // checkIfWinMaximum is MUST needed here.
        //   I don't know whether XSendEvent is designed like this,
        //   my test shows unmaximizeWindow by XSendEvent will work when others try to fetch its properties.
        //   i.e., checkIfWinMaximum
        XUtils::checkIfWinMaximum(this->currActiveWinId);

        // this->m_appInter->MinimizeWindow(this->currActiveWinId);
    } else {
        // sadly the dbus maximizeWindow cannot unmaximize window :(
        // this->m_appInter->MaximizeWindow(this->currActiveWinId);
        XUtils::maximizeWindow(this->currActiveWinId);
        XUtils::checkIfWinMaximum(this->currActiveWinId);
    }
}

void ActiveWindowControlWidget::mouseDoubleClickEvent(QMouseEvent *event) {
    if (this->childAt(event->pos()) != this->menuBar) {
        this->maximizeWindow();
    }
    QWidget::mouseDoubleClickEvent(event);
}

void ActiveWindowControlWidget::updateMenu() {
    if (QMenu *menu = m_appMenuModel->menu())
    {
        this->menuBar->insertActions(this->menuBar->actions().at(5), menu->actions());
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
}

void ActiveWindowControlWidget::windowChanged(WId wId, NET::Properties properties, NET::Properties2 properties2) {
        if (KWindowSystem::activeWindow() == wId && properties.testFlag(NET::WMGeometry) && QApplication::screens().size() > 1) {
            int currScreenNum = QApplication::desktop()->screenNumber(qobject_cast<QWidget *>(this->parent()));
            int activeWinScreenNum = XUtils::getWindowScreenNum(wId);
            if (activeWinScreenNum >= 0 && activeWinScreenNum != currScreenNum) {
                if (this->currActiveWinId == wId) {
                    this->activeWindowInfoChanged(wId);
                }
            } else if(wId != this->currActiveWinId){
                this->activeWindowInfoChanged(wId);
            }
        }

    if (wId == this->currActiveWinId && properties.testFlag(NET::WMState))
    {
        KWindowInfo kwin(this->currActiveWinId, NET::WMState);
        if(kwin.valid())
        {
            this->setButtonsVisible(kwin.hasState(NET::Max));
        }
    }
}

void ActiveWindowControlWidget::themeTypeChanged(DGuiApplicationHelper::ColorType themeType){
    CustomSettings *settings = CustomSettings::instance();

    if(settings->isButtonCustom())
    {
        this->menuBar->actions().at(1)->setIcon(QIcon(settings->getActiveCloseIconPath()));
        this->menuBar->actions().at(2)->setIcon(QIcon(settings->getActiveUnmaximizedIconPath()));
        this->menuBar->actions().at(3)->setIcon(QIcon(settings->getActiveMinimizedIconPath()));
    }
    else
    {
        // QString subfix = themeType == DGuiApplicationHelper::LightType ? "" : "-white";
        // this->closeButton->setIcon(QIcon(QString(":/icons/close%1.svg").arg(subfix)));
        // this->maxButton->setIcon(QIcon(QString(":/icons/maximum%1.svg").arg(subfix)));
        // this->minButton->setIcon(QIcon(QString(":/icons/minimum%1.svg").arg(subfix)));

        auto findIcon = [this](QStyle::StandardPixmap button){
            QIcon icon, retIcon;
            QPixmap pixmap;

            if(true)
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
                icon = style()->standardIcon(button);
                pixmap = icon.pixmap(40, QIcon::Normal, QIcon::On).copy(13, 13, 14, 14).scaled(24, 24);retIcon.addPixmap(pixmap, QIcon::Normal, QIcon::On);
                pixmap = icon.pixmap(40, QIcon::Active, QIcon::On).copy(13, 13, 14, 14).scaled(24, 24);retIcon.addPixmap(pixmap, QIcon::Active, QIcon::On);
                pixmap = icon.pixmap(40, QIcon::Selected, QIcon::On).copy(13, 13, 14, 14).scaled(24, 24);retIcon.addPixmap(pixmap, QIcon::Selected, QIcon::On);
            }
            return retIcon;
        };

        this->menuBar->actions().at(1)->setIcon(findIcon(QStyle::SP_TitleBarCloseButton));
        this->menuBar->actions().at(2)->setIcon(findIcon(QStyle::SP_TitleBarNormalButton));
        this->menuBar->actions().at(3)->setIcon(findIcon(QStyle::SP_TitleBarMinButton));
    }

    QGSettings iconThemeSettings("com.deepin.dde.appearance");
    if(CustomSettings::instance()->isIconThemeFollowSystem())
    {
        iconThemeSettings.set("icon-theme", themeType == DGuiApplicationHelper::LightType ? "bloom" : "bloom-dark");
    }

    QString color;

    if(settings->isPanelCustom())
        color = settings->getActiveFontColor().name();
    else
        color = themeType == DGuiApplicationHelper::LightType ? "black" : "white";
    int padding = QRandomGenerator::system()->bounded(1);
    this->menuBar->setStyleSheet(QString("QMenuBar {padding-left: %1; font-size: 15px; color: %2; background-color: rgba(0, 0, 0, 0); spacing: 1px; }\
        QMenuBar::item { padding-top: 3px; padding-bottom: 3px; padding-left: 5px; padding-right: 5px; }\
        QMenuBar::item:selected { border-radius: 4px; color: palette(highlighted-text); background-color: palette(highlight); }").arg(padding).arg(color));
}

void ActiveWindowControlWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        QWidget *pressedWidget = childAt(event->pos());
        if (pressedWidget == nullptr) {
            this->mouseClicked = !this->mouseClicked;
        }
    }
    QWidget::mousePressEvent(event);
}

void ActiveWindowControlWidget::mouseReleaseEvent(QMouseEvent *event) {
    this->mouseClicked = false;
    QWidget::mouseReleaseEvent(event);
}

void ActiveWindowControlWidget::mouseMoveEvent(QMouseEvent *event) {
    if (this->mouseClicked) {
        this->mouseClicked = false;
        if (XUtils::checkIfWinMaximum(this->currActiveWinId)) {
            NETRootInfo ri(QX11Info::connection(), NET::WMMoveResize);
            ri.moveResizeRequest(
                    this->currActiveWinId,
                    event->globalX() * this->devicePixelRatio(),
                    (this->height() + event->globalY()) * this->devicePixelRatio(),
                    NET::Move
            );
        }
    }
    QWidget::mouseMoveEvent(event);
}

void ActiveWindowControlWidget::toggleMenu(int id) {
    if(id < -1 || id >= this->menuBar->actions().count()){
        id = 0;
    }
    QThread::msleep(150);
    if(id == -1)
        this->menuBar->setActiveAction(this->menuBar->actions().last());
    else
        this->menuBar->setActiveAction(this->menuBar->actions()[id]);
}
