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

#include "../frame/item/components/hoverhighlighteffect.h"
#include "QClickableLabel.h"

ActiveWindowControlWidget::ActiveWindowControlWidget(QWidget *parent)
    : QWidget(parent)
    , m_appInter(new DBusDock("com.deepin.dde.daemon.Dock", "/com/deepin/dde/daemon/Dock", QDBusConnection::sessionBus(), this))
    , m_wmInter(new DBusWM("com.deepin.wm", "/com/deepin/wm", QDBusConnection::sessionBus(), this))
    , mouseClicked(false)
{
    QPalette palette1 = this->palette();
    palette1.setColor(QPalette::Background, Qt::transparent);
    this->setPalette(palette1);

    this->m_layout = new QHBoxLayout(this);
    this->m_layout->setSpacing(12);
    this->m_layout->setContentsMargins(10, 0, 0, 0);
    this->setLayout(this->m_layout);

    QClickableLabel *launchLabel = new QClickableLabel(this);
    launchLabel->setToolTip("启动器");
    launchLabel->setToolTipDuration(5000);
    launchLabel->setStyleSheet("QLabel{background-color: rgba(0,0,0,0);}");
    launchLabel->setPixmap(QPixmap(":/icons/launcher.svg"));
    launchLabel->setFixedSize(22, 22);
    launchLabel->setScaledContents(true);
    launchLabel->setGraphicsEffect(new HoverHighlightEffect(this));
    connect(launchLabel, &QClickableLabel::clicked, this, [=](){
        QProcess::startDetached("dde-launcher -p");
    });
    this->m_layout->addWidget(launchLabel);

    this->m_iconLabel = new QLabel(this);
    this->m_iconLabel->setFixedSize(22, 22);
    this->m_iconLabel->setScaledContents(true);
    this->m_iconLabel->setGraphicsEffect(new HoverHighlightEffect(this));
    this->m_layout->addWidget(this->m_iconLabel);

    int buttonSize = 22;
    this->m_buttonWidget = new QWidget(this);
    this->m_buttonLayout = new QHBoxLayout(this->m_buttonWidget);
    this->m_buttonLayout->setContentsMargins(0, 0, 0, 0);
    this->m_buttonLayout->setSpacing(12);
    this->m_buttonLayout->setMargin(0);

    this->closeButton = new QToolButton(this->m_buttonWidget);
    this->closeButton->setFixedSize(buttonSize, buttonSize);
    this->closeButton->setIcon(QIcon(":/icons/close.svg"));
    this->closeButton->setIconSize(QSize(buttonSize - 8, buttonSize - 8));
    this->m_buttonLayout->addWidget(this->closeButton);

    this->maxButton = new QToolButton(this->m_buttonWidget);
    this->maxButton->setFixedSize(buttonSize, buttonSize);
    this->maxButton->setIcon(QIcon(":/icons/maximum.svg"));
    this->maxButton->setIconSize(QSize(buttonSize - 8, buttonSize - 8));
    this->m_buttonLayout->addWidget(this->maxButton);

    this->minButton = new QToolButton(this->m_buttonWidget);
    this->minButton->setFixedSize(buttonSize, buttonSize);
    this->minButton->setIcon(QIcon(":/icons/minimum.svg"));
    this->minButton->setIconSize(QSize(buttonSize - 8, buttonSize - 8));
    this->m_buttonLayout->addWidget(this->minButton);
    this->m_layout->addWidget(this->m_buttonWidget);

    this->menuBar = new QMenuBar(this);
    this->menuBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    // this->menuBar->setFixedHeight(28);
    // this->menuBar->setStyleSheet("font-size: 14px; line-height: 100%; background-color: rgba(0,0,0,0)");    
    this->m_layout->addWidget(this->menuBar);

    this->m_appMenuModel = new AppMenuModel(this);
    connect(this->m_appMenuModel, &AppMenuModel::modelNeedsUpdate, this, &ActiveWindowControlWidget::updateMenu);

    this->m_winTitleLabel = new QLabel(this);
    this->m_winTitleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    this->m_layout->addWidget(this->m_winTitleLabel);

    this->m_layout->addStretch();

    this->m_buttonShowAnimation = new QPropertyAnimation(this->m_buttonWidget, "maximumWidth");
    this->m_buttonShowAnimation->setEndValue(this->m_buttonWidget->width());
    this->m_buttonShowAnimation->setDuration(150);

    this->m_buttonHideAnimation = new QPropertyAnimation(this->m_buttonWidget, "maximumWidth");
    this->m_buttonHideAnimation->setEndValue(0);
    this->m_buttonHideAnimation->setDuration(150);

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
        qDebug() << "Failed to get active window id !";
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

    if (activeWinId != this->currActiveWinId) {
        this->currActiveWinId = activeWinId;
        this->m_iconLabel->setPixmap(XUtils::getWindowIconName(this->currActiveWinId));
        // todo
    }

    this->setButtonsVisible(XUtils::checkIfWinMaximum(this->currActiveWinId));

    QString activeWinTitle = XUtils::getWindowName(activeWinId);
    if (activeWinTitle != this->currActiveWinTitle) {
        this->currActiveWinTitle = activeWinTitle;
        this->m_winTitleLabel->setText(this->currActiveWinTitle);
        this->m_iconLabel->setToolTip(activeWinTitle);
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
    if (visible) {
        this->m_buttonShowAnimation->setStartValue(this->m_buttonWidget->width());
        this->m_buttonShowAnimation->start();
    } else {
        this->m_buttonHideAnimation->setStartValue(this->m_buttonWidget->width());
        this->m_buttonHideAnimation->start();
    }
}

void ActiveWindowControlWidget::enterEvent(QEvent *event) {
    QWidget::enterEvent(event);
}

void ActiveWindowControlWidget::leaveEvent(QEvent *event) {
    QWidget::leaveEvent(event);
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
    this->maximizeWindow();
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
            this->menuBar->setStyleSheet("QMenuBar {color: black; background-color: rgba(0,0,0,0); margin: 0 5 0 5;} ");
            this->m_winTitleLabel->setStyleSheet("QLabel { color: black; }");
            break;
        case DGuiApplicationHelper::DarkType:
            this->closeButton->setIcon(QIcon(":/icons/close-white.svg"));
            this->maxButton->setIcon(QIcon(":/icons/maximum-white.svg"));
            this->minButton->setIcon(QIcon(":/icons/minimum-white.svg"));
            this->menuBar->setStyleSheet("QMenuBar {color: white; background-color: rgba(0,0,0,0); margin: 0 5 0 5;} ");
            this->m_winTitleLabel->setStyleSheet("QLabel { color: white; }");
        default:
            break;
    }
}

void ActiveWindowControlWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        QWidget *pressedWidget = childAt(event->pos());
        if (pressedWidget == nullptr || pressedWidget == m_winTitleLabel) {
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

void ActiveWindowControlWidget::applyCustomSettings(const CustomSettings& settings) {
    // buttons
    this->closeButton->setIcon(QIcon(settings.getActiveCloseIconPath()));
    this->maxButton->setIcon(QIcon(settings.getActiveUnmaximizedIconPath()));
    this->minButton->setIcon(QIcon(settings.getActiveMinimizedIconPath()));
    // todo: default app icon
}
