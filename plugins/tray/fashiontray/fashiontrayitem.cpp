/*
 * Copyright (C) 2011 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     listenerri <listenerri@gmail.com>
 *
 * Maintainer: listenerri <listenerri@gmail.com>
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

#include "fashiontrayitem.h"
#include "fashiontray/fashiontrayconstants.h"
#include "system-trays/systemtrayitem.h"
#include "containers/abstractcontainer.h"

#include <QDebug>
#include <QResizeEvent>

#define ExpandedKey "fashion-tray-expanded"

int FashionTrayItem::TrayWidgetWidth = PLUGIN_BACKGROUND_MAX_SIZE;
int FashionTrayItem::TrayWidgetHeight = PLUGIN_BACKGROUND_MAX_SIZE;

FashionTrayItem::FashionTrayItem(TrayPlugin *trayPlugin, QWidget *parent)
    : QWidget(parent),
      m_mainBoxLayout(new QBoxLayout(QBoxLayout::Direction::LeftToRight)),
      m_trayPlugin(trayPlugin),
      m_controlWidget(new FashionTrayControlWidget()),
      m_currentDraggingTray(nullptr),
      m_normalContainer(new NormalContainer(m_trayPlugin)),
      m_leftSpace(new QWidget),
      m_gsettings(new QGSettings("me.imever.dde.toppanel"))
{
    setAcceptDrops(true);

    m_normalContainer->setVisible(false);

    m_mainBoxLayout->setMargin(0);
    m_mainBoxLayout->setContentsMargins(0, 0, 0, 0);
    m_mainBoxLayout->setSpacing(0);

    m_leftSpace->setFixedSize(TraySpace, TraySpace);

    m_mainBoxLayout->addWidget(m_leftSpace);
    m_mainBoxLayout->addWidget(m_normalContainer);
    m_mainBoxLayout->addWidget(m_controlWidget);

    setLayout(m_mainBoxLayout);

    if(m_gsettings->get("traySupportFold").toBool())
    {
        connect(m_controlWidget, &FashionTrayControlWidget::expandChanged, this, &FashionTrayItem::onExpandChanged);
    }
    else
    {
        m_controlWidget->hide();
    }

    connect(m_gsettings, &QGSettings::changed, this, [ = ](const QString &key){
        if (key == "traySupportFold")
        {
                if(m_gsettings->get("traySupportFold").toBool())
                {
                    m_controlWidget->show();
                    connect(m_controlWidget, &FashionTrayControlWidget::expandChanged, this, &FashionTrayItem::onExpandChanged);
                }
                else
                {
                    m_controlWidget->hide();
                     disconnect(m_controlWidget, &FashionTrayControlWidget::expandChanged, this, &FashionTrayItem::onExpandChanged);
                     if (m_trayPlugin->getValue(FASHION_MODE_ITEM_KEY, ExpandedKey, true).toBool() == false)
                     {
                         m_controlWidget->setExpanded(true);
                         this->onExpandChanged(true);
                     }
                }
        }
    });

    connect(m_normalContainer, &NormalContainer::requestDraggingWrapper, this, &FashionTrayItem::onRequireDraggingWrapper);

    // do not call init immediately the TrayPlugin has not be constructed for now
    QTimer::singleShot(0, this, &FashionTrayItem::init);
}

void FashionTrayItem::setTrayWidgets(const QMap<QString, AbstractTrayWidget *> &itemTrayMap)
{
    clearTrayWidgets();

    for (auto it = itemTrayMap.constBegin(); it != itemTrayMap.constEnd(); ++it) {
        if(it.value())
            trayWidgetAdded(it.key(), it.value());
    }
}

void FashionTrayItem::trayWidgetAdded(const QString &itemKey, AbstractTrayWidget *trayWidget)
{
    if (m_normalContainer->containsWrapperByTrayWidget(trayWidget)) {
        qDebug() << "Reject! want to insert duplicate trayWidget:" << itemKey << trayWidget;
        return;
    }

    FashionTrayWidgetWrapper *wrapper = new FashionTrayWidgetWrapper(itemKey, trayWidget);

        if (m_normalContainer->acceptWrapper(wrapper))
            m_normalContainer->addWrapper(wrapper);

    requestResize();
}

void FashionTrayItem::trayWidgetRemoved(AbstractTrayWidget *trayWidget)
{
    bool deleted = false;

        if (m_normalContainer->removeWrapperByTrayWidget(trayWidget)) {
            deleted = true;
        }

    if (!deleted) {
        qDebug() << "Error! can not find the tray widget in fashion tray list" << trayWidget;
    }

    requestResize();
}

void FashionTrayItem::clearTrayWidgets()
{
    m_normalContainer->clearWrapper();

    requestResize();
}

void FashionTrayItem::onExpandChanged(const bool expand)
{
    m_trayPlugin->saveValue(FASHION_MODE_ITEM_KEY, ExpandedKey, expand);

    m_normalContainer->setExpand(expand);

    requestResize();
}

void FashionTrayItem::setRightSplitVisible(const bool visible)
{
}

void FashionTrayItem::onPluginSettingsChanged()
{
    m_controlWidget->setExpanded(m_trayPlugin->getValue(FASHION_MODE_ITEM_KEY, ExpandedKey, true).toBool());
}

void FashionTrayItem::showEvent(QShowEvent *event)
{
    requestResize();

    QWidget::showEvent(event);
}

void FashionTrayItem::hideEvent(QHideEvent *event)
{
    requestResize();

    QWidget::hideEvent(event);
}

void FashionTrayItem::resizeEvent(QResizeEvent *event)
{
    resizeTray();
    QWidget::resizeEvent(event);
}

void FashionTrayItem::dragEnterEvent(QDragEnterEvent *event)
{
    // accept but do not handle the trays drag event
    // in order to avoid the for forbidden label displayed on the mouse
    if (event->mimeData()->hasFormat(TRAY_ITEM_DRAG_MIMEDATA)) {
        event->accept();
        return;
    }

    QWidget::dragEnterEvent(event);
}

void FashionTrayItem::init()
{
    qDebug() << "init Fashion mode tray plugin item";
    m_controlWidget->setExpanded(m_trayPlugin->getValue(FASHION_MODE_ITEM_KEY, ExpandedKey, true).toBool());
    onExpandChanged(m_controlWidget->expanded());
}

void FashionTrayItem::requestResize()
{
    // 通知dock，当前托盘有几个图标显示，用来计算图标大小
    m_leftSpace->setVisible(!m_controlWidget->expanded());

    int count = m_normalContainer->itemCount();
    setProperty("TrayVisableItemCount", count + 1); // +1 : m_controlWidget

    resizeTray();
}

void FashionTrayItem::onRequireDraggingWrapper()
{
    AbstractContainer *container = dynamic_cast<AbstractContainer *>(sender());

    if (!container) {
        return;
    }

    FashionTrayWidgetWrapper *draggingWrapper = m_normalContainer->takeDraggingWrapper();

    if (!draggingWrapper) {
        return;
    }

    container->addDraggingWrapper(draggingWrapper);
}

bool FashionTrayItem::event(QEvent *event)
{
    if (event->type() == QEvent::DynamicPropertyChange) {
        const QString &propertyName = static_cast<QDynamicPropertyChangeEvent *>(event)->propertyName();
        if (propertyName == "iconSize") {
            m_iconSize = property("iconSize").toInt();
            m_normalContainer->setItemSize(m_iconSize);

            resizeTray();
        }
    }

    return QWidget::event(event);
}

void FashionTrayItem::resizeTray()
{
    if (!m_iconSize)
        return;

    m_mainBoxLayout->setContentsMargins(0, 0, 0, 0);

    m_normalContainer->updateSize();
}
