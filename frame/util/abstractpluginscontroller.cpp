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

#include "abstractpluginscontroller.h"
#include "pluginsiteminterface.h"
#include "DNotifySender"

#include <DSysInfo>
#include <QDebug>
#include <QGSettings>

static const QSet<QString> CompatiblePluginApiList {
    "1.1.1",
    "1.2",
    "1.2.1",
    "1.2.2",
    DOCK_PLUGIN_API_VERSION
};

AbstractPluginsController::AbstractPluginsController(QObject *parent)
    : QObject(parent)
    , m_dbusDaemonInterface(QDBusConnection::sessionBus().interface())
    , m_gsettings(new QGSettings("me.imever.dde.toppanel"))
{
    m_pluginSettingsObject = QJsonDocument::fromJson(m_gsettings->get("plugin-settings").toString().toUtf8()).object();
}

void AbstractPluginsController::saveValue(PluginsItemInterface *const itemInter, const QString &key, const QVariant &value)
{
    // save to local cache
    QJsonObject localObject = m_pluginSettingsObject.value(itemInter->pluginName()).toObject();
    localObject.insert(key, QJsonValue::fromVariant(value)); //Note: QVariant::toJsonValue() not work in Qt 5.7
    m_pluginSettingsObject.insert(itemInter->pluginName(), localObject);

    QString strJson(QJsonDocument(m_pluginSettingsObject).toJson(QJsonDocument::Compact));
    m_gsettings->set("plugin-settings", QVariant(strJson));
}

const QVariant AbstractPluginsController::getValue(PluginsItemInterface *const itemInter, const QString &key, const QVariant &fallback)
{
    // load from local cache
    QVariant v = m_pluginSettingsObject.value(itemInter->pluginName()).toObject().value(key).toVariant();
    if (v.isNull() || !v.isValid()) 
        v = fallback;

    return v;
}

void AbstractPluginsController::removeValue(PluginsItemInterface *const itemInter, const QStringList &keyList)
{
    if (keyList.isEmpty()) {
        m_pluginSettingsObject.remove(itemInter->pluginName());
    } else {
        QJsonObject localObject = m_pluginSettingsObject.value(itemInter->pluginName()).toObject();
        for (auto key : keyList) {
            localObject.remove(key);
        }
        m_pluginSettingsObject.insert(itemInter->pluginName(), localObject);
    }
    m_gsettings->set("plugin-settings", m_pluginSettingsObject);
}

QMap<PluginsItemInterface *, QMap<QString, QObject *> > &AbstractPluginsController::pluginsMap()
{
    return m_pluginsMap;
}

QObject *AbstractPluginsController::pluginItemAt(PluginsItemInterface *const itemInter, const QString &itemKey) const
{
    if (!m_pluginsMap.contains(itemInter) or !m_pluginsMap[itemInter].contains(itemKey))
        return nullptr;

    return m_pluginsMap[itemInter][itemKey];
}

PluginsItemInterface *AbstractPluginsController::pluginInterAt(const QString &itemKey)
{
    for (auto it = m_pluginsMap.constBegin(); it != m_pluginsMap.constEnd(); ++it) {
        if (it.value().keys().contains(itemKey))
            return it.key();
    }

    return nullptr;
}

PluginsItemInterface *AbstractPluginsController::pluginInterAt(QObject *destItem)
{
    for (auto it = m_pluginsMap.constBegin(); it != m_pluginsMap.constEnd(); ++it) {
        if (it.value().values().contains(destItem))
            return it.key();
    }

    return nullptr;
}

void AbstractPluginsController::startLoader(PluginLoader *loader)
{
    connect(loader, &PluginLoader::finished, loader, &PluginLoader::deleteLater, Qt::QueuedConnection);
    connect(loader, &PluginLoader::pluginFounded, this, [ this ](const QString &pluginFile){
        m_pluginLoadMap.insert(pluginFile);
        loadPlugin(pluginFile);
    });

    QTimer::singleShot(m_gsettings->get("delay-plugins-time").toUInt(), loader, [ loader ] { loader->start(QThread::LowestPriority); });
}

void AbstractPluginsController::loadPlugin(const QString &pluginFile)
{
    QPluginLoader *pluginLoader = new QPluginLoader(pluginFile);
    const QJsonObject &meta = pluginLoader->metaData().value("MetaData").toObject();
    const QString &pluginApi = meta.value("api").toString();

    if (pluginApi.isEmpty() || !CompatiblePluginApiList.contains(pluginApi)) {
        qWarning() << objectName()
                   << "plugin api version not matched! expect versions:" << CompatiblePluginApiList
                   << ", got version:" << pluginApi
                   << ", the plugin file is:" << pluginFile;

        pluginLoader->unload();
        pluginLoader->deleteLater();

        m_pluginLoadMap.remove(pluginFile);
        QString notifyMessage(tr("The plugin %1 is not compatible with the system."));
        Dtk::Core::DUtil::DNotifySender(notifyMessage.arg(QFileInfo(pluginFile).fileName())).appIcon("dialog-warning").call();

        if (m_pluginLoadMap.isEmpty()) emit pluginLoaderFinished();
        return;
    }

    PluginsItemInterface *interface = qobject_cast<PluginsItemInterface *>(pluginLoader->instance());
    if (!interface) {
        qWarning() << objectName() << "load plugin failed!!!" << pluginLoader->errorString() << pluginFile;

        pluginLoader->unload();
        pluginLoader->deleteLater();

        m_pluginLoadMap.remove(pluginFile);
        QString notifyMessage(tr("The plugin %1 is not compatible with the system."));
        Dtk::Core::DUtil::DNotifySender(notifyMessage.arg(QFileInfo(pluginFile).fileName())).appIcon("dialog-warning").call();

        if (m_pluginLoadMap.isEmpty()) emit pluginLoaderFinished();
        return;
    }

    if (interface->pluginName() == "multitasking") {
        if (qEnvironmentVariable("XDG_SESSION_TYPE").contains("wayland") or Dtk::Core::DSysInfo::deepinType() == Dtk::Core::DSysInfo::DeepinServer)
        {
            m_pluginLoadMap.remove(pluginFile);
            pluginLoader->unload();
            pluginLoader->deleteLater();
            delete interface;

            if (m_pluginLoadMap.isEmpty()) emit pluginLoaderFinished();
            return;
        }
    }

    // 保存 PluginLoader 对象指针
    m_pluginsMap.insert(interface, {{"pluginloader", pluginLoader}});

    QString dbusService = meta.value("depends-daemon-dbus-service").toString();
    if (!dbusService.isEmpty() && !m_dbusDaemonInterface->isServiceRegistered(dbusService).value())
    {
        static int dbusCount = 0;
        dbusCount++;
        qDebug() << objectName() << dbusService << "daemon has not started, waiting for signal";
        connect(m_dbusDaemonInterface, &QDBusConnectionInterface::serviceOwnerChanged, this,
            [ this, dbusService, interface, pluginFile ](const QString & name, const QString & oldOwner, const QString & newOwner) {
                Q_UNUSED(oldOwner);
                if (name == dbusService && !newOwner.isEmpty()) {
                    qDebug() << objectName() << dbusService << "daemon started, init plugin and disconnect";
                    initPlugin(interface, pluginFile);
                    if(--dbusCount < 1)
                        disconnect(m_dbusDaemonInterface);
                }
            });
        return;
    }

    // NOTE(justforlxz): 插件的所有初始化工作都在init函数中进行，
    // loadPlugin函数是按队列执行的，initPlugin函数会有可能导致
    // 函数执行被阻塞。
    QTimer::singleShot(1, this, [ this, interface, pluginFile ] {
        initPlugin(interface, pluginFile);
    });
}

void AbstractPluginsController::initPlugin(PluginsItemInterface *interface, const QString &pluginFile)
{
    qDebug() << objectName() << "init plugin: " << interface->pluginName();
    interface->init(this);

    m_pluginLoadMap.remove(pluginFile);

    //插件全部加载完成
    if (m_pluginLoadMap.isEmpty()) emit pluginLoaderFinished();

    qDebug() << objectName() << "init plugin finished: " << interface->pluginName();
}

void AbstractPluginsController::refreshPluginSettings()
{
    QJsonObject pluginSettingsObject = QJsonDocument::fromJson(m_gsettings->get("plugin-settings").toString().toUtf8()).object();

    // nothing changed
    if (pluginSettingsObject == m_pluginSettingsObject) return;

    for (auto pluginsIt = pluginSettingsObject.constBegin(); pluginsIt != pluginSettingsObject.constEnd(); ++pluginsIt) {
        const QString &pluginName = pluginsIt.key();
        const QJsonObject &settingsObject = pluginsIt.value().toObject();
        QJsonObject newSettingsObject = m_pluginSettingsObject.value(pluginName).toObject();
        for (auto settingsIt = settingsObject.constBegin(); settingsIt != settingsObject.constEnd(); ++settingsIt) {
            newSettingsObject.insert(settingsIt.key(), settingsIt.value());
        }
        // TODO: remove not exists key-values
        m_pluginSettingsObject.insert(pluginName, newSettingsObject);
    }

    // not notify plugins to refresh settings if this update is not emit by dock daemon
    if (sender() != m_gsettings) return;


    // notify all plugins to reload plugin settings
    for (PluginsItemInterface *pluginInter : m_pluginsMap.keys())
        pluginInter->pluginSettingsChanged();

    qDebug()<<"settings: "<<m_pluginSettingsObject<<Qt::endl;

    // reload all plugin items for sort order or container
    QMap<PluginsItemInterface *, QMap<QString, QObject *>> pluginsMapTemp = m_pluginsMap;
    for (auto it = pluginsMapTemp.constBegin(); it != pluginsMapTemp.constEnd(); ++it) {
        QList<QString> itemKeyList = it.value().keys();
        itemKeyList.removeOne("pluginloader");
        for (auto key : itemKeyList)
            itemRemoved(it.key(), key);

        for (auto key : itemKeyList)
            itemAdded(it.key(), key);
    }
}