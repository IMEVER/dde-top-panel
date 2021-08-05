/*
 * Copyright (C) 2016 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     chenwei <chenwei@uniontech.com>
 *
 * Maintainer: chenwei <chenwei@uniontech.com>
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

#include "bluetoothapplet.h"
#include "device.h"
#include "bluetoothconstants.h"
#include "adaptersmanager.h"
#include "adapter.h"
#include "bluetoothadapteritem.h"

#include <QString>
#include <QBoxLayout>
#include <QMouseEvent>
#include <QDebug>
#include <QScrollArea>

#include <DApplicationHelper>
#include <DDBusSender>
#include <DLabel>
#include <DSwitchButton>
#include <DScrollArea>
#include <DListView>

SettingLabel::SettingLabel(QString text, QWidget *parent)
    : QWidget(parent)
    , m_label(new DLabel(text, this))
    , m_layout(new QHBoxLayout(this))
{
    setContentsMargins(0, 0, 0, 0);
    m_layout->setMargin(0);
    m_layout->addSpacing(20);
    m_layout->addWidget(m_label, 0, Qt::AlignLeft | Qt::AlignHCenter);
    m_layout->addStretch();
}

void SettingLabel::addButton(QWidget *button, int space)
{
    m_layout->addWidget(button, 0, Qt::AlignRight | Qt::AlignHCenter);
    m_layout->addSpacing(space);
}

void SettingLabel::mousePressEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton) {
        Q_EMIT clicked();
        return;
    }

    return QWidget::mousePressEvent(ev);
}

void SettingLabel::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setPen(Qt::NoPen);
    if (DApplicationHelper::instance()->themeType() == DApplicationHelper::LightType) {
        painter.setBrush(QColor(0, 0, 0, 0.03 * 255));
    } else {
        painter.setBrush(QColor(255, 255, 255, 0.03 * 255));
    }
    painter.drawRoundedRect(rect(), 0, 0);

    return QWidget::paintEvent(event);
}

BluetoothApplet::BluetoothApplet(QWidget *parent)
    : QWidget(parent)
    , m_contentWidget(new QWidget(this))
    , m_adaptersManager(new AdaptersManager(this))
    , m_settingLabel(new SettingLabel("蓝牙设置", this))
    , m_mainLayout(new QVBoxLayout(this))
    , m_contentLayout(new QVBoxLayout(m_contentWidget))
{
    initUi();
    initConnect();
}

bool BluetoothApplet::poweredInitState()
{
    foreach (const auto adapter, m_adapterItems) {
        if (adapter->adapter()->powered()) {
            return true;
        }
    }

    return false;
}

bool BluetoothApplet::hasAadapter()
{
    return m_adaptersManager->adaptersCount();
}

void BluetoothApplet::setAdapterRefresh()
{
    for (BluetoothAdapterItem *adapterItem : m_adapterItems) {
        if (adapterItem->adapter()->discover())
            m_adaptersManager->adapterRefresh(adapterItem->adapter());
    }
    updateSize();
}

void BluetoothApplet::setAdapterPowered(bool state)
{
    for (BluetoothAdapterItem *adapterItem : m_adapterItems) {
        if (adapterItem)
            m_adaptersManager->setAdapterPowered(adapterItem->adapter(), state);
    }
}

QStringList BluetoothApplet::connectedDevicesName()
{
    QStringList deviceList;
    for (BluetoothAdapterItem *adapterItem : m_adapterItems) {
        if (adapterItem)
            deviceList << adapterItem->connectedDevicesName();
    }

    return deviceList;
}

void BluetoothApplet::onAdapterAdded(Adapter *adapter)
{
    if (m_adapterItems.contains(adapter->id())) {
        onAdapterRemoved(m_adapterItems.value(adapter->id())->adapter());
    }
    if (!m_adapterItems.size()) {
        emit justHasAdapter();
    }

    BluetoothAdapterItem *adapterItem = new BluetoothAdapterItem(adapter, this);
    connect(adapterItem, &BluetoothAdapterItem::requestSetAdapterPower, this, &BluetoothApplet::onSetAdapterPower);
    connect(adapterItem, &BluetoothAdapterItem::connectDevice, m_adaptersManager, &AdaptersManager::connectDevice);
    connect(adapterItem, &BluetoothAdapterItem::deviceCountChanged, this, &BluetoothApplet::updateSize);
    connect(adapterItem, &BluetoothAdapterItem::adapterPowerChanged, this, &BluetoothApplet::updateBluetoothPowerState);
    connect(adapterItem, &BluetoothAdapterItem::deviceStateChanged, this, &BluetoothApplet::deviceStateChanged);
    connect(adapterItem, &BluetoothAdapterItem::requestRefreshAdapter, m_adaptersManager, &AdaptersManager::adapterRefresh);

    m_adapterItems.insert(adapter->id(), adapterItem);
    m_contentLayout->insertWidget(0, adapterItem, Qt::AlignTop | Qt::AlignVCenter);
    updateBluetoothPowerState();
}

void BluetoothApplet::onAdapterRemoved(Adapter *adapter)
{
    m_contentLayout->removeWidget(m_adapterItems.value(adapter->id()));
    m_adapterItems.value(adapter->id())->deleteLater();
    m_adapterItems.remove(adapter->id());
    if (m_adapterItems.isEmpty()) {
        emit noAdapter();
    }
    updateBluetoothPowerState();
}

void BluetoothApplet::onSetAdapterPower(Adapter *adapter, bool state)
{
    m_adaptersManager->setAdapterPowered(adapter, state);
    updateSize();
}

void BluetoothApplet::updateBluetoothPowerState()
{
    emit powerChanged(this->poweredInitState());
    updateSize();
}

void BluetoothApplet::initUi()
{
    setFixedWidth(ItemWidth);
    setContentsMargins(0, 0, 0, 0);

    m_settingLabel->setFixedHeight(DeviceItemHeight);
    DFontSizeManager::instance()->bind(m_settingLabel->label(), DFontSizeManager::T7);

    m_contentWidget->setContentsMargins(0, 0, 0, 0);
    m_contentLayout->setMargin(0);
    m_contentLayout->setSpacing(0);
    m_contentLayout->addWidget(m_settingLabel, 0, Qt::AlignBottom | Qt::AlignVCenter);

    QScrollArea *scroarea = new QScrollArea(this);
    m_contentWidget->setAttribute(Qt::WA_TranslucentBackground);

    scroarea->setWidgetResizable(true);
    scroarea->setFrameStyle(QFrame::NoFrame);
    scroarea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroarea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroarea->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
    scroarea->setContentsMargins(0, 0, 0, 0);
    scroarea->setWidget(m_contentWidget);

    m_mainLayout->setMargin(0);
    m_mainLayout->setSpacing(0);
    m_mainLayout->addWidget(scroarea);
    updateSize();
}

void BluetoothApplet::initConnect()
{
    connect(m_adaptersManager, &AdaptersManager::adapterIncreased, this, &BluetoothApplet::onAdapterAdded);
    connect(m_adaptersManager, &AdaptersManager::adapterDecreased, this, &BluetoothApplet::onAdapterRemoved);

    connect(m_settingLabel, &SettingLabel::clicked, this, [ = ] {
        DDBusSender()
        .service("com.deepin.dde.ControlCenter")
        .interface("com.deepin.dde.ControlCenter")
        .path("/com/deepin/dde/ControlCenter")
        .method(QString("ShowModule"))
        .arg(QString("bluetooth"))
        .call();
    });
}

void BluetoothApplet::updateSize()
{
    int hetght = 0;
    int count = 0;
    foreach (const auto item, m_adapterItems) {
        hetght += TitleHeight + TitleSpace;
        if (item->adapter()->powered()) {
            count += item->currentDeviceCount();
            hetght += count * DeviceItemHeight;
        }
    }

    hetght += DeviceItemHeight;
    int maxHeight = (TitleHeight + TitleSpace) + MaxDeviceCount * DeviceItemHeight;
    hetght = hetght > maxHeight ? maxHeight : hetght;
    setFixedSize(ItemWidth, hetght);
}

