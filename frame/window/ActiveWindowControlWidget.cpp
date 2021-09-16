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
#include "QClickableLabel.h"
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
// #include <DStyle>
#include <QGSettings>
#include <QLineEdit>

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
    connect(this->m_appMenuModel, &AppMenuModel::clearMenu, this, [ = ]{
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

    connect(KWindowSystem::self(), &KWindowSystem::activeWindowChanged, this, [ this ](WId id) { this->activeWindowInfoChanged(); });

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &ActiveWindowControlWidget::themeTypeChanged);
    themeTypeChanged(DGuiApplicationHelper::instance()->themeType());
}

void ActiveWindowControlWidget::initMenuBar()
{
    this->menuBar = new CustomizeMenubar(this);
    this->menuBar->setFixedHeight(height());

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

    QAction *closeAction = new QAction(findIcon(QStyle::SP_TitleBarCloseButton), "关闭(&C)", this);
    closeAction->setVisible(false);
    connect(closeAction, &QAction::triggered, [ = ]{
        if(this->currActiveWinId > 0)
        {
            NETRootInfo ri( QX11Info::connection(), NET::CloseWindow );
            ri.closeWindowRequest( this->currActiveWinId );
        }
    });
    this->menuBar->addAction(closeAction);

    QAction *restoreAction = this->menuBar->addAction( "还原", [ = ]{
        this->maximizeWindow();
    });
    restoreAction->setIcon(findIcon(QStyle::SP_TitleBarNormalButton));
    restoreAction->setVisible(false);

    QAction *minimizeAction = this->menuBar->addAction("最小化", [ = ]{
        // this->m_appInter->MinimizeWindow(this->currActiveWinId);
        KWindowSystem::minimizeWindow(this->currActiveWinId);
    });
    minimizeAction->setIcon(findIcon(QStyle::SP_TitleBarMinButton));
    minimizeAction->setVisible(false);

    QMenu *startMenu = new QMenu;
        startMenu->addAction(QIcon::fromTheme("applications-other"), "关   于", [ = ](){
            QProcess::startDetached("/usr/bin/dde-file-manager", {"-p", "computer:///"});
            // QProcess::startDetached("/usr/bin/dbus-send --session --print-reply --dest=com.deepin.SessionManager /com/deepin/StartManager com.deepin.StartManager.RunCommand string:\"/usr/bin/dde-file-manager\" array:string:\"-p\",\"computer:///\"");
            // QProcess::startDetached("/usr/bin/qdbus", {"com.deepin.SessionManager", "/com/deepin/StartManager", "com.deepin.StartManager.RunCommand", "dde-file-manager", "(", "-p", "computer:///", ")"});
        });
        startMenu->addAction(QIcon::fromTheme("app-launcher"), "启动器", this, [ = ](){
            QProcess::startDetached("/usr/bin/qdbus", {"com.deepin.dde.Launcher", "/com/deepin/dde/Launcher", "com.deepin.dde.Launcher.Toggle"});
        });

        startMenu->addAction(QIcon::fromTheme("deepin-app-store"), "应用商店", [ = ](){
            // QProcess::startDetached("/usr/bin/qdbus", {"com.deepin.SessionManager", "/com/deepin/StartManager", "com.deepin.StartManager.LaunchApp", "/usr/share/applications/deepin-app-store.desktop", "0", "[]"});
            QProcess::startDetached("/usr/bin/deepin-home-appstore-client", {});
        });

        startMenu->addAction(QIcon::fromTheme("preferences-system"), "设    置", [ = ](){
            // QProcess::startDetached("/usr/bin/qdbus", {"com.deepin.SessionManager", "/com/deepin/StartManager", "com.deepin.StartManager.LaunchApp", "/usr/share/applications/dde-control-center.desktop", "0", "[]"});
            QProcess::startDetached("/usr/bin/dde-control-center", {"--show"});
        });

        startMenu->addAction(QIcon::fromTheme("deepin-system-monitor"), "资源管理器", [ = ](){
            // QProcess::startDetached("/usr/bin/qdbus", {"com.deepin.SessionManager", "/com/deepin/StartManager", "com.deepin.StartManager.LaunchApp", "/usr/share/applications/deepin-system-monitor.desktop", "0", "[]"});
            QProcess::startDetached("/usr/bin/deepin-system-monitor", {});
        });

        startMenu->addAction(QIcon::fromTheme("folder"), "文件管理器", [ = ](){
            QProcess::startDetached("/usr/bin/dde-file-manager", {"-O"});
        });

        startMenu->addAction(QIcon::fromTheme("gnome-panel-force-quit"), "强制关闭窗口", [ = ](){
            QProcess::startDetached("/usr/bin/xkill", {});
        });

        QMenu *shutdownMenu = new QMenu("关机");
        shutdownMenu->setIcon(QIcon::fromTheme("system-shutdown-symbolic"));
        shutdownMenu->addAction(QIcon::fromTheme("system-log-out-symbolic"), "注销", [](){
            DDBusSender().service("com.deepin.dde.shutdownFront").path("/com/deepin/dde/shutdownFront").interface("com.deepin.dde.shutdownFront").method("Logout").call();
        });
        shutdownMenu->addAction(QIcon::fromTheme("system-lock-screen-symbolic"), "锁定", [](){
            DDBusSender().service("com.deepin.dde.lockFront").path("/com/deepin/dde/lockFront").interface("com.deepin.dde.lockFront").method("Show").call();
        });
        shutdownMenu->addAction(QIcon::fromTheme("system-switch-user-symbolic"), "切换用户", [](){
            DDBusSender().service("com.deepin.dde.shutdownFront").path("/com/deepin/dde/shutdownFront").interface("com.deepin.dde.shutdownFront").method("SwitchUser").call();
        });
        shutdownMenu->addAction(QIcon::fromTheme("system-suspend-symbolic"), "待机", [](){
            DDBusSender().service("com.deepin.dde.shutdownFront").path("/com/deepin/dde/shutdownFront").interface("com.deepin.dde.shutdownFront").method("Suspend").call();
        });
        shutdownMenu->addAction(QIcon::fromTheme("system-reboot-symbolic"), "重启", [](){
            DDBusSender().service("com.deepin.dde.shutdownFront").path("/com/deepin/dde/shutdownFront").interface("com.deepin.dde.shutdownFront").method("Restart").call();
        });
        shutdownMenu->addAction(QIcon::fromTheme("system-shutdown-symbolic"), "关机", [](){
            DDBusSender().service("com.deepin.dde.shutdownFront").path("/com/deepin/dde/shutdownFront").interface("com.deepin.dde.shutdownFront").method("Shutdown").call();
        });

        startMenu->addMenu(shutdownMenu);

    startMenu->setIcon(QIcon::fromTheme("app-launcher"));

    this->menuBar->addMenu(startMenu);

    this->appMenu = new QMenu();
    this->appMenu->menuAction()->setObjectName("appMenu");
    connect(this->appMenu, &QMenu::aboutToShow, [ this ]{
        int index=1;
        for(auto role : {QAction::AboutRole, QAction::PreferencesRole})
        {
            this->appMenu->actions().at(index++)->setEnabled(this->m_appMenuModel->getAction(role));
        }
    });

    this->appMenu->addAction(QIcon::fromTheme("applications-other"), "关于包(&P)", [ = ]{
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

    this->appMenu->addAction(QIcon(), "关于(&A)", [ = ]{
        if(QAction *action = this->m_appMenuModel->getAction(QAction::AboutRole))
        {
            action->trigger();
        }
    });

    this->appMenu->addAction(QIcon(), "偏好设置(&S)", [ = ]{
        if(QAction *action = this->m_appMenuModel->getAction(QAction::PreferencesRole))
        {
            action->trigger();
        }
    });

    QAction *keepTopAction = new QAction(/*QIcon::fromTheme("top"),*/ "置顶", this);
    keepTopAction->setCheckable(true);
    connect(keepTopAction, &QAction::triggered, [ = ](bool checked){
        KWindowInfo kwin(this->currActiveWinId, NET::WMState);
        if(kwin.valid() && checked != kwin.hasState(NET::KeepAbove))
        {
            if (checked)
                KWindowSystem::setState(this->currActiveWinId, NET::KeepAbove);
            else
                KWindowSystem::clearState(this->currActiveWinId, NET::KeepAbove);
        }
    });
    this->appMenu->addAction(keepTopAction);

    this->appMenu->addAction(/*QIcon::fromTheme("window-close-symbolic"),*/ "关闭(&C)", [ = ]{
        this->menuBar->actions().at(0)->trigger();
    });
    // this->appMenu->addAction(closeAction);

    this->appMenu->addAction(/*QIcon::fromTheme("application-exit"),*/ "退出(&Q)", [ = ]{
        if(QAction *action = this->m_appMenuModel->getAction(QAction::QuitRole))
        {
            action->trigger();
        }
        else if (this->currActiveWinId > 0)
        {
            KWindowInfo info(this->currActiveWinId, NET::WMPid);
            int pid = info.pid();
            if(pid > 0)
                QProcess::startDetached("/usr/bin/kill", {"-9", QString::number(pid)});
        }
    });
    this->menuBar->addMenu(this->appMenu);

    QMenu *searchMenu = new QMenu("搜索(&S)", menuBar);
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

        QString searchTxt = searchEdit->text();

        if(searchTxt.length() > 0 && menuBar->actions().count() > 6)
        {
            QList<QAction *> list;
            std::function<void(QAction *)> searchFunc = [ &searchFunc, searchTxt, &list ](QAction *action) {
                if(action->menu())
                {
                    for(auto *subAction : action->menu()->actions())
                        searchFunc(subAction);
                }
                else if(action->text().contains(searchTxt))
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
    connect(searchMenu, &QMenu::aboutToHide, [ searchEdit ]{ searchEdit->clear(); });
    connect(searchMenu, &QMenu::aboutToShow, [ searchEdit ]{ searchEdit->setFocus(); });
    connect(searchEdit, &QLineEdit::textChanged, [ = ](const QString &searchTxt){
        searchTimer->start();
    });
    QWidgetAction *searchAction = new QWidgetAction(searchMenu);
    searchAction->setDefaultWidget(searchEdit);
    searchMenu->addAction(searchAction);

    this->menuBar->addMenu(searchMenu);

    for(auto item : this->menuBar->actions())
    {
        if(item->menu())
        {
            connect(item->menu(), &QMenu::aboutToShow, []{ DockItemManager::instance()->requestWindowAutoHide(false); });
            connect(item->menu(), &QMenu::aboutToHide, []{ DockItemManager::instance()->requestWindowAutoHide(true); });
        }
    }
}

void ActiveWindowControlWidget::activeWindowInfoChanged() {
    bool isDesktop = false;
    int activeWinId = XUtils::getFocusWindowId();
    // fix strange focus losing when pressing alt in some applications like chrome
    if (activeWinId == 0 || activeWinId == this->winId() || activeWinId == this->currActiveWinId) {
        return;
    }
    QString activeWinTitle;
    KWindowInfo kwin(activeWinId, NET::WMName|NET::WMPid | NET::WMState, NET::WM2WindowClass);
    if (kwin.valid())
    {
        if(kwin.pid() == QCoreApplication::applicationPid())
            return;

        if (kwin.windowClassName() == "dde-desktop")
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
                activeWinTitle = entry->displayName;

            if (activeWinTitle.isEmpty())
            {
                activeWinTitle = kwin.name();
                if (activeWinTitle.contains(QRegExp("[–—-]"))) {
                    activeWinTitle = activeWinTitle.split(QRegExp("[–—-]")).last().trimmed();
                }
            }
        }
    }

    int currScreenNum = this->currScreenNum();
    int activeWinScreenNum = XUtils::getWindowScreenNum(activeWinId);
    if (activeWinScreenNum >= 0 && activeWinScreenNum != currScreenNum) {
        if (XUtils::checkIfBadWindow(this->currActiveWinId) || this->currActiveWinId == activeWinId || XUtils::checkIfWinMinimun(this->currActiveWinId)) {
            int newCurActiveWinId = -1;
            while (!this->activeIdStack.isEmpty()) {
                int prevActiveId = this->activeIdStack.pop();
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


    this->appMenu->menuAction()->setText(isDesktop ? "桌面" : activeWinTitle);

    QIcon appIcon;
    if (isDesktop)
    {
        appIcon = QIcon::fromTheme("dde-file-manager");
    } else {
        this->activeIdStack.push(activeWinId);
        appIcon.addPixmap(XUtils::getWindowIconNameX11(activeWinId));
        if (kwin.valid())
        {
            this->appMenu->actions().at(3)->setChecked(kwin.hasState(NET::KeepAbove));
        }
    }

    this->appMenu->actions().at(0)->setIcon(appIcon);

    this->currActiveWinId = activeWinId;
    this->setButtonsVisible(!isDesktop && XUtils::checkIfWinMaximum(this->currActiveWinId));

    this->m_appMenuModel->setWinId(this->currActiveWinId, isDesktop);
    int index = 3;
    while(index < 6)
    {
        this->appMenu->actions().at(index++)->setDisabled(isDesktop);
    }
}

void ActiveWindowControlWidget::setButtonsVisible(bool visible) {
    if (visible != this->menuBar->actions().first()->isVisible())
    {
        int i = 0;
        while (i < 3)
        {
            QTimer::singleShot(50 * i, [ = ] {
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
    if (wId != this->currActiveWinId)
        return;

    if (properties.testFlag(NET::WMState))
    {
        KWindowInfo kwin(this->currActiveWinId, NET::WMState);
        if(kwin.valid())
        {
            this->appMenu->actions().at(3)->setChecked(kwin.hasState(NET::KeepAbove));
            this->setButtonsVisible(kwin.hasState(NET::Max));
        }
    }

    // this->setButtonsVisible(XUtils::checkIfWinMaximum(this->currActiveWinId));
}

void ActiveWindowControlWidget::themeTypeChanged(DGuiApplicationHelper::ColorType themeType){
    QGSettings iconThemeSettings("com.deepin.dde.appearance");
    QString color;
    QString iconTheme;
    // QString subfix;
    switch (themeType)
    {
        case DGuiApplicationHelper::LightType:
            color = "black";
            iconTheme = "bloom";
            // subfix = "";
            break;
        case DGuiApplicationHelper::DarkType:
        default:
            color = "white";
            iconTheme = "bloom-dark";
            // subfix = "-white";
            break;
    }
    // this->closeButton->setIcon(QIcon(QString(":/icons/close%1.svg").arg(subfix)));
    // this->maxButton->setIcon(QIcon(QString(":/icons/maximum%1.svg").arg(subfix)));
    // this->minButton->setIcon(QIcon(QString(":/icons/minimum%1.svg").arg(subfix)));

    if(iconThemeSettings.get("theme-auto").toBool())
        iconThemeSettings.set("icon-theme", iconTheme);
    this->menuBar->setStyleSheet(QString("QMenuBar { font-size: 15px; color: %1; background-color: rgba(0, 0, 0, 0); margin: 0 0 0 0; }\
        QMenuBar::item { padding-top: 3px; padding-right: 4px; padding-left: 3px;}").arg(color));

    qDebug()<<"QmenuBar size: "<<this->menuBar->size();
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
        if (XUtils::checkIfWinMaximum(this->currActiveWinId)) {
            NETRootInfo ri(QX11Info::connection(), NET::WMMoveResize);
            ri.moveResizeRequest(
                    this->currActiveWinId,
                    event->globalX() * this->devicePixelRatio(),
                    (this->height() + event->globalY()) * this->devicePixelRatio(),
                    NET::Move
            );
            this->mouseClicked = false;
        }
    }
    QWidget::mouseMoveEvent(event);
}

int ActiveWindowControlWidget::currScreenNum() {
    return QApplication::desktop()->screenNumber(this);
}

void ActiveWindowControlWidget::toggleMenu(int id) {
    if(id < -1 || id >= this->menuBar->actions().count()){
        id = 3;
    }
    QThread::msleep(150);
    if(id == -1)
        this->menuBar->setActiveAction(this->menuBar->actions().last());
    else
        this->menuBar->setActiveAction(this->menuBar->actions()[id]);
}
