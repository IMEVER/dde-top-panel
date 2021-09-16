/*
 * Copyright (C) 2019 ~ 2019 Deepin Technology Co., Ltd.
 *
 * Author:     wangshaojun <wangshaojun_cm@deepin.com>
 *
 * Maintainer: wangshaojun <wangshaojun_cm@deepin.com>
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

#include "dockitemmanager.h"
#include "item/pluginsitem.h"
#include "item/traypluginitem.h"

DockItemManager *DockItemManager::INSTANCE = nullptr;

DockItemManager::DockItemManager(QObject *parent)
    : QObject(parent)
    , m_updatePluginsOrderTimer(new QTimer(this))
    , m_pluginsInter(new DockPluginsController(this))
{
    // 更新插件顺序
    m_updatePluginsOrderTimer->setSingleShot(true);
    m_updatePluginsOrderTimer->setInterval(1000);
    connect(m_updatePluginsOrderTimer, &QTimer::timeout, this, &DockItemManager::updatePluginsItemOrderKey);

    // 插件信号
    connect(m_pluginsInter, &DockPluginsController::pluginItemInserted, this, &DockItemManager::pluginItemInserted, Qt::QueuedConnection);
    connect(m_pluginsInter, &DockPluginsController::pluginItemRemoved, this, &DockItemManager::pluginItemRemoved, Qt::QueuedConnection);
    connect(m_pluginsInter, &DockPluginsController::pluginItemUpdated, this, &DockItemManager::itemUpdated, Qt::QueuedConnection);

    // 刷新图标
    QMetaObject::invokeMethod(this, "refershItemsIcon", Qt::QueuedConnection);
}

DockItemManager *DockItemManager::instance(QObject *parent)
{
    if (!INSTANCE)
        INSTANCE = new DockItemManager(parent);

    return INSTANCE;
}

const QList<QPointer<DockItem>> DockItemManager::itemList() const
{
    return m_itemList;
}

const QList<PluginsItemInterface *> DockItemManager::pluginList() const
{
    return m_pluginsInter->pluginsMap().keys();
}

void DockItemManager::startLoadPlugins() const
{
    m_pluginsInter->startLoader();
}

void DockItemManager::refershItemsIcon()
{
    for (auto item : m_itemList) {
        item->refershIcon();
        item->update();
    }
}

void DockItemManager::updatePluginsItemOrderKey()
{
    Q_ASSERT(sender() == m_updatePluginsOrderTimer);

    int index = 0;
    for (auto item : m_itemList) {
        if (item.isNull() || item->itemType() != DockItem::Plugins)
            continue;
        static_cast<PluginsItem *>(item.data())->setItemSortKey(++index);
    }

    // 固定区域插件排序
    index = 0;
    for (auto item : m_itemList) {
        if (item.isNull() || item->itemType() != DockItem::FixedPlugin)
            continue;
        static_cast<PluginsItem *>(item.data())->setItemSortKey(++index);
    }
}

void DockItemManager::itemMoved(DockItem *const sourceItem, DockItem *const targetItem)
{
    Q_ASSERT(sourceItem != targetItem);

    const DockItem::ItemType moveType = sourceItem->itemType();
    const DockItem::ItemType replaceType = targetItem->itemType();

    // plugins move
    if (moveType == DockItem::Plugins || moveType == DockItem::TrayPlugin)
        if (replaceType != DockItem::Plugins && replaceType != DockItem::TrayPlugin)
            return;

    const int moveIndex = m_itemList.indexOf(sourceItem);
    const int replaceIndex = m_itemList.indexOf(targetItem);

    m_itemList.removeAt(moveIndex);
    m_itemList.insert(replaceIndex, sourceItem);

    // update plugins sort key if order changed
    if (moveType == DockItem::Plugins || replaceType == DockItem::Plugins
            || moveType == DockItem::TrayPlugin || replaceType == DockItem::TrayPlugin
            || moveType == DockItem::FixedPlugin || replaceType == DockItem::FixedPlugin)
        m_updatePluginsOrderTimer->start();
}

void DockItemManager::pluginItemInserted(PluginsItem *item)
{
    DockItem::ItemType pluginType = item->itemType();

    // find first plugins item position
    int firstPluginPosition = m_itemList.size();
    for (int i(0); i != m_itemList.size(); ++i)
    {
        DockItem::ItemType type = m_itemList[i]->itemType();
        if (type != pluginType)
            continue;

        firstPluginPosition = i;
        break;
    }

    // find insert position
    int insertIndex = 0;
    const int itemSortKey = item->itemSortKey();
    if (firstPluginPosition == m_itemList.size()) {
        insertIndex = m_itemList.size();
    } else if (itemSortKey == 0) {
        insertIndex = firstPluginPosition;
    } else {
        insertIndex = m_itemList.size();
        for (int i(firstPluginPosition); i < m_itemList.size(); ++i) {
            PluginsItem *pItem = static_cast<PluginsItem *>(m_itemList[i].data());
            Q_ASSERT(pItem);

            const int sortKey = pItem->itemSortKey();
            if (pluginType == pItem->itemType())
            {
                if (itemSortKey >= sortKey)
                    continue;
                insertIndex = i;
                break;
            }
            else
            {
                insertIndex = i;
                break;
            }
        }
    }

    m_itemList.insert(insertIndex, item);

    // qDebug()<<item->pluginName()<<":\t"<<insertIndex<<", "<<firstPluginPosition<<endl;

    emit itemInserted(insertIndex - firstPluginPosition, item);
    connect(item, &DockItem::requestWindowAutoHide, this, &DockItemManager::requestWindowAutoHide);
    connect(item, &DockItem::requestRefreshWindowVisible, this, &DockItemManager::requestRefershWindowVisible, Qt::UniqueConnection);
}

void DockItemManager::pluginItemRemoved(PluginsItem *item)
{
    item->hidePopup();

    emit itemRemoved(item);

    m_itemList.removeOne(item);
}
