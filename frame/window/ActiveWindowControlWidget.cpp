//
// Created by septemberhx on 2020/5/26.
//

#include <QDebug>
#include <QWindow>
#include "ActiveWindowControlWidget.h"
#include "util/XUtils.h"
#include <QMouseEvent>
#include <NETWM>
#include <QtX11Extras/QX11Info>
#include <QProcess>
#include "QClickableLabel.h"

#include "../item/components/hoverhighlighteffect.h"
#include <QApplication>
#include <QScreen>
#include <QEvent>
#include <QDesktopWidget>
#include <DDBusSender>

ActiveWindowControlWidget::ActiveWindowControlWidget(QWidget *parent)
    : QWidget(parent)
    , m_appInter(new DBusDock("com.deepin.dde.daemon.Dock", "/com/deepin/dde/daemon/Dock", QDBusConnection::sessionBus(), this))
    , mouseClicked(false)
{
    QPalette palette1 = this->palette();
    palette1.setColor(QPalette::Background, Qt::transparent);
    this->setPalette(palette1);

    this->m_layout = new QHBoxLayout(this);
    this->m_layout->setSpacing(10);
    this->m_layout->setContentsMargins(5, 0, 0, 0);
    this->setLayout(this->m_layout);
    
    QClickableLabel *launchLabel = new QClickableLabel(this);
    launchLabel->setToolTip("启动器");
    launchLabel->setToolTipDuration(5000);
    launchLabel->setStyleSheet("QLabel{background-color: rgba(0,0,0,0);}");
    launchLabel->setPixmap(QPixmap(":/icons/launcher.svg"));
    launchLabel->setFixedSize(18, 18);
    launchLabel->setScaledContents(true);
 
    this->menu = new QMenu;
        this->menu->addAction("关于", [ = ](){
            // QProcess::startDetached("dde-file-manager -p computer:///");
            QProcess::startDetached("/usr/bin/dbus-send --session --print-reply --dest=com.deepin.SessionManager /com/deepin/StartManager com.deepin.StartManager.RunCommand string:\"/usr/bin/dde-file-manager\" array:string:\"-p\",\"computer:///\"");
            // QProcess::startDetached("/usr/bin/qdbus --literal com.deepin.SessionManager /com/deepin/StartManager com.deepin.StartManager.RunCommand 'dde-file-manager' '(' '-p' 'computer:///' ')'");
        });
        this->menu->addAction("启动器", this, [ = ](){
            QProcess::startDetached("/usr/bin/qdbus", {"com.deepin.dde.Launcher", "/com/deepin/dde/Launcher", "com.deepin.dde.Launcher.Toggle"});
        });

        this->menu->addAction("应用商店", [ = ](){
            // QProcess::startDetached("deepin-app-store");
            QProcess::startDetached("/usr/bin/qdbus --literal com.deepin.SessionManager /com/deepin/StartManager com.deepin.StartManager.LaunchApp /usr/share/applications/deepin-app-store.desktop 0 []");
        });

        this->menu->addAction("设  置", [ = ](){
            QProcess::startDetached("/usr/bin/qdbus --literal com.deepin.SessionManager /com/deepin/StartManager com.deepin.StartManager.LaunchApp /usr/share/applications/dde-control-center.desktop 0 []");
        });

        this->menu->addAction("资源管理器", [ = ](){
            QProcess::startDetached("/usr/bin/qdbus com.deepin.SessionManager /com/deepin/StartManager com.deepin.StartManager.LaunchApp /usr/share/applications/deepin-system-monitor.desktop 0 []");
        });

        this->menu->addAction("强制关闭窗口", [ = ](){
            // QProcess::startDetached("xkill");
            QProcess::startDetached("/usr/bin/dbus-send --session --print-reply --dest=com.deepin.SessionManager /com/deepin/StartManager com.deepin.StartManager.RunCommand string:\"/usr/bin/xkill\" array:string:\"\"");
        });

        QMenu *shutdownMenu = new QMenu;
        shutdownMenu->addAction("注销", [](){
            QProcess::startDetached("qdbus com.deepin.dde.shutdownFront /com/deepin/dde/shutdownFront com.deepin.dde.shutdownFront.Logout");
        });
        shutdownMenu->addAction("锁定", [](){
            // QProcess::startDetached("qdbus com.deepin.dde.lockFront /com/deepin/dde/lockFront com.deepin.dde.lockFront.Show");
            DDBusSender().service("com.deepin.dde.lockFront").path("/com/deepin/dde/lockFront").interface("com.deepin.dde.lockFront").method("Show").call();
        });
        shutdownMenu->addAction("切换用户", [](){
            QProcess::startDetached("qdbus com.deepin.dde.shutdownFront /com/deepin/dde/shutdownFront com.deepin.dde.shutdownFront.SwitchUser");
        });        
        shutdownMenu->addAction("待机", [](){
            QProcess::startDetached("qdbus com.deepin.dde.shutdownFront /com/deepin/dde/shutdownFront com.deepin.dde.shutdownFront.Suspend");
        });
        shutdownMenu->addAction("重启", [](){
            QProcess::startDetached("qdbus com.deepin.dde.shutdownFront /com/deepin/dde/shutdownFront com.deepin.dde.shutdownFront.Restart");
        });
        shutdownMenu->addAction("关机", [](){
            QProcess::startDetached("qdbus com.deepin.dde.shutdownFront /com/deepin/dde/shutdownFront com.deepin.dde.shutdownFront.Shutdown");
        });

        QAction *shutdownAction = new QAction("关机");
        shutdownAction->setMenu(shutdownMenu);
        this->menu->addAction(shutdownAction);   // launchLabel->setGraphicsEffect(new HoverHighlightEffect(this));
    
    connect(launchLabel, &QClickableLabel::clicked, this, &ActiveWindowControlWidget::toggleStartMenu);
    this->m_layout->addWidget(launchLabel);

    this->m_iconLabel = new QLabel(this);
    this->m_iconLabel->setFixedSize(18, 18);
    this->m_iconLabel->setScaledContents(true);
    this->m_iconLabel->setGraphicsEffect(new HoverHighlightEffect(this));
    
    this->m_layout->addWidget(this->m_iconLabel);

    int buttonSize = 18;
    this->m_buttonWidget = new QWidget(this);
    this->m_buttonLayout = new QHBoxLayout(this->m_buttonWidget);
    this->m_buttonLayout->setContentsMargins(0, 0, 0, 0);
    this->m_buttonLayout->setSpacing(12);
    this->m_buttonLayout->setMargin(0);

    this->closeButton = new QToolButton(this->m_buttonWidget);
    this->closeButton->setToolTip("关闭");
    this->closeButton->setFixedSize(buttonSize, buttonSize);
    this->closeButton->setIcon(QIcon(":/icons/close.svg"));
    this->closeButton->setIconSize(QSize(buttonSize - 8, buttonSize - 8));
    this->m_buttonLayout->addWidget(this->closeButton);

    this->maxButton = new QToolButton(this->m_buttonWidget);
    this->maxButton->setToolTip("还原");
    this->maxButton->setFixedSize(buttonSize, buttonSize);
    this->maxButton->setIcon(QIcon(":/icons/maximum.svg"));
    this->maxButton->setIconSize(QSize(buttonSize - 8, buttonSize - 8));
    this->m_buttonLayout->addWidget(this->maxButton);

    this->minButton = new QToolButton(this->m_buttonWidget);
    this->minButton->setToolTip("最小化");
    this->minButton->setFixedSize(buttonSize, buttonSize);
    this->minButton->setIcon(QIcon(":/icons/minimum.svg"));
    this->minButton->setIconSize(QSize(buttonSize - 8, buttonSize - 8));
    this->m_buttonLayout->addWidget(this->minButton);
    this->m_layout->addWidget(this->m_buttonWidget);

    this->menuBar = new QMenuBar(this);
    // this->menuBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    // this->menuBar->setFixedHeight(20);
    // QFont font = this->menuBar->font();font.setPointSize(10);this->menuBar->setFont(font);
    // this->menuBar->setMaximumHeight(20);
    // this->menuBar->setStyleSheet("QMenuBar {font-size: 12px;}");
    // this->menuBar->setStyleSheet("QMenuBar::item { height: 18px; font-size: 12px; margin: 0px; padding: 0px}");    
    this->m_layout->addWidget(this->menuBar);

    this->m_appMenuModel = new AppMenuModel(this);
    connect(this->m_appMenuModel, &AppMenuModel::modelNeedsUpdate, this, &ActiveWindowControlWidget::updateMenu);
    connect(this->m_appMenuModel, &AppMenuModel::requestActivateIndex, this, [ this ](int index){
        if(this->menuBar->actions().size() > index && index >= 0){
            QThread::msleep(150);
            this->menuBar->setActiveAction(this->menuBar->actions()[index]);
        }
    });

    this->m_winTitleLabel = new QLabel(this);
    this->m_winTitleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    this->m_layout->addWidget(this->m_winTitleLabel);

    this->m_layout->addStretch();

    this->m_buttonShowAnimation = new QPropertyAnimation(this->m_buttonWidget, "maximumWidth");
    this->m_buttonShowAnimation->setEndValue(this->m_buttonWidget->width());
    this->m_buttonShowAnimation->setDuration(150);
    this->m_buttonShowAnimation->setEasingCurve(QEasingCurve::Linear);

    this->m_buttonHideAnimation = new QPropertyAnimation(this->m_buttonWidget, "maximumWidth");
    this->m_buttonHideAnimation->setEndValue(0);
    this->m_buttonHideAnimation->setDuration(150);
    this->m_buttonHideAnimation->setEasingCurve(QEasingCurve::Linear);
    connect(this->m_buttonHideAnimation, &QPropertyAnimation::finished, this, [this](){
        this->m_buttonWidget->hide();
    });

    this->setButtonsVisible(false);
    this->setMouseTracking(true);

    connect(this->maxButton, &QToolButton::clicked, this, &ActiveWindowControlWidget::maxButtonClicked);
    connect(this->minButton, &QToolButton::clicked, this, &ActiveWindowControlWidget::minButtonClicked);
    connect(this->closeButton, &QToolButton::clicked, this, &ActiveWindowControlWidget::closeButtonClicked);

    // detect whether active window maximized signal
    connect(KWindowSystem::self(), qOverload<WId, NET::Properties, NET::Properties2>(&KWindowSystem::windowChanged), this, &ActiveWindowControlWidget::windowChanged);

    this->m_fixTimer = new QTimer(this);
    this->m_fixTimer->setSingleShot(true);
    this->m_fixTimer->setInterval(100);
    connect(this->m_fixTimer, &QTimer::timeout, this, &ActiveWindowControlWidget::activeWindowInfoChanged);
    // some applications like Gtk based and electron based seems still holds the focus after clicking the close button for a little while
    // Thus, we need to check the active window when some windows are closed.
    // However, we will use the dock dbus signal instead of other X operations.
    connect(this->m_appInter, &DBusDock::EntryRemoved, this->m_fixTimer, qOverload<>(&QTimer::start));
    connect(this->m_appInter, &DBusDock::EntryAdded, this->m_fixTimer, qOverload<>(&QTimer::start));

    applyCustomSettings(*CustomSettings::instance());

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &ActiveWindowControlWidget::themeTypeChanged);
    themeTypeChanged(DGuiApplicationHelper::instance()->themeType());
}

void ActiveWindowControlWidget::activeWindowInfoChanged() {
    int activeWinId = XUtils::getFocusWindowId();
    if (activeWinId < 0) {
        this->currActiveWinId = -1;
        this->m_winTitleLabel->setText(tr("桌面"));
        this->m_iconLabel->setPixmap(QPixmap(CustomSettings::instance()->getActiveDefaultAppIconPath()));
        return;
    }

    // fix strange focus losing when pressing alt in some applications like chrome
    if (activeWinId == 0) {
        return;
    }

    // fix focus moving to top panel bug with aurorae
    if (activeWinId == this->winId()) {
        return;
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
                this->currActiveWinId = -1;
                this->m_winTitleLabel->setText(tr("桌面"));
                this->m_iconLabel->setPixmap(QPixmap(CustomSettings::instance()->getActiveDefaultAppIconPath()));
            } else {
                activeWinId = newCurActiveWinId;
                ifFoundPrevActiveWinId = true;
            }
        }
        if (!ifFoundPrevActiveWinId) {
            return;
        }
    }

    if (activeWinId != this->currActiveWinId) {
        this->currActiveWinId = activeWinId;
        this->activeIdStack.push(this->currActiveWinId);
    }

    this->setButtonsVisible(XUtils::checkIfWinMaximum(this->currActiveWinId));

    QString activeWinTitle = XUtils::getWindowName(this->currActiveWinId);
    this->currActiveWinTitle = activeWinTitle;
    this->m_winTitleLabel->setText(this->currActiveWinTitle);

    if (!activeWinTitle.isEmpty()) {
        this->m_iconLabel->setPixmap(XUtils::getWindowIconNameX11(this->currActiveWinId));
        this->m_iconLabel->setToolTip(XUtils::getWindowAppName(this->currActiveWinId));
    }

    // KWindowSystem will not update menu for desktop when focusing on the desktop
    // It is not a good idea to do the filter here instead of the AppmenuModel.
    // However, it works, and works pretty well.
    if (activeWinTitle == tr("桌面")) {
        // hide buttons
        this->setButtonsVisible(false);
        this->m_appMenuModel->setMenuAvailable(false);
        this->updateMenu();
    }

    // some applications like KWrite will expose its global menu with an invalid dbus path
    //   thus we need to recheck it again :(
    this->m_appMenuModel->setWinId(this->currActiveWinId);
}

void ActiveWindowControlWidget::setButtonsVisible(bool visible) {
    if (CustomSettings::instance()->isShowControlButtons()) {
        if (visible) {
            this->m_buttonWidget->show();
            this->m_buttonShowAnimation->setStartValue(this->m_buttonWidget->width());
            this->m_buttonShowAnimation->start();
        } else {
            this->m_buttonHideAnimation->setStartValue(this->m_buttonWidget->width());
            this->m_buttonHideAnimation->start();
        }
    }
}

void ActiveWindowControlWidget::maxButtonClicked() {
    if (XUtils::checkIfWinMaximum(this->currActiveWinId)) {
        XUtils::unmaximizeWindow(this->currActiveWinId);

        // checkIfWinMaximum is MUST needed here.
        //   I don't know whether XSendEvent is designed like this,
        //   my test shows unmaximizeWindow by XSendEvent will work when others try to fetch its properties.
        //   i.e., checkIfWinMaximum
        XUtils::checkIfWinMaximum(this->currActiveWinId);
    } else {
        // sadly the dbus maximizeWindow cannot unmaximize window :(
        this->m_appInter->MaximizeWindow(this->currActiveWinId);
    }

//    this->activeWindowInfoChanged();
}

void ActiveWindowControlWidget::minButtonClicked() {
    this->m_appInter->MinimizeWindow(this->currActiveWinId);
}

void ActiveWindowControlWidget::closeButtonClicked() {
    this->m_appInter->CloseWindow(this->currActiveWinId);
}

void ActiveWindowControlWidget::maximizeWindow() {
    this->maxButtonClicked();
}

void ActiveWindowControlWidget::mouseDoubleClickEvent(QMouseEvent *event) {
    if (this->childAt(event->pos()) != this->menuBar) {
        this->maximizeWindow();
    }
    QWidget::mouseDoubleClickEvent(event);
}

void ActiveWindowControlWidget::updateMenu() {
    foreach (QAction *action, this->menuBar->actions())
    {
        this->menuBar->removeAction(action);
    }
        
    if (m_appMenuModel->rowCount() == 0 || m_appMenuModel->visible() == false)
    {
        this->m_winTitleLabel->show();
        this->menuBar->hide();
    } 
    else 
    {
        QMenu *menu = m_appMenuModel->menu();
        this->m_winTitleLabel->hide();
        this->menuBar->addActions(menu->actions());
        this->menuBar->show();
        this->menuBar->adjustSize();
        // qDebug()<<"menuBar is hidden: " << this->menuBar->isHidden() <<", action count: "<<menu->actions().count();
    }
}

void ActiveWindowControlWidget::windowChanged(WId id, NET::Properties properties, NET::Properties2 properties2) {
    if (properties.testFlag(NET::WMGeometry)) {
        this->activeWindowInfoChanged();
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
            this->m_winTitleLabel->setStyleSheet("QLabel { color: black; }");
            break;
        case DGuiApplicationHelper::DarkType:
            this->closeButton->setIcon(QIcon(":/icons/close-white.svg"));
            this->maxButton->setIcon(QIcon(":/icons/maximum-white.svg"));
            this->minButton->setIcon(QIcon(":/icons/minimum-white.svg"));
            this->menuBar->setStyleSheet("QMenuBar {font-size: 13px; color: white; background-color: rgba(0,0,0,0); margin: 0 0 0 0;} ");
            this->m_winTitleLabel->setStyleSheet("QLabel { color: white; }");
        default:
            break;
    }
    qDebug()<<"QmenuBar size: "<<this->menuBar->size();
}

void ActiveWindowControlWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        QWidget *pressedWidget = childAt(event->pos());
        if (pressedWidget == nullptr || pressedWidget == m_winTitleLabel) {
            this->mouseClicked = !this->mouseClicked;
        } else if (qobject_cast<QLabel*>(pressedWidget) == this->m_iconLabel) {

        }
        // KWindowSystem::activateWindow(this->currActiveWinId);
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

void ActiveWindowControlWidget::applyCustomSettings(const CustomSettings& settings) {
    // buttons
    this->m_buttonWidget->setVisible(settings.isShowControlButtons());
    this->closeButton->setIcon(QIcon(settings.getActiveCloseIconPath()));
    this->maxButton->setIcon(QIcon(settings.getActiveUnmaximizedIconPath()));
    this->minButton->setIcon(QIcon(settings.getActiveMinimizedIconPath()));
    // todo: default app icon
}

int ActiveWindowControlWidget::currScreenNum() {
    return QApplication::desktop()->screenNumber(this);
}

void ActiveWindowControlWidget::toggleStartMenu() {
    QThread::msleep(150);
    if (this->menu->isHidden()) {
        this->menu->popup(this->pos() + QPoint(0, height()));
    } else {
        this->menu->hide();
    }
}

void ActiveWindowControlWidget::toggleMenu() {
    if (this->menuBar->actions().size() > 0)
    {
        QThread::msleep(150);
        this->menuBar->setActiveAction(this->menuBar->actions()[0]);
    }
}