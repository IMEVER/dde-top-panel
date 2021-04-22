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

#include <QApplication>
// #include <QQmlContext>
// #include <QQuickWindow>
#include <QGuiApplication>
#include <QScreen>
#include <QEvent>
#include <DDBusSender>

ActiveWindowControlWidget::ActiveWindowControlWidget(QWidget *parent)
    : QWidget(parent)
    , m_appInter(new DBusDock("com.deepin.dde.daemon.Dock", "/com/deepin/dde/daemon/Dock", QDBusConnection::sessionBus(), this))
    , mouseClicked(false)
{
    QPalette palette = this->palette();
    palette.setColor(QPalette::Background, Qt::transparent);
    this->setPalette(palette);

    QHBoxLayout *m_layout = new QHBoxLayout(this);
    m_layout->setAlignment(Qt::AlignLeft);
    m_layout->setSpacing(10);
    m_layout->setContentsMargins(0, 0, 0, 0);

    int buttonSize = 18;
    this->m_buttonWidget = new QWidget(this);
    this->m_buttonWidget->hide();
    this->m_buttonWidget->setLayout(new QHBoxLayout());
    this->m_buttonWidget->layout()->setSpacing(12);
    this->m_buttonWidget->layout()->setContentsMargins(0, 0, 0, 0);

    this->closeButton = new QToolButton(this->m_buttonWidget);
    this->closeButton->setToolTip("关闭");
    this->closeButton->setFixedSize(buttonSize, buttonSize);
    this->closeButton->setIcon(QIcon(":/icons/close.svg"));
    this->closeButton->setIconSize(QSize(buttonSize - 8, buttonSize - 8));
    this->m_buttonWidget->layout()->addWidget(this->closeButton);

    this->maxButton = new QToolButton(this->m_buttonWidget);
    this->maxButton->setToolTip("还原");
    this->maxButton->setFixedSize(buttonSize, buttonSize);
    this->maxButton->setIcon(QIcon(":/icons/maximum.svg"));
    this->maxButton->setIconSize(QSize(buttonSize - 8, buttonSize - 8));
    this->m_buttonWidget->layout()->addWidget(this->maxButton);

    this->minButton = new QToolButton(this->m_buttonWidget);
    this->minButton->setToolTip("最小化");
    this->minButton->setFixedSize(buttonSize, buttonSize);
    this->minButton->setIcon(QIcon(":/icons/minimum.svg"));
    this->minButton->setIconSize(QSize(buttonSize - 8, buttonSize - 8));
    this->m_buttonWidget->layout()->addWidget(this->minButton);
    m_layout->addWidget(this->m_buttonWidget);

    this->initMenuBar(m_layout);

    this->m_appMenuModel = new AppMenuModel(this);
    connect(this->m_appMenuModel, &AppMenuModel::clearMenu, this, [ = ](){
        while (this->menuBar->actions().size() > 2)
        {
            this->menuBar->removeAction(this->menuBar->actions()[2]);
        }
    });
    connect(this->m_appMenuModel, &AppMenuModel::modelNeedsUpdate, this, &ActiveWindowControlWidget::updateMenu);
    connect(this->m_appMenuModel, &AppMenuModel::requestActivateIndex, this, [ this ](int index){
        if(this->menuBar->actions().size() -2 > index && index >= 0){
            QThread::msleep(150);
            this->menuBar->setActiveAction(this->menuBar->actions()[index + 2]);
        }
    });

    m_layout->addStretch();

    this->m_buttonShowAnimation = new QPropertyAnimation(this->m_buttonWidget, "maximumWidth");
    this->m_buttonShowAnimation->setStartValue(0);
    this->m_buttonShowAnimation->setEndValue(this->m_buttonWidget->width());
    this->m_buttonShowAnimation->setDuration(150);
    this->m_buttonShowAnimation->setEasingCurve(QEasingCurve::Linear);

    this->m_buttonHideAnimation = new QPropertyAnimation(this->m_buttonWidget, "maximumWidth");
    this->m_buttonHideAnimation->setStartValue(this->m_buttonWidget->width());
    this->m_buttonHideAnimation->setEndValue(0);
    this->m_buttonHideAnimation->setDuration(150);
    this->m_buttonHideAnimation->setEasingCurve(QEasingCurve::Linear);
    connect(this->m_buttonHideAnimation, &QPropertyAnimation::finished, this->m_buttonWidget, &QWidget::hide);

    // this->setMouseTracking(true);

    connect(this->maxButton, &QToolButton::clicked, this, &ActiveWindowControlWidget::maximizeWindow);
    connect(this->minButton, &QToolButton::clicked, this, [ = ]{
        this->m_appInter->MinimizeWindow(this->currActiveWinId);
    });
    connect(this->closeButton, &QToolButton::clicked, this, [ = ]{
        this->m_appInter->CloseWindow(this->currActiveWinId);
    });

    // detect whether active window maximized signal
    connect(KWindowSystem::self(), qOverload<WId, NET::Properties, NET::Properties2>(&KWindowSystem::windowChanged), this, &ActiveWindowControlWidget::windowChanged);

    this->m_fixTimer = new QTimer(this);
    this->m_fixTimer->setSingleShot(true);
    this->m_fixTimer->setInterval(100);
    connect(this->m_fixTimer, &QTimer::timeout, this, &ActiveWindowControlWidget::activeWindowInfoChanged);
    // some applications like Gtk based and electron based seems still holds the focus after clicking the close button for a little while
    // Thus, we need to check the active window when some windows are closed.
    // However, we will use the dock dbus signal instead of other X operations.
    connect(this->m_appInter, &DBusDock::EntryAdded, this, [ = ](const QDBusObjectPath &path, const int index){
        AppItem *item = new AppItem(path);
        connect(item, &AppItem::windowInfoChanged, this, &ActiveWindowControlWidget::activeWindowInfoChanged);
        m_appItemMap.insert(item->appId(), item);
        this->m_fixTimer->start();
    });
    connect(this->m_appInter, &DBusDock::EntryRemoved, this, [ = ](const QString &key){
        if(AppItem *appItem = m_appItemMap.take(key)){
            disconnect(appItem, &AppItem::windowInfoChanged, this, &ActiveWindowControlWidget::activeWindowInfoChanged);
            appItem->deleteLater();
            this->m_fixTimer->start();
        }
    });
    connect(this->m_appInter, &DBusDock::ServiceRestarted, this, &ActiveWindowControlWidget::reloadAppItems);

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &ActiveWindowControlWidget::themeTypeChanged);
    themeTypeChanged(DGuiApplicationHelper::instance()->themeType());

    reloadAppItems();
}

void ActiveWindowControlWidget::initMenuBar(QLayout *layout)
{
    QMenu *menu = new QMenu;
        menu->addAction("关于", [ = ](){
            // QProcess::startDetached("dde-file-manager -p computer:///");
            // QProcess::startDetached("/usr/bin/dbus-send --session --print-reply --dest=com.deepin.SessionManager /com/deepin/StartManager com.deepin.StartManager.RunCommand string:\"/usr/bin/dde-file-manager\" array:string:\"-p\",\"computer:///\"");
            QProcess::startDetached("/usr/bin/qdbus", {"com.deepin.SessionManager", "/com/deepin/StartManager", "com.deepin.StartManager.RunCommand", "dde-file-manager", "(", "-p", "computer:///", ")"});
        });
        menu->addAction("启动器", this, [ = ](){
            QProcess::startDetached("/usr/bin/qdbus", {"com.deepin.dde.Launcher", "/com/deepin/dde/Launcher", "com.deepin.dde.Launcher.Toggle"});
        });

        menu->addAction("应用商店", [ = ](){
            QProcess::startDetached("/usr/bin/qdbus", {"com.deepin.SessionManager", "/com/deepin/StartManager", "com.deepin.StartManager.LaunchApp", "/usr/share/applications/deepin-app-store.desktop", "0", "[]"});
        });

        menu->addAction("设  置", [ = ](){
            QProcess::startDetached("/usr/bin/qdbus", {"com.deepin.SessionManager", "/com/deepin/StartManager", "com.deepin.StartManager.LaunchApp", "/usr/share/applications/dde-control-center.desktop", "0", "[]"});
        });

        menu->addAction("资源管理器", [ = ](){
            QProcess::startDetached("/usr/bin/qdbus", {"com.deepin.SessionManager", "/com/deepin/StartManager", "com.deepin.StartManager.LaunchApp", "/usr/share/applications/deepin-system-monitor.desktop", "0", "[]"});
        });

        menu->addAction("强制关闭窗口", [ = ](){
            QProcess::startDetached("/usr/bin/dbus-send", {"--dest=com.deepin.SessionManager", "/com/deepin/StartManager", "com.deepin.StartManager.RunCommand", "string:\"/usr/bin/xkill\"", "array:string:\"\""});
        });

        QMenu *shutdownMenu = new QMenu;
        shutdownMenu->addAction("注销", [](){
            DDBusSender().service("com.deepin.dde.shutdownFront").path("/com/deepin/dde/shutdownFront").interface("com.deepin.dde.shutdownFront").method("Logout").call();
        });
        shutdownMenu->addAction("锁定", [](){
            DDBusSender().service("com.deepin.dde.lockFront").path("/com/deepin/dde/lockFront").interface("com.deepin.dde.lockFront").method("Show").call();
        });
        shutdownMenu->addAction("切换用户", [](){
            DDBusSender().service("com.deepin.dde.shutdownFront").path("/com/deepin/dde/shutdownFront").interface("com.deepin.dde.shutdownFront").method("SwitchUser").call();
        });
        shutdownMenu->addAction("待机", [](){
            DDBusSender().service("com.deepin.dde.shutdownFront").path("/com/deepin/dde/shutdownFront").interface("com.deepin.dde.shutdownFront").method("Suspend").call();
        });
        shutdownMenu->addAction("重启", [](){
            DDBusSender().service("com.deepin.dde.shutdownFront").path("/com/deepin/dde/shutdownFront").interface("com.deepin.dde.shutdownFront").method("Restart").call();
        });
        shutdownMenu->addAction("关机", [](){
            DDBusSender().service("com.deepin.dde.shutdownFront").path("/com/deepin/dde/shutdownFront").interface("com.deepin.dde.shutdownFront").method("Shutdown").call();
        });

        QAction *shutdownAction = new QAction("关机");
        shutdownAction->setMenu(shutdownMenu);
        menu->addAction(shutdownAction);

    startAction = new QAction(this);
    startAction->setIcon(QIcon(":/icons/launcher.svg"));
    startAction->setMenu(menu);

    this->appTitleAction = new QAction(this);
    this->appTitleAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_P));
    QMenu *titleMenu = new QMenu();

    titleMenu->addAction("关于(&A)", [ = ]{
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

    titleMenu->addAction("关闭(&C)", [ = ]{
        if(this->currActiveWinId > 0)
            this->m_appInter->CloseWindow(this->currActiveWinId);
    });
    titleMenu->addAction("退出(&Q)", [ = ]{
        if (this->currActiveWinId > 0)
        {
            KWindowInfo info(this->currActiveWinId, NET::WMPid);
            int pid = info.pid();
            if(pid > 0)
                QProcess::startDetached("/usr/bin/kill", {"-9", QString::number(pid)});
        }
    });
    this->appTitleAction->setMenu(titleMenu);

    this->menuBar = new QMenuBar(this);
    this->menuBar->addAction(startAction);
    this->menuBar->addAction(this->appTitleAction);

    layout->addWidget(this->menuBar);
}

// void ActiveWindowControlWidget::clearAboutWindow()
// {
//     QObject *about = qobject_cast<QObject *>(engine->rootObjects().at(0));
//     about->deleteLater();
//     engine->deleteLater();
//     engine = nullptr;
//     qInfo()<<"Clear about complete!!!!!!!";
// }

void ActiveWindowControlWidget::reloadAppItems()
{
    QMap<QString, AppItem *>::iterator iterator = m_appItemMap.begin();
    while (iterator != m_appItemMap.end())
    {
        QString key = iterator.key();
        iterator++;
        AppItem *appItem = m_appItemMap.take(key);

        disconnect(appItem, &AppItem::windowInfoChanged, this, &ActiveWindowControlWidget::activeWindowInfoChanged);
        appItem->deleteLater();
    }

    for (auto path : m_appInter->entries())
    {
        AppItem *item = new AppItem(path);
        connect(item, &AppItem::windowInfoChanged, this, &ActiveWindowControlWidget::activeWindowInfoChanged);
        m_appItemMap.insert(item->appId(), item);
    }
    this->m_fixTimer->start();
}

void ActiveWindowControlWidget::activeWindowInfoChanged() {
    int activeWinId = XUtils::getFocusWindowId();
    // fix strange focus losing when pressing alt in some applications like chrome
    if (activeWinId == 0 || activeWinId == this->winId() || activeWinId == this->currActiveWinId) {
        return;
    }
    QString activeWinTitle = "未知";
    KWindowInfo kwin(activeWinId, NET::WMName|NET::WMPid, NET::WM2WindowClass);
    if (kwin.valid())
    {
        if(kwin.pid() == QCoreApplication::applicationPid())
            return;

        if (kwin.windowClassName() == "dde-desktop")
        {
            activeWinTitle = "桌面";
        }
        else
        {
            DesktopEntry entry = DesktopEntryStat::instance()->getDesktopEntryByName(kwin.windowClassName());
            if (!entry)
            {
                entry = DesktopEntryStat::instance()->getDesktopEntryByPid(kwin.pid());
            }
            if (entry)
            {
                activeWinTitle = entry->displayName;
            }
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
        bool ifFoundPrevActiveWinId = false;
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
                this->currActiveWinId = 0;
                this->appTitleAction->setText("桌面");
            } else {
                activeWinId = newCurActiveWinId;
                ifFoundPrevActiveWinId = true;
            }
        }
        if (!ifFoundPrevActiveWinId) {
            return;
        }
    }


    if (!activeWinTitle.isEmpty()) {
        this->appTitleAction->setText(activeWinTitle.append("(&P)"));
    }

    if ((kwin.valid() && QString(kwin.windowClassName()) == "dde-desktop") || activeWinTitle == "桌面")
    {
        this->appTitleAction->menu()->actions().at(1)->setDisabled(true);
        this->appTitleAction->menu()->actions().at(2)->setDisabled(true);
    } else {
        this->activeIdStack.push(activeWinId);
        this->appTitleAction->menu()->actions().at(1)->setEnabled(true);
        this->appTitleAction->menu()->actions().at(2)->setEnabled(true);
    }

    this->currActiveWinId = activeWinId;
    this->setButtonsVisible(activeWinTitle != "桌面" && XUtils::checkIfWinMaximum(this->currActiveWinId));

    this->m_appMenuModel->setWinId(this->currActiveWinId);
}

void ActiveWindowControlWidget::setButtonsVisible(bool visible) {
    if (visible != this->m_buttonWidget->isVisible()) {
        if (visible) {
            this->m_buttonHideAnimation->stop();
            this->m_buttonWidget->show();
            this->m_buttonShowAnimation->start();
        } else {
            this->m_buttonShowAnimation->stop();
            this->m_buttonHideAnimation->start();
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
        this->m_appInter->MaximizeWindow(this->currActiveWinId);
    }
}

void ActiveWindowControlWidget::mouseDoubleClickEvent(QMouseEvent *event) {
    if (this->childAt(event->pos()) != this->menuBar) {
        this->maximizeWindow();
    }
    QWidget::mouseDoubleClickEvent(event);
}

void ActiveWindowControlWidget::updateMenu() {
    while (this->menuBar->actions().size() > 2)
    {
        this->menuBar->removeAction(this->menuBar->actions()[2]);
    }

    if (m_appMenuModel->menuAvailable())
    {
        QMenu *menu = m_appMenuModel->menu();
        this->menuBar->addActions(menu->actions());
        this->menuBar->adjustSize();
    }
}

void ActiveWindowControlWidget::windowChanged(WId id, NET::Properties properties, NET::Properties2 properties2) {
    if (properties.testFlag(NET::WMGeometry)) {
        this->m_fixTimer->start();
    }

    // we still don't know why active window is 0 when pressing alt in some applications like chrome.
    if (KWindowSystem::activeWindow() != this->currActiveWinId && KWindowSystem::activeWindow() != 0) {
        return;
    }

    this->setButtonsVisible(XUtils::checkIfWinMaximum(this->currActiveWinId));
}

void ActiveWindowControlWidget::themeTypeChanged(DGuiApplicationHelper::ColorType themeType){
    switch (themeType)
    {
        case DGuiApplicationHelper::LightType:
            this->closeButton->setIcon(QIcon(":/icons/close.svg"));
            this->maxButton->setIcon(QIcon(":/icons/maximum.svg"));
            this->minButton->setIcon(QIcon(":/icons/minimum.svg"));
            this->menuBar->setStyleSheet("QMenuBar {font-size: 13px; color: black; background-color: rgba(0,0,0,0); margin: 0 0 0 0;} ");
            break;
        case DGuiApplicationHelper::DarkType:
            this->closeButton->setIcon(QIcon(":/icons/close-white.svg"));
            this->maxButton->setIcon(QIcon(":/icons/maximum-white.svg"));
            this->minButton->setIcon(QIcon(":/icons/minimum-white.svg"));
            this->menuBar->setStyleSheet("QMenuBar {font-size: 13px; color: white; background-color: rgba(0,0,0,0); margin: 0 0 0 0;} ");
        default:
            break;
    }
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
    if(id < 0 || id >= this->menuBar->actions().count()){
        id = 0;
    }
    QThread::msleep(150);
    this->menuBar->setActiveAction(this->menuBar->actions()[id]);
}
