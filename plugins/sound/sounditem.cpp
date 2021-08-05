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

#include <QPainter>
#include <QIcon>
#include <QMouseEvent>
#include <QApplication>

#include <DApplication>
#include <DDBusSender>
#include <DGuiApplicationHelper>

DWIDGET_USE_NAMESPACE
DGUI_USE_NAMESPACE

#include "sounditem.h"
#include "constants.h"
#include "../frame/util/imageutil.h"

// menu actions
#define MUTE     "mute"
#define SETTINGS "settings"


using namespace Dock;
SoundItem::SoundItem(QWidget *parent)
    : QWidget(parent)
    , m_tipsLabel(new TipsWidget(this))
    , m_applet(new SoundApplet(this))
    , m_sinkInter(nullptr)
{
    m_tipsLabel->setVisible(false);

    m_applet->setVisible(false);

    connect(m_applet, static_cast<void (SoundApplet::*)(DBusSink *) const>(&SoundApplet::defaultSinkChanged), this, &SoundItem::sinkChanged);
    connect(m_applet, &SoundApplet::volumeChanged, this, &SoundItem::refresh, Qt::QueuedConnection);

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, [ = ] {
        refreshIcon();
    });
}

QWidget *SoundItem::tipsWidget()
{
    refreshTips(m_applet->volumeValue(), true);

    // m_tipsLabel->resize(m_tipsLabel->sizeHint().width() + 10, m_tipsLabel->sizeHint().height());
    return m_tipsLabel;
}

QWidget *SoundItem::popupApplet()
{
    return m_applet;
}

const QString SoundItem::contextMenu() const
{
    QList<QVariant> items;
    items.reserve(2);

    QMap<QString, QVariant> open;
    open["itemId"] = MUTE;
    if (m_sinkInter->mute())
        open["itemText"] = "取消静音";
    else
        open["itemText"] = "静音";
    open["isActive"] = true;
    items.push_back(open);

        QMap<QString, QVariant> settings;
        settings["itemId"] = SETTINGS;
        settings["itemText"] = "声音设置";
        settings["isActive"] = true;
        items.push_back(settings);

    QMap<QString, QVariant> menu;
    menu["items"] = items;
    menu["checkableMenu"] = false;
    menu["singleCheck"] = false;

    return QJsonDocument::fromVariant(menu).toJson();
}

void SoundItem::invokeMenuItem(const QString menuId, const bool checked)
{
    Q_UNUSED(checked);

    if (menuId == MUTE)
        m_sinkInter->SetMuteQueued(!m_sinkInter->mute());
    else if (menuId == SETTINGS)
        DDBusSender()
        .service("com.deepin.dde.ControlCenter")
        .interface("com.deepin.dde.ControlCenter")
        .path("/com/deepin/dde/ControlCenter")
        .method(QString("ShowModule"))
        .arg(QString("sound"))
        .call();
}

void SoundItem::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);

    setMaximumWidth(height());
    setMaximumHeight(QWIDGETSIZE_MAX);
    refreshIcon();
}

void SoundItem::wheelEvent(QWheelEvent *e)
{
    QWheelEvent *event = new QWheelEvent(e->position(), e->globalPosition(), e->pixelDelta(), e->angleDelta(), e->buttons(), e->modifiers(), e->phase(), e->inverted());
    qApp->postEvent(m_applet->mainSlider(), event);

    e->accept();
}

void SoundItem::paintEvent(QPaintEvent *e)
{
    QWidget::paintEvent(e);

    QPainter painter(this);
    const QRectF &rf = QRectF(rect());
    const QRectF &rfp = QRectF(m_iconPixmap.rect());
    painter.drawPixmap(rf.center() - rfp.center() / m_iconPixmap.devicePixelRatioF(), m_iconPixmap);
}

void SoundItem::refresh(const int volume)
{
    refreshIcon();
    refreshTips(volume, false);
}

void SoundItem::refreshIcon()
{
    if (!m_sinkInter)
        return;

    const double volmue = m_applet->volumeValue();
    const double maxVolmue = m_applet->maxVolumeValue();
    const bool mute = m_sinkInter->mute();

    QString iconString;
        QString volumeString;
        if (mute)
            volumeString = "muted";
        else if (int(volmue) == 0)
            volumeString = "off";
        else if (volmue / maxVolmue > double(2) / 3)
            volumeString = "high";
        else if (volmue / maxVolmue > double(1) / 3)
            volumeString = "medium";
        else
            volumeString = "low";

        iconString = QString("audio-volume-%1-symbolic").arg(volumeString);

    const auto ratio = devicePixelRatioF();
    int iconSize = PLUGIN_ICON_MAX_SIZE;
    if (height() <= PLUGIN_BACKGROUND_MIN_SIZE && DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType)
        iconString.append(PLUGIN_MIN_ICON_NAME);

    m_iconPixmap = ImageUtil::loadSvg(iconString, ":/", iconSize, ratio);

    update();
}

void SoundItem::refreshTips(const int volume, const bool force)
{
    if (!force && !m_tipsLabel->isVisible())
        return;

    if (m_sinkInter->mute()) {
        m_tipsLabel->setText(QString("静音"));
    } else {
        m_tipsLabel->setText(QString("音量 %1").arg(QString::number(volume) + '%'));
    }
}

void SoundItem::sinkChanged(DBusSink *sink)
{
    m_sinkInter = sink;
    refresh(m_applet->volumeValue());
}
