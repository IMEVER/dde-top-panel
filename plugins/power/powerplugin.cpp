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

#include "powerplugin.h"

#include <DDBusSender>

#define PLUGIN_STATE_KEY    "enable"
#define DELAYTIME           (20 * 1000)

using namespace Dock;

PowerPlugin::PowerPlugin(QObject *parent)
    : QObject(parent)
    , m_pluginLoaded(false)
    , m_tipsLabel(new TipsWidget)
    , m_preChargeTimer(new QTimer(this))
{
    m_tipsLabel->setObjectName("power");
    m_preChargeTimer->setInterval(DELAYTIME);
    m_preChargeTimer->setSingleShot(true);
    connect(m_preChargeTimer,&QTimer::timeout,this, [&] {
        if(m_tipsLabel->isVisible())
            refreshTipsData();
    });
}

const QString PowerPlugin::pluginName() const
{
    return "power";
}

const QString PowerPlugin::pluginDisplayName() const
{
    return "电源";
}

QWidget *PowerPlugin::itemWidget(const QString &itemKey)
{
    if (itemKey == POWER_KEY)
        return m_powerStatusWidget;

    return nullptr;
}

QWidget *PowerPlugin::itemTipsWidget(const QString &itemKey)
{
    const BatteryPercentageMap data = m_powerInter->batteryPercentage();

    if (data.isEmpty()) {
        return nullptr;
    }

    refreshTipsData();

    return m_tipsLabel;
}

void PowerPlugin::init(PluginProxyInterface *proxyInter)
{
    m_proxyInter = proxyInter;

    if (!pluginIsDisable()) {
        loadPlugin();
    }
}

void PowerPlugin::pluginStateSwitched()
{
    m_proxyInter->saveValue(this, PLUGIN_STATE_KEY, pluginIsDisable());

    refreshPluginItemsVisible();
}

bool PowerPlugin::pluginIsDisable()
{
    return !m_proxyInter->getValue(this, PLUGIN_STATE_KEY, true).toBool();
}

const QString PowerPlugin::itemCommand(const QString &itemKey)
{
    if (itemKey == POWER_KEY)
        return QString("dbus-send --print-reply --dest=com.deepin.dde.ControlCenter /com/deepin/dde/ControlCenter com.deepin.dde.ControlCenter.ShowModule string:power");

    return QString();
}

const QString PowerPlugin::itemContextMenu(const QString &itemKey)
{
    if (itemKey != POWER_KEY) {
        return QString();
    }

    QList<QVariant> items;
    items.reserve(1);

        QMap<QString, QVariant> power;
        power["itemId"] = "power";
        power["itemText"] = "电源设置";
        power["isActive"] = true;
        items.push_back(power);

    QMap<QString, QVariant> menu;
    menu["items"] = items;
    menu["checkableMenu"] = false;
    menu["singleCheck"] = false;

    return QJsonDocument::fromVariant(menu).toJson();
}

void PowerPlugin::invokedMenuItem(const QString &itemKey, const QString &menuId, const bool checked)
{
    Q_UNUSED(itemKey)
    Q_UNUSED(checked)

    if (menuId == "power") {
        DDBusSender()
        .service("com.deepin.dde.ControlCenter")
        .interface("com.deepin.dde.ControlCenter")
        .path("/com/deepin/dde/ControlCenter")
        .method(QString("ShowModule"))
        .arg(QString("power"))
        .call();
     }
}

void PowerPlugin::refreshIcon(const QString &itemKey)
{
    if (itemKey == POWER_KEY) {
        m_powerStatusWidget->refreshIcon();
    }
}

int PowerPlugin::itemSortKey(const QString &itemKey)
{
    const QString key = QString("pos_%1").arg(itemKey);

    return m_proxyInter->getValue(this, key, 5).toInt();
}

void PowerPlugin::setSortKey(const QString &itemKey, const int order)
{
    const QString key = QString("pos_%1").arg(itemKey);

    m_proxyInter->saveValue(this, key, order);
}

void PowerPlugin::pluginSettingsChanged()
{
    refreshPluginItemsVisible();
}

void PowerPlugin::updateBatteryVisible()
{
    const bool exist = !m_powerInter->batteryPercentage().isEmpty();

    if (!exist)
        m_proxyInter->itemRemoved(this, POWER_KEY);
    else if (exist && !pluginIsDisable())
        m_proxyInter->itemAdded(this, POWER_KEY);
}

void PowerPlugin::loadPlugin()
{
    if (m_pluginLoaded) {
        qDebug() << "power plugin has been loaded! return";
        return;
    }

    m_pluginLoaded = true;

    m_powerStatusWidget = new PowerStatusWidget;
    m_powerInter = new DBusPower(this);

    m_systemPowerInter = new SystemPowerInter("com.deepin.system.Power", "/com/deepin/system/Power", QDBusConnection::systemBus(), this);
    m_systemPowerInter->setSync(true);

    connect(m_systemPowerInter, &SystemPowerInter::BatteryStatusChanged, [&](uint  value) {
        if(m_tipsLabel->isVisible())
        {
            if (value == BatteryState::CHARGING)
                m_preChargeTimer->start();
            refreshTipsData();
        }
    });
    connect(m_systemPowerInter, &SystemPowerInter::BatteryTimeToEmptyChanged, [&]{
        if(m_tipsLabel->isVisible())
            refreshTipsData();
    });
    connect(m_systemPowerInter, &SystemPowerInter::BatteryTimeToFullChanged, [&] {
        if(m_tipsLabel->isVisible())
            refreshTipsData();
    });

    connect(m_powerInter, &DBusPower::BatteryPercentageChanged, this, &PowerPlugin::updateBatteryVisible);

    updateBatteryVisible();
}

void PowerPlugin::refreshPluginItemsVisible()
{
    if (pluginIsDisable()) {
        m_proxyInter->itemRemoved(this, POWER_KEY);
    } else {
        if (!m_pluginLoaded) {
            loadPlugin();
            return;
        }
        updateBatteryVisible();
    }
}

void PowerPlugin::refreshTipsData()
{
    const BatteryPercentageMap data = m_powerInter->batteryPercentage();
    const uint percentage = qMin(100.0, qMax(0.0, data.value("Display")));
    const QString value = QString("%1%").arg(std::round(percentage));
    const int batteryState = m_powerInter->batteryState()["Display"];

    QString tips;
    if (m_preChargeTimer->isActive()) {
        // 插入电源后，20秒内算作预充电时间，此时计算剩余充电时间是不准确的
        tips = QString("电量 %1 ...").arg(value);
    } else if (batteryState == BatteryState::DIS_CHARGING || batteryState == BatteryState::NOT_CHARGED || batteryState == BatteryState::UNKNOWN) {
        qulonglong timeToEmpty = m_systemPowerInter->batteryTimeToEmpty();
        QDateTime time = QDateTime::fromTime_t(timeToEmpty).toUTC();
        uint hour = time.toString("hh").toUInt();
        uint min = time.toString("mm").toUInt();
        uint sencond = time.toString("ss").toInt();
        if (sencond > 0)
            min += 1;
        if (hour == 0) {
            tips = min == 0 ? "已充电" : QString("电量 %1, 还可使用 %2 分钟").arg(value).arg(min);
        } else {
            tips = QString("电量 %1, 还可使用 %2 小时 %3 分钟").arg(value).arg(hour).arg(min);
        }
    } else if (batteryState == BatteryState::FULLY_CHARGED || percentage == 100.) {
        tips = QString("电量 100%, 已充满");
    } else {
        qulonglong timeToFull = m_systemPowerInter->batteryTimeToFull();
        QDateTime time = QDateTime::fromTime_t(timeToFull).toUTC();
        uint hour = time.toString("hh").toUInt();
        uint min = time.toString("mm").toUInt();
        if (timeToFull == 0) {  // 电量已充満或电量计算中,剩余充满时间会返回0
            tips = QString("电量 %1 ...").arg(value);
        } else {
            tips = hour == 0 ? QString("充电 %1, 还剩 %2 分钟充满").arg(value).arg(min)
                        : QString("充电 %1, 还剩 %2 小时 %3 分钟充满").arg(value).arg(hour).arg(min);
        }
    }
    m_tipsLabel->setText(tips);
}
