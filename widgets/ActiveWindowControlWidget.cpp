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
    this->m_layout->setContentsMargins(10, 5, 10, 5);
    this->setLayout(this->m_layout);

    this->m_iconLabel = new QLabel(this);
    this->m_iconLabel->setFixedSize(22, 22);
    this->m_iconLabel->setScaledContents(true);
    this->m_layout->addWidget(this->m_iconLabel);

    int buttonSize = 22;
    this->closeButton = new QToolButton(this);
    this->closeButton->setFixedSize(buttonSize, buttonSize);
    this->closeButton->setIcon(QIcon(":/icons/close.svg"));
    this->closeButton->setIconSize(QSize(buttonSize - 8, buttonSize - 8));
    this->m_layout->addWidget(this->closeButton);

    this->minButton = new QToolButton(this);
    this->minButton->setFixedSize(buttonSize, buttonSize);
    this->minButton->setIcon(QIcon(":/icons/minimum.svg"));
    this->minButton->setIconSize(QSize(buttonSize - 8, buttonSize - 8));
    this->m_layout->addWidget(this->minButton);

    this->maxButton = new QToolButton(this);
    this->maxButton->setFixedSize(buttonSize, buttonSize);
    this->maxButton->setIcon(QIcon(":/icons/maximum.svg"));
    this->maxButton->setIconSize(QSize(buttonSize - 8, buttonSize - 8));
    this->m_layout->addWidget(this->maxButton);

    this->m_menuWidget = new QWidget(this);
    this->m_layout->addWidget(this->m_menuWidget);
    this->m_menuLayout = new QHBoxLayout(this->m_menuWidget);
    this->m_menuLayout->setContentsMargins(0, 0, 0, 0);
    this->m_menuLayout->setSpacing(2);

    this->m_appMenuModel = new AppMenuModel(this);
    connect(this->m_appMenuModel, &AppMenuModel::modelNeedsUpdate, this, &ActiveWindowControlWidget::updateMenu);

    this->m_winTitleLabel = new QLabel(this);
    this->m_winTitleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    QPalette sample_palette;
    sample_palette.setColor(QPalette::WindowText, Qt::black);
    this->m_winTitleLabel->setPalette(sample_palette);

    this->m_layout->addWidget(this->m_winTitleLabel);

    this->m_layout->addStretch();

    this->setButtonsVisible(false);
    this->setMouseTracking(true);

    connect(this->maxButton, &QToolButton::clicked, this, &ActiveWindowControlWidget::maxButtonClicked);
    connect(this->minButton, &QToolButton::clicked, this, &ActiveWindowControlWidget::minButtonClicked);
    connect(this->closeButton, &QToolButton::clicked, this, &ActiveWindowControlWidget::closeButtonClicked);
}

void ActiveWindowControlWidget::activeWindowInfoChanged() {
    int activeWinId = XUtils::getFocusWindowId();
    if (activeWinId < 0) {
        qDebug() << "Failed to get active window id !";
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
    this->closeButton->setVisible(visible);
    this->minButton->setVisible(visible);
    this->maxButton->setVisible(visible);
}

void ActiveWindowControlWidget::enterEvent(QEvent *event) {
    if (XUtils::checkIfWinMaximum(this->currActiveWinId)) {
        this->setButtonsVisible(true);
    }
    QWidget::enterEvent(event);
}

void ActiveWindowControlWidget::leaveEvent(QEvent *event) {
    if (!XUtils::checkIfWinMaximum(this->currActiveWinId)) {
        this->setButtonsVisible(false);
    }
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
        this->setButtonsVisible(false);
    } else {
        // sadly the dbus maximizeWindow cannot unmaximize window :(
        this->m_appInter->MaximizeWindow(this->currActiveWinId);
        this->setButtonsVisible(true);
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
    this->m_menuWidget->hide();
    for (auto m_label : this->buttonLabelList) {
        this->m_menuLayout->removeWidget(m_label);
        m_label->close();
        delete m_label;
    }
    this->buttonLabelList.clear();

    QList<QString> existedMenu;  // tricks for twice menu of libreoffice
    for (int r = 0; r < m_appMenuModel->rowCount(); ++r) {
        QString menuStr = m_appMenuModel->data(m_appMenuModel->index(r, 0), AppMenuModel::MenuRole).toString();

        if (existedMenu.contains(menuStr)) {
            int index = existedMenu.indexOf(menuStr);
            auto *m_label = this->buttonLabelList.at(index);
            m_label->hide();
        }

        existedMenu.append(menuStr);
        auto *m_label = new QClickableLabel(this->m_menuWidget);
        m_label->setText(QString("  %1  ").arg(menuStr.remove('&')));
        m_label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        connect(m_label, &QClickableLabel::clicked, this, &ActiveWindowControlWidget::menuLabelClicked);
        this->m_menuLayout->addWidget(m_label);
        this->buttonLabelList.append(m_label);

        if (menuStr.isEmpty()) {
            m_label->hide();
        }
    }
    this->m_menuWidget->show();

    if (this->buttonLabelList.isEmpty()) {
        this->m_winTitleLabel->show();
    } else {
        this->m_winTitleLabel->hide();
    }
}

void ActiveWindowControlWidget::menuLabelClicked() {
    QClickableLabel *label = dynamic_cast<QClickableLabel*>(sender());
    int index = this->buttonLabelList.indexOf(label);
    this->trigger(label, index);
}

QMenu *ActiveWindowControlWidget::createMenu(int idx) const {
    QMenu *menu = nullptr;
    QAction *action = nullptr;

    const QModelIndex index = m_appMenuModel->index(idx, 0);
    const QVariant data = m_appMenuModel->data(index, AppMenuModel::ActionRole);
    action = (QAction *)data.value<void *>();
    if (action) {
        menu = action->menu();
    }

    return menu;
}

void ActiveWindowControlWidget::trigger(QWidget *ctx, int idx) {
    QMenu *actionMenu = createMenu(idx);
    if (actionMenu) {

        //this is a workaround where Qt will fail to realise a mouse has been released
        // this happens if a window which does not accept focus spawns a new window that takes focus and X grab
        // whilst the mouse is depressed
        // https://bugreports.qt.io/browse/QTBUG-59044
        // this causes the next click to go missing

        //by releasing manually we avoid that situation
//        auto ungrabMouseHack = [ctx]() {
//            if (ctx && ctx->window() && ctx->window()->mouseGrabberItem()) {
//                // FIXME event forge thing enters press and hold move mode :/
//                ctx->window()->mouseGrabberItem()->ungrabMouse();
//            }
//        };
//
//        QTimer::singleShot(0, ctx, ungrabMouseHack);
        //end workaround

//        const auto &geo = ctx->window()->screen()->availableVirtualGeometry();
//
//        QPoint pos = ctx->window()->mapToGlobal(ctx->mapToScene(QPointF()).toPoint());
//        if (location() == Plasma::Types::TopEdge) {
//            pos.setY(pos.y() + ctx->height());
//        }

        actionMenu->adjustSize();

//        pos = QPoint(qBound(geo.x(), pos.x(), geo.x() + geo.width() - actionMenu->width()),
//                     qBound(geo.y(), pos.y(), geo.y() + geo.height() - actionMenu->height()));

//        if (view() == FullView) {
//            actionMenu->installEventFilter(this);
//        }

        actionMenu->winId();//create window handle
        actionMenu->windowHandle()->setTransientParent(this->windowHandle());
        actionMenu->popup(this->m_menuWidget->mapToGlobal(ctx->geometry().bottomLeft()));

//        if (view() == FullView) {
//            // hide the old menu only after showing the new one to avoid brief flickering
//            // in other windows as they briefly re-gain focus
//            QMenu *oldMenu = m_currentMenu;
//            m_currentMenu = actionMenu;
//            if (oldMenu && oldMenu != actionMenu) {
//                //! dont trigger initialization of index because there is a new menu created
//                disconnect(oldMenu, &QMenu::aboutToHide, this, &AppMenuApplet::onMenuAboutToHide);
//
//                oldMenu->hide();
//            }
//        }

//        setCurrentIndex(idx);

        // FIXME TODO connect only once
//        connect(actionMenu, &QMenu::aboutToHide, this, &AppMenuApplet::onMenuAboutToHide, Qt::UniqueConnection);
        return;
    }
}
