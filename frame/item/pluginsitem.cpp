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

#include "constants.h"
#include "pluginsitem.h"
#include "pluginsiteminterface.h"

#include <QPainter>
#include <QBoxLayout>
#include <QMouseEvent>

#define PLUGIN_ITEM_DRAG_THRESHOLD      20

QPoint PluginsItem::MousePressPoint = QPoint();

PluginsItem::PluginsItem(PluginsItemInterface *const pluginInter, const QString &itemKey, const QString &api, QWidget *parent)
    : DockItem(parent)
    , m_pluginInter(pluginInter)
    , m_centralWidget(m_pluginInter->itemWidget(itemKey))
    , m_itemKey(itemKey)
    , m_api(api)
{
    qDebug() << "load plugins item: " << pluginInter->pluginName() << itemKey << pluginInter->type() << itemType();

    m_centralWidget->setParent(this);
    m_centralWidget->setVisible(true);

    QBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addWidget(m_centralWidget);
    hLayout->setSpacing(0);
    hLayout->setMargin(0);

    setLayout(hLayout);
    setAttribute(Qt::WA_TranslucentBackground);
}

PluginsItem::~PluginsItem()
{
}

int PluginsItem::itemSortKey() const
{
    return m_pluginInter->itemSortKey(m_itemKey);
}

void PluginsItem::setItemSortKey(const int order) const
{
    m_pluginInter->setSortKey(m_itemKey, order);
}

void PluginsItem::detachPluginWidget()
{
    QWidget *widget = m_pluginInter->itemWidget(m_itemKey);
    if (widget)
        widget->setParent(nullptr);
}

QString PluginsItem::pluginName() const
{
    return m_pluginInter->pluginName();
}

DockItem::ItemType PluginsItem::itemType() const
{
    if (m_pluginInter->type() == PluginsItemInterface::Normal) {
        return Plugins;
    } else {
        return FixedPlugin;
    }
}

PluginsItemInterface::PluginSizePolicy PluginsItem::pluginSizePolicy() const
{
    if(this->comparePluginApi(m_api, "1.2.2") > 0)
        return m_pluginInter->pluginSizePolicy();
    else
        return PluginsItemInterface::System;
}

QSize PluginsItem::sizeHint() const
{
    return m_centralWidget->sizeHint();
}

void PluginsItem::refershIcon()
{
    m_pluginInter->refreshIcon(m_itemKey);
}

QWidget *PluginsItem::centralWidget() const
{
    return m_centralWidget;
}

void PluginsItem::setDraging(bool bDrag)
{
    m_centralWidget->setVisible(!bDrag);
}

void PluginsItem::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
        MousePressPoint = e->pos();

    // context menu will handle in DockItem
    DockItem::mousePressEvent(e);
}

void PluginsItem::mouseReleaseEvent(QMouseEvent *e)
{
    DockItem::mouseReleaseEvent(e);

    if (e->button() != Qt::LeftButton)
        return;

    e->accept();

    const QPoint distance = e->pos() - MousePressPoint;
    if (distance.manhattanLength() < PLUGIN_ITEM_DRAG_THRESHOLD)
        mouseClicked();
}

void PluginsItem::invokedMenuItem(const QString &itemId, const bool checked)
{
    m_pluginInter->invokedMenuItem(m_itemKey, itemId, checked);
}

const QString PluginsItem::contextMenu() const
{
    return m_pluginInter->itemContextMenu(m_itemKey);
}

QWidget *PluginsItem::popupTips()
{
    return m_pluginInter->itemTipsWidget(m_itemKey);
}

QWidget *PluginsItem::popupApplet()
{
    return m_pluginInter->itemPopupApplet(m_itemKey);
}

void PluginsItem::resizeEvent(QResizeEvent *event)
{
    setMaximumSize(m_centralWidget->maximumSize());
    return DockItem::resizeEvent(event);
}

void PluginsItem::mouseClicked()
{
    const QString command = m_pluginInter->itemCommand(m_itemKey);
    if (!command.isEmpty()) {
        QStringList args = command.split(" ");
        QProcess::startDetached(args.takeFirst(), args);
    } else if( QWidget *w = m_pluginInter->itemPopupApplet(m_itemKey)){
        showPopupApplet(w);
    }
}

/**
* @brief 比较两个插件版本号的大小
* @param pluginApi1 第一个插件版本号
* @param pluginApi2 第二个插件版本号
* @return 0:两个版本号相等,1:第一个版本号大,-1:第二个版本号大
*/
int PluginsItem::comparePluginApi(QString pluginApi1, QString pluginApi2) const {
    // 版本号相同
    if (pluginApi1 == pluginApi2)
        return 0;

    // 拆分版本号
    QStringList subPluginApis1 = pluginApi1.split(".", Qt::SkipEmptyParts, Qt::CaseSensitive);
    QStringList subPluginApis2 = pluginApi2.split(".", Qt::SkipEmptyParts, Qt::CaseSensitive);
    for (int i = 0; i < subPluginApis1.size(); ++i) {
        auto subPluginApi1 = subPluginApis1[i];
        if (subPluginApis2.size() > i) {
            auto subPluginApi2 = subPluginApis2[i];

            // 相等判断下一个子版本号
            if (subPluginApi1 == subPluginApi2)
                continue;

            // 转成整形比较
            if (subPluginApi1.toInt() > subPluginApi2.toInt()) {
                return 1;
            } else {
                return -1;
            }
        }
    }

    // 循环结束但是没有返回,说明子版本号个数不同,且前面的子版本号都相同
    // 子版本号多的版本号大
    if (subPluginApis1.size() > subPluginApis2.size()) {
        return 1;
    } else {
        return -1;
    }
}