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

#include "soundapplet.h"
#include "sinkinputwidget.h"
#include "componments/horizontalseparator.h"
#include "../frame/util/imageutil.h"

#include <DGuiApplicationHelper>
#include <DApplication>
#include <DStandardItem>
#include <DFontSizeManager>
#include <QGSettings>

#include <QLabel>
#include <QIcon>
#include <QScrollBar>

#define WIDTH       260
#define MAX_HEIGHT  300
#define ICON_SIZE   24
#define ITEM_HEIGHT 24
#define ITEM_SPACING 5
#define DEVICE_SPACING 10
#define SLIDER_HIGHT 32
#define GSETTING_SOUND_OUTPUT_SLIDER "soundOutputSlider"

DWIDGET_USE_NAMESPACE
DGUI_USE_NAMESPACE

Q_DECLARE_METATYPE(const Port *)

Port::Port(QObject *parent)
    : QObject(parent)
    , m_isActive(false)
    , m_direction(Out)
{

}

void Port::setId(const QString &id)
{
    if (id != m_id) {
        m_id = id;
        Q_EMIT idChanged(id);
    }
}

void Port::setName(const QString &name)
{
    if (name != m_name) {
        m_name = name;
        Q_EMIT nameChanged(name);
    }
}

void Port::setCardName(const QString &cardName)
{
    if (cardName != m_cardName) {
        m_cardName = cardName;
        Q_EMIT cardNameChanged(cardName);
    }
}

void Port::setIsActive(bool isActive)
{
    if (isActive != m_isActive) {
        m_isActive = isActive;
        Q_EMIT isActiveChanged(isActive);
    }
}

void Port::setDirection(const Direction &direction)
{
    if (direction != m_direction) {
        m_direction = direction;
        Q_EMIT directionChanged(direction);
    }
}

void Port::setCardId(const uint &cardId)
{
    if (cardId != m_cardId) {
        m_cardId = cardId;
        Q_EMIT cardIdChanged(cardId);
    }
}

SoundApplet::SoundApplet(QWidget *parent)
    : QScrollArea(parent)
    , m_centralWidget(new QWidget)
    , m_volumeBtn(new DIconButton(this))
    , m_volumeIconMax(new QLabel)
    , m_volumeSlider(new VolumeSlider)
    , m_soundShow(new TipsWidget)
    , m_separator(new HorizontalSeparator)
    , m_deviceLabel(nullptr)
    , m_audioInter(new DBusAudio("com.deepin.daemon.Audio", "/com/deepin/daemon/Audio", QDBusConnection::sessionBus(), this))
    , m_defSinkInter(nullptr)
    , m_listView(new DListView(this))
    , m_model(new QStandardItemModel(m_listView))
    , m_deviceInfo("")
    , m_lastPort(nullptr)
    , m_gsettings(new QGSettings("com.deepin.dde.dock.module.sound", QByteArray(), this))

{
    initUi();
}

void SoundApplet::initUi()
{
    m_listView->setEditTriggers(DListView::NoEditTriggers);
    m_listView->setSelectionMode(QAbstractItemView::NoSelection);
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listView->setBackgroundType(DStyledItemDelegate::NoBackground);
    m_listView->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    m_listView->setViewportMargins(QMargins(0, 0, 12, 0));
    m_listView->setSpacing(ITEM_SPACING);
    m_listView->setFixedHeight(0);

    m_volumeIconMax->setFixedSize(ICON_SIZE, ICON_SIZE);

    m_soundShow->setText(QString("%1%").arg(0));

    m_deviceLabel = new TipsWidget;
    m_deviceLabel->setText("设备");
    DFontSizeManager::instance()->bind(m_deviceLabel, DFontSizeManager::T4, QFont::Medium);

    QHBoxLayout *deviceLayout = new QHBoxLayout;
    deviceLayout->addSpacing(2);
    deviceLayout->addWidget(m_deviceLabel, 0, Qt::AlignLeft);
    deviceLayout->addWidget(m_soundShow, 0, Qt::AlignRight);
    deviceLayout->setSpacing(0);
    deviceLayout->setMargin(0);

    QVBoxLayout *deviceLineLayout = new QVBoxLayout;
    deviceLineLayout->addLayout(deviceLayout);
    deviceLineLayout->addWidget(m_separator);
    deviceLineLayout->setMargin(0);
    deviceLineLayout->setSpacing(DEVICE_SPACING);

    QHBoxLayout *volumeCtrlLayout = new QHBoxLayout;
    volumeCtrlLayout->addSpacing(2);
    volumeCtrlLayout->addWidget(m_volumeBtn);
    volumeCtrlLayout->addSpacing(10);
    volumeCtrlLayout->addWidget(m_volumeSlider);
    volumeCtrlLayout->addSpacing(10);
    volumeCtrlLayout->addWidget(m_volumeIconMax);
    volumeCtrlLayout->setSpacing(0);
    volumeCtrlLayout->setMargin(0);

    m_volumeBtn->setFixedSize(ICON_SIZE, ICON_SIZE);
    m_volumeBtn->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
    m_volumeBtn->setFlat(true);

    m_volumeSlider->setFixedHeight(SLIDER_HIGHT);
    m_volumeSlider->setMinimum(0);
    m_volumeSlider->setMaximum(m_audioInter->maxUIVolume() * 100.0f);
    updateVolumeSliderStatus("Enabled");
    connect(m_gsettings, &QGSettings::changed, [ = ] (const QString &key) {
        if (key == GSETTING_SOUND_OUTPUT_SLIDER) {
            updateVolumeSliderStatus(m_gsettings->get(GSETTING_SOUND_OUTPUT_SLIDER).toString());
        }
    });

    m_centralLayout = new QVBoxLayout;
    m_centralLayout->addLayout(deviceLineLayout);
    m_centralLayout->addSpacing(8);
    m_centralLayout->addLayout(volumeCtrlLayout);
    m_centralLayout->setContentsMargins(8, 8, 8, 0);

    m_listView->setModel(m_model);
    m_centralLayout->addWidget(m_listView);
    m_centralWidget->setLayout(m_centralLayout);
    m_centralWidget->setFixedWidth(WIDTH);
    m_centralWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    setFixedWidth(WIDTH);
    setWidget(m_centralWidget);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_centralWidget->setAutoFillBackground(false);
    viewport()->setAutoFillBackground(false);

    connect(qApp, &QGuiApplication::fontChanged, this, &SoundApplet::updateListHeight);
    connect(m_volumeBtn, &DIconButton::clicked, this, &SoundApplet::toggleMute);
    connect(m_volumeSlider, &VolumeSlider::valueChanged, this, &SoundApplet::volumeSliderValueChanged);
    connect(m_volumeSlider, &VolumeSlider::requestPlaySoundEffect, this, &SoundApplet::onPlaySoundEffect);
    connect(m_audioInter, &DBusAudio::DefaultSinkChanged, this, static_cast<void (SoundApplet::*)()>(&SoundApplet::defaultSinkChanged));
    connect(m_audioInter, &DBusAudio::IncreaseVolumeChanged, this, &SoundApplet::increaseVolumeChanged);
    connect(m_audioInter, &DBusAudio::PortEnabledChanged, [this](uint cardId, QString portId) {
            portEnableChange(cardId, portId);
    });;
    connect(m_listView, &DListView::clicked, this, [this](const QModelIndex & idx) {
        const Port * port = m_listView->model()->data(idx, Qt::WhatsThisPropertyRole).value<const Port *>();
        if (port) {
            m_audioInter->SetPort(port->cardId(), port->id(), int(port->direction()));
            //手动勾选时启用设备
            m_audioInter->SetPortEnabled(port->cardId(), port->id(), true);
        }

    });
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &SoundApplet::refreshIcon);
    connect(qApp, &DApplication::iconThemeChanged, this, &SoundApplet::refreshIcon);
    QDBusConnection::sessionBus().connect("com.deepin.daemon.Audio", "/com/deepin/daemon/Audio", "org.freedesktop.DBus.Properties"
                                          ,"PropertiesChanged", "sa{sv}as", this, SLOT(haldleDbusSignal(QDBusMessage)));

    QMetaObject::invokeMethod(this, "defaultSinkChanged", Qt::QueuedConnection);

    refreshIcon();

    updateCradsInfo();
}

int SoundApplet::volumeValue() const
{
    return m_volumeSlider->value();
}

int SoundApplet::maxVolumeValue() const
{
    return m_volumeSlider->maximum();
}

VolumeSlider *SoundApplet::mainSlider()
{
    return m_volumeSlider;
}

void SoundApplet::defaultSinkChanged()
{
    //防止手动切换设备，与后端交互时，获取到多个信号，设备切换多次，造成混乱
    QThread::msleep(200);

    if (m_defSinkInter) {
        delete m_defSinkInter;
        m_defSinkInter = nullptr;
    }

    const QDBusObjectPath defSinkPath = m_audioInter->defaultSink();
    m_defSinkInter = new DBusSink("com.deepin.daemon.Audio", defSinkPath.path(), QDBusConnection::sessionBus(), this);

    connect(m_defSinkInter, &DBusSink::VolumeChanged, this, &SoundApplet::onVolumeChanged);
    connect(m_defSinkInter, &DBusSink::MuteChanged, this, [ = ] {
        onVolumeChanged(m_defSinkInter->volume());
    });

    QString portId = m_defSinkInter->activePort().name;
    uint cardId = m_defSinkInter->card();
    //最后一个设备会被移除，但是当在控制中心选中此设备后需要添加，并勾选
    if (!m_lastPort.isNull() && m_lastPort->cardId() == cardId && m_lastPort->id() == portId) {
        startAddPort(m_lastPort);
    }
    activePort(portId,cardId);

    emit defaultSinkChanged(m_defSinkInter);
    onVolumeChanged(m_defSinkInter->volume());
}

void SoundApplet::onVolumeChanged(double volume)
{
    m_volumeSlider->setValue(static_cast<int>(std::min(150.0, volume * 100.0)));

    m_soundShow->setText(QString::number(volume * 100) + '%');
    emit volumeChanged(m_volumeSlider->value());
    refreshIcon();
}

void SoundApplet::volumeSliderValueChanged()
{
    m_defSinkInter->SetVolume(m_volumeSlider->value() / 100.0f, true);
    if (m_defSinkInter->mute())
        m_defSinkInter->SetMuteQueued(false);
}

void SoundApplet::sinkInputsChanged()
{
    m_centralWidget->setVisible(false);
    QVBoxLayout *appLayout = m_centralLayout;
    while (QLayoutItem *item = appLayout->takeAt(4)) {
        delete item->widget();
        delete item;
    }

    for (auto input : m_audioInter->sinkInputs()) {
        appLayout->addWidget(new HorizontalSeparator);

        SinkInputWidget *si = new SinkInputWidget(input.path());
        appLayout->addWidget(si);
    }

    m_centralWidget->setVisible(true);
}

void SoundApplet::cardsChanged(const QString &cards)
{
    QMap<uint, QStringList> tmpCardIds;

    QJsonDocument doc = QJsonDocument::fromJson(cards.toUtf8());
    QJsonArray jCards = doc.array();
    for (QJsonValue cV : jCards) {
        QJsonObject jCard = cV.toObject();
        const uint cardId = jCard["Id"].toInt();
        const QString cardName = jCard["Name"].toString();
        QJsonArray jPorts = jCard["Ports"].toArray();

        QStringList tmpPorts;

        for (QJsonValue pV : jPorts) {
            QJsonObject jPort = pV.toObject();
            const double portAvai = jPort["Available"].toDouble();
            if (portAvai == 2 || portAvai == 0 ) { // 0 Unknow 1 Not available 2 Available
                const QString portId = jPort["Name"].toString();
                const QString portName = jPort["Description"].toString();

                Port *port = findPort(portId, cardId);
                const bool include = port != nullptr;
                if (!include) { port = new Port(m_model); }

                port->setId(portId);
                port->setName(portName);
                port->setDirection(Port::Direction(jPort["Direction"].toDouble()));
                port->setCardId(cardId);
                port->setCardName(cardName);

                if (!include) {
                    startAddPort(port);
                }

                tmpPorts << portId;
            }
        }
        tmpCardIds.insert(cardId, tmpPorts);
    }

    defaultSinkChanged();//重新获取切换的设备信息
    enableDevice(true);

    for (Port *port : m_ports) {
        //只要有一个设备在控制中心被禁用后，在任务栏声音设备列表中该设备会被移除，
        if (!m_audioInter->IsPortEnabled(port->cardId(), port->id())) {
            removeDisabledDevice(port->id(), port->cardId());
        }
        //判断端口是否在最新的设备列表中
        if (tmpCardIds.contains(port->cardId())) {
            if (!tmpCardIds[port->cardId()].contains(port->id())) {
                startRemovePort(port->id(), port->cardId());
            }
        }
        else {
            startRemovePort(port->id(), port->cardId());
        }
    }
    //当只有一个设备剩余时，该设备也需要移除
    removeLastDevice();
    updateListHeight();
}
void SoundApplet::toggleMute()
{
    m_defSinkInter->SetMuteQueued(!m_defSinkInter->mute());
}

void SoundApplet::onPlaySoundEffect()
{

}

void SoundApplet::increaseVolumeChanged()
{
    m_volumeSlider->setMaximum(m_audioInter->maxUIVolume() * 100.0f);
}

void SoundApplet::refreshIcon()
{
    if (!m_defSinkInter)
        return;

    const bool mute = m_defSinkInter->mute();

    QString volumeString;

    if (mute) {
        volumeString = "muted";
    } else {
        volumeString = "off";
    }

    QString iconLeft = QString("audio-volume-%1-symbolic").arg(volumeString);
    QString iconRight = QString("audio-volume-high-symbolic");

    QColor color;
    switch (DGuiApplicationHelper::instance()->themeType()) {
    case DGuiApplicationHelper::LightType:
        color = Qt::black;
        iconLeft.append("-dark");
        iconRight.append("-dark");
        break;
    default:
        color = Qt::white;
        break;
    }
    //主题改变时，同步修改item颜色
    for (int i = 0; i < m_model->rowCount(); i++) {
        auto item = m_model->item(i);
        item->setForeground(color);
    }
    const auto ratio = devicePixelRatioF();
    QPixmap ret = ImageUtil::loadSvg(iconRight, ":/", ICON_SIZE, ratio);
    m_volumeIconMax->setPixmap(ret);

    ret = ImageUtil::loadSvg(iconLeft, ":/", ICON_SIZE, ratio);
    m_volumeBtn->setIcon(ret);
}

/**
 * @brief SoundApplet::startAddPort 添加端口前判断
 * @param port 端口
 */
void SoundApplet::startAddPort(Port *port)
{
    if (!containsPort(port) && port->direction() == Port::Out) {
        m_ports.append(port);
        addPort(port);
    }
}

/**
 * @brief SoundApplet::startRemovePort 移除端口前判断
 * @param portId 端口
 * @param cardId 声卡
 */
void SoundApplet::startRemovePort(const QString &portId, const uint &cardId)
{
    Port *port = findPort(portId, cardId);
    if (port) {
        m_ports.removeOne(port);
        port->deleteLater();
        removePort(portId, cardId);
    }
}

bool SoundApplet::containsPort(const Port *port)
{
    return findPort(port->id(), port->cardId()) != nullptr;
}

Port *SoundApplet::findPort(const QString &portId, const uint &cardId) const
{
    for (Port *port : m_ports) {
        if (port->id() == portId && port->cardId() == cardId) {
            return port;
        }
    }

    return nullptr;
}

void SoundApplet::addPort(const Port *port)
{
    DStandardItem *pi = new DStandardItem;
    QString deviceName = port->name() + "(" + port->cardName() + ")";
    pi->setText(deviceName);
    pi->setBackground(Qt::transparent);
    pi->setForeground(QBrush(Qt::black));
    pi->setData(QVariant::fromValue<const Port *>(port), Qt::WhatsThisPropertyRole);

    connect(port, &Port::nameChanged, this, [ = ](const QString &str) {
        QString devName = str + "(" + port->cardName() + ")";
        pi->setText(devName);
    });
    connect(port, &Port::cardNameChanged, this, [ = ](const QString &str) {
        QString devName = port->name() + "(" + str + ")";
        pi->setText(devName);
    });
    connect(port, &Port::isActiveChanged, this, [ = ](bool isActive) {
        pi->setCheckState(isActive ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    });

    if (port->isActive()) {
        pi->setCheckState(Qt::CheckState::Checked);
    }
    m_model->appendRow(pi);
    m_model->sort(0);
}

void SoundApplet::removePort(const QString &portId, const uint &cardId)
{
    auto rmFunc = [ = ](QStandardItemModel * model) {
        for (int i = 0; i < model->rowCount();) {
            auto item = model->item(i);
            auto port = item->data(Qt::WhatsThisPropertyRole).value<const Port *>();
            if (port->id() == portId && cardId == port->cardId()) {
                model->removeRow(i);
                break;
            } else {
                ++i;
            }
        }
    };

    rmFunc(m_model);
}

/**
 * @brief SoundApplet::activePort 激活某一指定端口
 * @param portId 端口
 * @param cardId 声卡
 */
void SoundApplet::activePort(const QString &portId, const uint &cardId)
{
    for (Port *it : m_ports) {
        if (it->id() == portId && it->cardId() == cardId) {
            it->setIsActive(true);
            enableDevice(true);
        }
        else {
            it->setIsActive(false);
        }
    }
}

void SoundApplet::updateCradsInfo()
{
    QDBusInterface inter("com.deepin.daemon.Audio", "/com/deepin/daemon/Audio","com.deepin.daemon.Audio",QDBusConnection::sessionBus(), this);
    QString info = inter.property("CardsWithoutUnavailable").toString();
    if(m_deviceInfo != info){
        cardsChanged(info);
        m_deviceInfo = info;
    }
}

void SoundApplet::enableDevice(bool flag)
{
    m_volumeSlider->setEnabled(flag);
    m_volumeBtn->setEnabled(flag);
    m_soundShow->setEnabled(flag);
    m_volumeIconMax->setEnabled(flag);
    m_deviceLabel->setEnabled(flag);
}

void SoundApplet::disableAllDevice()
{
    for (Port *port : m_ports) {
        port->setIsActive(false);
    }
}

/**
 * @brief SoundApplet::removeLastDevice
 * 移除最后一个设备
 */
void SoundApplet::removeLastDevice()
{
    if (m_ports.count() == 1 && m_ports.at(0)) {
        m_lastPort = new Port(m_model);
        m_lastPort->setId(m_ports.at(0)->id());
        m_lastPort->setName(m_ports.at(0)->name());
        m_lastPort->setDirection(m_ports.at(0)->direction());
        m_lastPort->setCardId(m_ports.at(0)->cardId());
        m_lastPort->setCardName(m_ports.at(0)->cardName());
        startRemovePort(m_ports.at(0)->id(), m_ports.at(0)->cardId());
        qDebug() << "remove last output device";
    }
}

/**
 * @brief SoundApplet::removeDisabledDevice 移除禁用设备
 * @param portId
 * @param cardId
 */
void SoundApplet::removeDisabledDevice(QString portId, unsigned int cardId)
{
    startRemovePort(portId, cardId);
    if (m_defSinkInter->activePort().name == portId && m_defSinkInter->card() == cardId) {
        enableDevice(false);
        disableAllDevice();
    }
    qDebug() << "remove disabled output device";
}

void SoundApplet::updateVolumeSliderStatus(const QString &status)
{
    bool flag = true;
    if ("Enabled" == status) {
        flag = true;
    } else if ("Disabled" == status) {
        flag = false;
    }
    m_volumeSlider->setEnabled(flag);
    m_volumeBtn->setEnabled(flag);
    m_volumeIconMax->setEnabled(flag);

    flag = "Hiden" != status;
    m_volumeSlider->setVisible(flag);
    m_volumeBtn->setVisible(flag);
    m_volumeIconMax->setVisible(flag);
}

void SoundApplet::haldleDbusSignal(const QDBusMessage &msg)
{
    Q_UNUSED(msg)

    updateCradsInfo();
}

void SoundApplet::updateListHeight()
{
    //设备数多于10个时显示滚动条,固定高度
    int count = m_model->rowCount();
    if (m_model->rowCount() > 10) {
        count = 10;
        m_listView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    } else {
        m_listView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    int visualHeight = 0;
    for (int i = 0; i < count; i++)
        visualHeight += m_listView->visualRect(m_model->index(i, 0)).height();

    int listMargin = m_listView->contentsMargins().top() + m_listView->contentsMargins().bottom();
    //显示声音设备列表高度 = 设备的高度 + 间隔 + 边距
    int viewHeight = visualHeight + m_listView->spacing() * count * 2 + listMargin;
    // 设备信息高度 = 设备标签 + 分隔线 + 滚动条 + 间隔
    int labelHeight = m_deviceLabel->height() > m_soundShow->height() ? m_deviceLabel->height() : m_soundShow->height();
    int infoHeight = labelHeight + m_separator->height() + m_volumeSlider->height() + m_centralLayout->spacing() * 3 + DEVICE_SPACING;
    int margain = m_centralLayout->contentsMargins().top() + m_centralLayout->contentsMargins().bottom();
    //整个界面高度 = 显示声音设备列表高度 + 设备信息高度 + 边距
    int totalHeight = viewHeight + infoHeight + margain;
    m_listView->setFixedHeight(viewHeight);
    setFixedHeight(totalHeight);
    m_centralWidget->setFixedHeight(totalHeight);
    update();
}

void SoundApplet::portEnableChange(unsigned int cardId, QString portId)
{
    Q_UNUSED(cardId)
    Q_UNUSED(portId)
    m_deviceInfo = "";
    updateCradsInfo();
}
