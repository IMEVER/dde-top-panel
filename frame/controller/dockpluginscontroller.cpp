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

#include "dockpluginscontroller.h"
#include "pluginsiteminterface.h"
#include "item/traypluginitem.h"

#include <QDebug>
#include <QDir>

DockPluginsController::DockPluginsController(QObject *parent)
    : AbstractPluginsController(parent)
{
}

void DockPluginsController::itemAdded(PluginsItemInterface *const itemInter, const QString &itemKey)
{
    QMap<PluginsItemInterface *, QMap<QString, QObject *>> &mPluginsMap = pluginsMap();

    // check if same item added
    if (mPluginsMap.contains(itemInter) and mPluginsMap[itemInter].contains(itemKey))
        return;

    // 取 plugin api
    QString pluginApi = "1.1.1";
    if(mPluginsMap[itemInter].contains("pluginloader"))
    {
        QPluginLoader *pluginLoader = qobject_cast<QPluginLoader*>(mPluginsMap[itemInter].value("pluginloader"));
        const QJsonObject &meta = pluginLoader->metaData().value("MetaData").toObject();
        pluginApi = meta.value("api").toString();
    }

    PluginsItem *item = nullptr;
    if (itemInter->pluginName() == "tray") {
        item = new TrayPluginItem(itemInter, itemKey, pluginApi);
        if (item->graphicsEffect())
            item->graphicsEffect()->setEnabled(false);

        item->centralWidget()->setProperty("iconSize", DEFAULT_HEIGHT - 4);
    } else {
        item = new PluginsItem(itemInter, itemKey, pluginApi);
    }

    mPluginsMap[itemInter][itemKey] = item;

    emit pluginItemInserted(item);
}

void DockPluginsController::itemUpdate(PluginsItemInterface *const itemInter, const QString &itemKey)
{
    if(PluginsItem *item = static_cast<PluginsItem *>(pluginItemAt(itemInter, itemKey))) {
        item->update();
        emit pluginItemUpdated(item);
    }
}

void DockPluginsController::itemRemoved(PluginsItemInterface *const itemInter, const QString &itemKey)
{
    if(PluginsItem *item = static_cast<PluginsItem *>(pluginItemAt(itemInter, itemKey))) {

        item->detachPluginWidget();

        emit pluginItemRemoved(item);

        QMap<PluginsItemInterface *, QMap<QString, QObject *>> &mPluginsMap = pluginsMap();
        mPluginsMap[itemInter].remove(itemKey);

        // just delete our wrapper object(PluginsItem)
        item->deleteLater();
    }
}
/*
void DockPluginsController::requestSetAppletVisible(PluginsItemInterface *const itemInter, const QString &itemKey, const bool visible)
{
    PluginsItem *item = static_cast<PluginsItem *>(pluginItemAt(itemInter, itemKey));
    if (!item)
        return;

    if (visible) {
        item->showPopupApplet(itemInter->itemPopupApplet(itemKey));
    } else {
        item->hidePopup();
    }
}
*/
void DockPluginsController::requestWindowAutoHide(PluginsItemInterface *const itemInter, const QString &itemKey, const bool autoHide)
{
    if(PluginsItem *item = static_cast<PluginsItem *>(pluginItemAt(itemInter, itemKey)))
        Q_EMIT item->requestWindowAutoHide(autoHide);
}
/*
void DockPluginsController::requestRefreshWindowVisible(PluginsItemInterface *const itemInter, const QString &itemKey)
{
    PluginsItem *item = static_cast<PluginsItem *>(pluginItemAt(itemInter, itemKey));
    if (!item)
        return;

    Q_EMIT item->requestRefreshWindowVisible();
}
*/

void DockPluginsController::startLoader()
{
    // QString pluginsDir(QString("%1/.local/lib/dde-top-panel/plugins/").arg(QDir::homePath()));
    QStringList pluginsDirs({QString("/usr/lib/dde-dock/plugins/"), QString("%1/.local/lib/dde-top-panel/plugins/").arg(QDir::homePath())});

    for(QString pluginsDir : pluginsDirs)
    {
        if (QDir(pluginsDir).exists()) {
            qDebug() << "using dock local plugins dir:" << pluginsDir;
            AbstractPluginsController::startLoader(new PluginLoader(pluginsDir, this));
        }
    }
}
