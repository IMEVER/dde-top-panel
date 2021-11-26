/*
 * Copyright (C) 2011 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     sbw <sbw@sbw.so>
 *
 * Maintainer: sbw <sbw@sbw.so>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "dockitem.h"
// #include "pluginsitem.h"

#include <QMouseEvent>
#include <QJsonObject>
#include <QCursor>

QPointer<DockPopupWindow> DockItem::PopupWindow(nullptr);

DockItem::DockItem(QWidget *parent)
    : QWidget(parent)
    , m_popupShown(false)
    , m_popupTipsDelayTimer(new QTimer(this))
    , m_popupAdjustDelayTimer(new QTimer(this))
{
    if (PopupWindow.isNull()) {
        DockPopupWindow *arrowRectangle = new DockPopupWindow(nullptr);
        arrowRectangle->setShadowBlurRadius(20);
        arrowRectangle->setRadius(6);
        arrowRectangle->setShadowYOffset(2);
        arrowRectangle->setShadowXOffset(0);
        arrowRectangle->setArrowWidth(18);
        arrowRectangle->setArrowHeight(10);
        PopupWindow = arrowRectangle;
    }

    m_popupTipsDelayTimer->setInterval(100);
    m_popupTipsDelayTimer->setSingleShot(true);

    m_popupAdjustDelayTimer->setInterval(10);
    m_popupAdjustDelayTimer->setSingleShot(true);

    connect(m_popupTipsDelayTimer, &QTimer::timeout, this, &DockItem::showHoverTips);
    connect(m_popupAdjustDelayTimer, &QTimer::timeout, this, &DockItem::updatePopupPosition, Qt::QueuedConnection);
    connect(&m_contextMenu, &QMenu::triggered, this, &DockItem::menuActionClicked);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

QSize DockItem::sizeHint() const
{
    int size = qMin(maximumWidth(), maximumHeight());

    return QSize(size, size);
}

QString DockItem::accessibleName()
{
    return QString();
}

DockItem::~DockItem()
{
    if (m_popupShown)
        hidePopup();
}

bool DockItem::event(QEvent *event)
{
    if (m_popupShown && PopupWindow->model() && popupApplet() == PopupWindow->getContent()) {
        switch (event->type()) {
        case QEvent::Paint:
            if (!m_popupAdjustDelayTimer->isActive())
                m_popupAdjustDelayTimer->start();
            break;
        default:;
        }
    }

    return QWidget::event(event);
}

void DockItem::updatePopupPosition()
{
    Q_ASSERT(sender() == m_popupAdjustDelayTimer);

    if (!m_popupShown || !PopupWindow->model())
        return;

    const QPoint p = popupMarkPoint();
    PopupWindow->show(p, PopupWindow->model());
}

void DockItem::mousePressEvent(QMouseEvent *e)
{
    m_popupTipsDelayTimer->stop();
    hideNonModel();

    if (e->button() == Qt::RightButton) {
        if (perfectIconRect().contains(e->pos())) {
            return showContextMenu();
        }
    }

    // same as e->ignore above
    QWidget::mousePressEvent(e);
}

void DockItem::enterEvent(QEvent *e)
{
    // Remove the bottom area to prevent unintentional operation in auto-hide mode.
    if (!rect().adjusted(0, 0, width(), height() - 5).contains(mapFromGlobal(QCursor::pos()))) {
        return;
    }

    m_popupTipsDelayTimer->start();

    return QWidget::enterEvent(e);
}

void DockItem::leaveEvent(QEvent *e)
{
    QWidget::leaveEvent(e);

    m_popupTipsDelayTimer->stop();

    // auto hide if popup is not model window
    if (m_popupShown && !PopupWindow->model())
        hidePopup();
}

const QRect DockItem::perfectIconRect() const
{
    const QRect itemRect = rect();
    QRect iconRect;

    if (itemType() == Plugins || itemType() == FixedPlugin)
    {
        iconRect.setWidth(itemRect.width());
        iconRect.setHeight(itemRect.height());
    }
    else
    {
        const int iconSize = std::min(itemRect.width(), itemRect.height()) * 0.8;
        iconRect.setWidth(iconSize);
        iconRect.setHeight(iconSize);
    }

    iconRect.moveTopLeft(itemRect.center() - iconRect.center());
    return iconRect;
}

void DockItem::showContextMenu()
{
    const QString menuJson = contextMenu();
    if (menuJson.isEmpty())
        return;

    QJsonDocument jsonDocument = QJsonDocument::fromJson(menuJson.toLocal8Bit().data());
    if (jsonDocument.isNull())
        return;

    QJsonObject jsonMenu = jsonDocument.object();

    qDeleteAll(m_contextMenu.actions());

    QJsonArray jsonMenuItems = jsonMenu.value("items").toArray();
    for (auto item : jsonMenuItems) {
        QJsonObject itemObj = item.toObject();
        QAction *action = new QAction(itemObj.value("itemText").toString());
        action->setCheckable(itemObj.value("isCheckable").toBool());
        action->setChecked(itemObj.value("checked").toBool());
        action->setData(itemObj.value("itemId").toString());
        action->setEnabled(itemObj.value("isActive").toBool());
        m_contextMenu.addAction(action);
    }

    hidePopup();

    emit requestWindowAutoHide(false);

    m_contextMenu.exec(QCursor::pos());

    onContextMenuAccepted();
}

void DockItem::menuActionClicked(QAction *action)
{
    invokedMenuItem(action->data().toString(), action->isChecked());
}

void DockItem::onContextMenuAccepted()
{
    emit requestWindowAutoHide(true);
}

void DockItem::showHoverTips()
{
    // another model popup window already exists
    if (PopupWindow->model())
    {
        if(QWidget *widget = popupApplet())
        {
            if(widget != PopupWindow->getContent())
                showPopupApplet(widget);
        }
    }
    else if (QRect(topleftPoint(), size()).contains(QCursor::pos()))
    {
        if (QWidget *const content = popupTips())
            showPopupWindow(content);
    }
}

void DockItem::showPopupApplet(QWidget *const applet)
{
    showPopupWindow(applet, true);
}

void DockItem::showPopupWindow(QWidget *const content, const bool model)
{
    if(model)
        emit requestWindowAutoHide(false);

    m_popupShown = true;

    PopupWindow->resize(content->sizeHint());
    PopupWindow->setContent(content);

    QVariant canFocus = content->property("canFocus");
    PopupWindow->setWindowFlag(Qt::WindowDoesNotAcceptFocus, !(canFocus.isValid() && canFocus.toBool()));

    const QPoint p = popupMarkPoint();
    if (PopupWindow->isVisible())
        PopupWindow->show(p, model);
    else
        QMetaObject::invokeMethod(PopupWindow, "show", Qt::QueuedConnection, Q_ARG(QPoint, p), Q_ARG(bool, model));

    connect(PopupWindow, &DockPopupWindow::accept, this, &DockItem::hidePopup, Qt::UniqueConnection);
}

void DockItem::invokedMenuItem(const QString &itemId, const bool checked)
{
    Q_UNUSED(itemId)
    Q_UNUSED(checked)
}

const QString DockItem::contextMenu() const
{
    return QString();
}

QWidget *DockItem::popupTips()
{
    return nullptr;
}

QWidget *DockItem::popupApplet()
{
    return nullptr;
}

const QPoint DockItem::popupMarkPoint()
{
    QPoint p(topleftPoint());
    const QRect r = rect();

    p += QPoint(r.width() / 2, r.height() + 10);

    return p;
}

const QPoint DockItem::topleftPoint() const
{
    QPoint p;
    const QWidget *w = this;
    do {
        p += w->pos();
        w = qobject_cast<QWidget *>(w->parent());
    } while (w);

    return p;
}

void DockItem::hidePopup()
{
    if (m_popupShown)
    {
        disconnect(PopupWindow.data(), &DockPopupWindow::accept, this, &DockItem::hidePopup);

        if(m_popupShown && PopupWindow->model())
            emit requestWindowAutoHide(true);

        m_popupTipsDelayTimer->stop();
        m_popupAdjustDelayTimer->stop();
        m_popupShown = false;
        PopupWindow->hide();
    }
}

void DockItem::hideNonModel()
{
    // auto hide if popup is not model window
    if (m_popupShown && !PopupWindow->model())
        hidePopup();
}
