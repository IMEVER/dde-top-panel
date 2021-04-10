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

#include "datetimeplugin.h"

#include <DDBusSender>
#include <QLabel>
#include <QDebug>
#include <QDBusConnectionInterface>

#include <unistd.h>

#define PLUGIN_STATE_KEY "enable"
#define TIME_FORMAT_KEY "Use24HourFormat"
#define DATE_SHOW_KEY "showDate"
#define WEEK_SHOW_KEY "showWeek"
#define LUNAR_SHOW_KEY "showLunar"

DatetimePlugin::DatetimePlugin(QObject *parent)
    : QObject(parent)
    , m_pluginLoaded(false)
{
    QDBusConnection sessionBus = QDBusConnection::sessionBus();
    sessionBus.connect("com.deepin.daemon.Timedate", "/com/deepin/daemon/Timedate", "org.freedesktop.DBus.Properties",  "PropertiesChanged", this, SLOT(propertiesChanged()));
}

const QString DatetimePlugin::pluginName() const
{
    return "datetime";
}

const QString DatetimePlugin::pluginDisplayName() const
{
    return "时间";
}

void DatetimePlugin::init(PluginProxyInterface *proxyInter)
{
    m_proxyInter = proxyInter;

    if (pluginIsDisable()) {
        return;
    }

    loadPlugin();
}

void DatetimePlugin::loadPlugin()
{
    if (m_pluginLoaded)
        return;

    m_pluginLoaded = true;
    m_dateTipsLabel = new TipsWidget;
    m_refershTimer = new QTimer(this);
    m_dateTipsLabel->setObjectName("datetime");

    m_refershTimer->setInterval(1000);
    m_refershTimer->start();

    m_centralWidget = new DatetimeWidget;
    m_centralWidget->setShowDate(m_proxyInter->getValue(this, DATE_SHOW_KEY, true).toBool());
    m_centralWidget->setShowLunar(m_proxyInter->getValue(this, LUNAR_SHOW_KEY, true).toBool());

    connect(m_centralWidget, &DatetimeWidget::requestUpdateGeometry, [this] { m_proxyInter->itemUpdate(this, pluginName()); });
    connect(m_refershTimer, &QTimer::timeout, this, &DatetimePlugin::updateCurrentTimeString);

    m_proxyInter->itemAdded(this, pluginName());

    pluginSettingsChanged();
}

void DatetimePlugin::pluginStateSwitched()
{
    m_proxyInter->saveValue(this, PLUGIN_STATE_KEY, pluginIsDisable());

    refreshPluginItemsVisible();
}

bool DatetimePlugin::pluginIsDisable()
{
    return !(m_proxyInter->getValue(this, PLUGIN_STATE_KEY, true).toBool());
}

int DatetimePlugin::itemSortKey(const QString &itemKey)
{
    Q_UNUSED(itemKey);
    return m_proxyInter->getValue(this, "pos", 5).toInt();
}

void DatetimePlugin::setSortKey(const QString &itemKey, const int order)
{
    Q_UNUSED(itemKey);
    m_proxyInter->saveValue(this, "pos", order);
}

QWidget *DatetimePlugin::itemWidget(const QString &itemKey)
{
    Q_UNUSED(itemKey);

    return m_centralWidget;
}

QWidget *DatetimePlugin::itemTipsWidget(const QString &itemKey)
{
    Q_UNUSED(itemKey);

    return m_dateTipsLabel;
}

const QString DatetimePlugin::itemCommand(const QString &itemKey)
{
    Q_UNUSED(itemKey);

    return "dbus-send --print-reply --dest=com.deepin.Calendar /com/deepin/Calendar com.deepin.Calendar.RaiseWindow";
}

const QString DatetimePlugin::itemContextMenu(const QString &itemKey)
{
    Q_UNUSED(itemKey);

    QList<QVariant> items;
    items.reserve(1);

    QMap<QString, QVariant> settings;
    settings["itemId"] = "settings";
    if (m_centralWidget->is24HourFormat())
        settings["itemText"] = "12小时制";
    else
        settings["itemText"] = "24小时制";
    settings["isActive"] = true;
    items.push_back(settings);

    QMap<QString, QVariant> showDate;
    showDate["itemId"] = "showDate";
    showDate["itemText"] = m_centralWidget->isShowDate() ? "隐藏日期" : "显示日期";
    showDate["isActive"] = true;
    items.push_back(showDate);

    QMap<QString, QVariant> showWeek;
    showWeek["itemId"] = "showWeek";
    showWeek["itemText"] = m_centralWidget->isShowWeek() ? "隐藏星期" : "显示星期";
    showWeek["isActive"] = true;
    items.push_back(showWeek);

    QMap<QString, QVariant> showLunar;
    showLunar["itemId"] = "showLunar";
    showLunar["itemText"] = m_centralWidget->isShowLunar() ? "隐藏六十甲子" : "显示六十甲子";
    showLunar["isActive"] = true;
    items.push_back(showLunar);

    QMap<QString, QVariant> open;
    open["itemId"] = "open";
    open["itemText"] = "时间设置";
    open["isActive"] = true;
    items.push_back(open);

    QMap<QString, QVariant> menu;
    menu["items"] = items;
    menu["checkableMenu"] = false;
    menu["singleCheck"] = false;

    return QJsonDocument::fromVariant(menu).toJson();
}

void DatetimePlugin::invokedMenuItem(const QString &itemKey, const QString &menuId, const bool checked)
{
    Q_UNUSED(itemKey)
    Q_UNUSED(checked)

    if (menuId == "open") 
    {
        DDBusSender()
        .service("com.deepin.dde.ControlCenter")
        .interface("com.deepin.dde.ControlCenter")
        .path("/com/deepin/dde/ControlCenter")
        .method(QString("ShowModule"))
        .arg(QString("datetime"))
        .call();
    }
    else if (menuId == "showDate")
    {
        m_centralWidget->setShowDate(!m_centralWidget->isShowDate());
        m_proxyInter->saveValue(this, DATE_SHOW_KEY, m_centralWidget->isShowDate());
    }
    else if (menuId == "showWeek")
    {	
	m_centralWidget->setShowWeek(!m_centralWidget->isShowWeek());
	m_proxyInter->saveValue(this, WEEK_SHOW_KEY, m_centralWidget->isShowWeek());
    }
    else if (menuId == "showLunar")
    {
        m_centralWidget->setShowLunar(!m_centralWidget->isShowLunar());
        m_proxyInter->saveValue(this, LUNAR_SHOW_KEY, m_centralWidget->isShowLunar());
    }    
    else
    {
        const bool value = is24HourFormat();
        save24HourFormat(!value);
        m_centralWidget->set24HourFormat(!value);
    }
}

void DatetimePlugin::pluginSettingsChanged()
{
    if (!m_pluginLoaded)
        return;

    const bool value = is24HourFormat();

    m_proxyInter->saveValue(this, TIME_FORMAT_KEY, value);
    m_centralWidget->set24HourFormat(value);

    refreshPluginItemsVisible();
}

void DatetimePlugin::updateCurrentTimeString()
{
    const QDateTime currentDateTime = QDateTime::currentDateTime();

    if(tips.count() == 0 || m_dateTipsLabel->isVisible())
    {
	    int h = currentDateTime.time().hour();

	    if (tips.count() == 0 || h != hour)
	    {
		    hour = h;
		    tips = m_centralWidget->dateString();
	    }

	    QStringList t;
	    foreach(QString s, tips)
		    t.append(s);
	    
	    if (m_centralWidget->is24HourFormat())
	    {
		    t.append("时间：" + currentDateTime.toString(" HH:mm:ss"));
	    }
	    else
	    {
		    t.append("时间：" + currentDateTime.toString(" hh:mm:ss A"));
	    }
	    m_dateTipsLabel->setTextList(t);
    }
    const int min = currentDateTime.time().minute();

    if (min == minute)
        return;

    minute = min;
    m_centralWidget->update();
}

void DatetimePlugin::refreshPluginItemsVisible()
{
    if (!pluginIsDisable()) {

        if (!m_pluginLoaded) {
            loadPlugin();
            return;
        }
        m_proxyInter->itemAdded(this, pluginName());
    } else {
        m_proxyInter->itemRemoved(this, pluginName());
    }
}

void DatetimePlugin::propertiesChanged()
{
    pluginSettingsChanged();
}

bool DatetimePlugin::is24HourFormat()
{
    QDBusInterface *m_interface;
        if (QDBusConnection::sessionBus().interface()->isServiceRegistered("com.deepin.daemon.Timedate")) {
            m_interface = new QDBusInterface("com.deepin.daemon.Timedate", "/com/deepin/daemon/Timedate", "com.deepin.daemon.Timedate");
        } else {
            QString path = QString("/com/deepin/daemon/Accounts/User%1").arg(QString::number(getuid()));
            m_interface = new QDBusInterface("com.deepin.daemon.Accounts", path, "com.deepin.daemon.Accounts.User",
                                      QDBusConnection::systemBus(), this);
        }

    bool format = m_interface->property(TIME_FORMAT_KEY).toBool();

    m_interface->deleteLater();
    return format;
}

void DatetimePlugin::save24HourFormat(bool format)
{
    if (QDBusConnection::sessionBus().interface()->isServiceRegistered("com.deepin.daemon.Timedate")) {
        QDBusInterface *m_interface = new QDBusInterface("com.deepin.daemon.Timedate", "/com/deepin/daemon/Timedate", "com.deepin.daemon.Timedate");
        m_interface->setProperty(TIME_FORMAT_KEY, format);
        m_interface->deleteLater();
    } 
}
