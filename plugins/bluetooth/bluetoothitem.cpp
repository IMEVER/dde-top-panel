/*
 * Copyright (C) 2016 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     zhaolong <zhaolong@uniontech.com>
 *
 * Maintainer: zhaolong <zhaolong@uniontech.com>
 *
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

#include "bluetoothitem.h"
#include "constants.h"
#include "../../widgets/tipswidget.h"
#include "../frame/util/imageutil.h"
#include "componments/bluetoothapplet.h"

#include <DApplication>
#include <DDBusSender>
#include <DGuiApplicationHelper>

#include <QPainter>

// menu actions
#define SHIFT       "shift"
#define SETTINGS    "settings"

DWIDGET_USE_NAMESPACE
DGUI_USE_NAMESPACE

using namespace Dock;

BluetoothItem::BluetoothItem(QWidget *parent)
    : QWidget(parent)
    , m_tipsLabel(new TipsWidget(this))
    , m_applet(new BluetoothApplet(this))
    , m_devState(Device::State::StateUnavailable)
    , m_adapterPowered(m_applet->poweredInitState())
{
    m_applet->setVisible(false);
    m_tipsLabel->setVisible(false);
    refreshIcon();

    connect(m_applet, &BluetoothApplet::powerChanged, [ & ](bool powered) {
        m_adapterPowered = powered;
        refreshIcon();
    });
    connect(m_applet, &BluetoothApplet::deviceStateChanged, [ & ](const Device *device) {
        m_devState = device->state();
        refreshIcon();
        refreshTips();
    });
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &BluetoothItem::refreshIcon);
    connect(m_applet, &BluetoothApplet::noAdapter, this, &BluetoothItem::noAdapter);
    connect(m_applet, &BluetoothApplet::justHasAdapter, this, &BluetoothItem::justHasAdapter);
}

QWidget *BluetoothItem::tipsWidget()
{
    refreshTips();
    return m_tipsLabel;
}

QWidget *BluetoothItem::popupApplet()
{
    if (m_applet && m_applet->hasAadapter())
        m_applet->setAdapterRefresh();
    return m_applet->hasAadapter() ? m_applet : nullptr;
}

const QString BluetoothItem::contextMenu() const
{
    if (m_applet->hasAadapter()) {
        QList<QVariant> items;
        items.reserve(2);

        QMap<QString, QVariant> shift;
        shift["itemId"] = SHIFT;
        if (m_adapterPowered)
            shift["itemText"] = "关闭";
        else
            shift["itemText"] = "开启";
        shift["isActive"] = true;
        items.push_back(shift);

        QMap<QString, QVariant> settings;
        settings["itemId"] = SETTINGS;
        settings["itemText"] = "蓝牙设置";
        settings["isActive"] = true;
        items.push_back(settings);

        QMap<QString, QVariant> menu;
        menu["items"] = items;
        menu["checkableMenu"] = false;
        menu["singleCheck"] = false;
        return QJsonDocument::fromVariant(menu).toJson();
    }
    return QByteArray();
}

void BluetoothItem::invokeMenuItem(const QString menuId, const bool checked)
{
    Q_UNUSED(checked);

    if (menuId == SHIFT) {
        m_applet->setAdapterPowered(!m_adapterPowered);
    } else if (menuId == SETTINGS) {
        DDBusSender()
        .service("com.deepin.dde.ControlCenter")
        .interface("com.deepin.dde.ControlCenter")
        .path("/com/deepin/dde/ControlCenter")
        .method(QString("ShowModule"))
        .arg(QString("bluetooth"))
        .call();
    }
}

void BluetoothItem::refreshIcon()
{
    QString stateString;
    QString iconString;

    if (m_adapterPowered) {
        if (m_applet->connectedDevicesName().isEmpty()) {
            stateString = "disable";
        } else {
            stateString = "active";
        }
    } else {
        stateString = "disable";
    }

    iconString = QString("bluetooth-%1-symbolic").arg(stateString);

    const qreal ratio = devicePixelRatioF();
    int iconSize = PLUGIN_ICON_MAX_SIZE;
    if (height() <= PLUGIN_BACKGROUND_MIN_SIZE && DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType)
        iconString.append(PLUGIN_MIN_ICON_NAME);

    m_iconPixmap = ImageUtil::loadSvg(iconString, ":/", iconSize, ratio);

    update();
}

void BluetoothItem::refreshTips()
{
    QString tipsText;

    if (m_adapterPowered) {
        if (!m_applet->connectedDevicesName().isEmpty() && m_devState != Device::StateAvailable) {
            QStringList textList;
            for (QString devName : m_applet->connectedDevicesName()) {
                textList << QString("%1 已连接").arg(devName);
            }
            m_tipsLabel->setTextList(textList);
            return;
        }
        if (m_devState == Device::StateAvailable) {
            tipsText = "连接中...";
        } else {
            tipsText = "蓝牙";
        }
    } else {
        tipsText = "已关闭";
    }

    m_tipsLabel->setText(tipsText);
}


bool BluetoothItem::hasAdapter()
{
    return m_applet->hasAadapter();
}

void BluetoothItem::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    setMaximumWidth(height());
    setMaximumHeight(QWIDGETSIZE_MAX);
    refreshIcon();
}

void BluetoothItem::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    const QRectF &rf = QRectF(rect());
    const QRectF &rfp = QRectF(m_iconPixmap.rect());
    painter.drawPixmap(rf.center() - rfp.center() / m_iconPixmap.devicePixelRatioF(), m_iconPixmap);
}
