//
// Created by septemberhx on 2020/5/26.
//

#include <QDebug>
#include <QWindow>
#include "ActiveWindowControlWidget.h"
#include "util/XUtils.h"

ActiveWindowControlWidget::ActiveWindowControlWidget(QWidget *parent)
    : QWidget(parent)
    , m_appInter(new DBusDock("com.deepin.dde.daemon.Dock", "/com/deepin/dde/daemon/Dock", QDBusConnection::sessionBus(), this))
{
    this->m_layout = new QHBoxLayout(this);
    this->m_layout->setSpacing(12);
    this->m_layout->setContentsMargins(10, 0, 0, 0);
    this->setLayout(this->m_layout);

    this->m_iconLabel = new QLabel(this);
    this->m_iconLabel->setFixedSize(22, 22);
    this->m_iconLabel->setScaledContents(true);
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

    this->minButton = new QToolButton(this->m_buttonWidget);
    this->minButton->setFixedSize(buttonSize, buttonSize);
    this->minButton->setIcon(QIcon(":/icons/minimum.svg"));
    this->minButton->setIconSize(QSize(buttonSize - 8, buttonSize - 8));
    this->m_buttonLayout->addWidget(this->minButton);

    this->maxButton = new QToolButton(this->m_buttonWidget);
    this->maxButton->setFixedSize(buttonSize, buttonSize);
    this->maxButton->setIcon(QIcon(":/icons/maximum.svg"));
    this->maxButton->setIconSize(QSize(buttonSize - 8, buttonSize - 8));
    this->m_buttonLayout->addWidget(this->maxButton);
    this->m_layout->addWidget(this->m_buttonWidget);

    QLabel *l1 = new QLabel("[", this);
    this->m_layout->addWidget(l1);

    this->menuBar = new QMenuBar(this);
    this->menuBar->setContentsMargins(0, 0, 0, 0);
    this->menuBar->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    this->menuBar->setFixedHeight(28);
    // this->menuBar->adjustSize();
    // this->menuBar->addMenu("Test")->addAction("Hello");

    this->menuBar->setStyleSheet("font-size: 12px; line-height: 100%; color: black; background-color: rgba(0,0,0,0)");
    
    this->m_layout->addWidget(this->menuBar);

    QLabel *l2 = new QLabel("]", this);
    this->m_layout->addWidget(l2);


    this->m_appMenuModel = new AppMenuModel(this);
    connect(this->m_appMenuModel, &AppMenuModel::modelNeedsUpdate, this, &ActiveWindowControlWidget::updateMenu);

    this->m_winTitleLabel = new QLabel(this);
    this->m_winTitleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    QPalette sample_palette;
    sample_palette.setColor(QPalette::WindowText, Qt::black);
    this->m_winTitleLabel->setPalette(sample_palette);

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
    connect(KWindowSystem::self(), qOverload<WId>(&KWindowSystem::windowChanged), this, &ActiveWindowControlWidget::windowChanged);
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
        // todo
    }

    this->setButtonsVisible(XUtils::checkIfWinMaximum(this->currActiveWinId));

    QString activeWinTitle = XUtils::getWindowName(activeWinId);
    if (activeWinTitle != this->currActiveWinTitle) {
        this->currActiveWinTitle = activeWinTitle;
        this->m_winTitleLabel->setText(this->currActiveWinTitle);
    }

    if (!activeWinTitle.isEmpty()) {
        this->m_iconLabel->setPixmap(XUtils::getWindowIconName(this->currActiveWinId));
    }
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
        qDebug()<<"menuBar is hidden: " << this->menuBar->isHidden() <<", action count: "<<menu->actions().count();
        // qDebug()<<"action count: "<<menu->actions().count();
    }
}

void ActiveWindowControlWidget::windowChanged() {
    // we still don't know why active window is 0 when pressing alt in some applications like chrome.
    if (KWindowSystem::activeWindow() != this->currActiveWinId && KWindowSystem::activeWindow() != 0) {
        return;
    }

    this->setButtonsVisible(XUtils::checkIfWinMaximum(this->currActiveWinId));
}
