/*
 * Copyright (C) 2011 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     sbw <sbw@sbw.so>
 *
 * Maintainer: sbw <sbw@sbw.so>
 *             listenerri <listenerri@gmail.com>
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

#include "appitem.h"

AppItem::AppItem(const QDBusObjectPath &entry, QWidget *parent)
    : QObject(parent),
      m_itemEntryInter(new DockEntryInter("com.deepin.dde.daemon.Dock", entry.path(), QDBusConnection::sessionBus(), this))
{
    m_id = m_itemEntryInter->id();

    connect(m_itemEntryInter, &DockEntryInter::IsActiveChanged, this, &AppItem::windowChanged);
    connect(m_itemEntryInter, &DockEntryInter::WindowInfosChanged, this, &AppItem::updateWindowInfos, Qt::QueuedConnection);
    connect(m_itemEntryInter, &DockEntryInter::CurrentWindowChanged, this, &AppItem::windowChanged);

    updateWindowInfos(m_itemEntryInter->windowInfos());
}

AppItem::~AppItem()
{
    disconnect(m_itemEntryInter, &DockEntryInter::IsActiveChanged, this, &AppItem::windowChanged);
    disconnect(m_itemEntryInter, &DockEntryInter::WindowInfosChanged, this, &AppItem::updateWindowInfos);
    disconnect(m_itemEntryInter, &DockEntryInter::CurrentWindowChanged, this, &AppItem::windowChanged);
    delete m_itemEntryInter;
}

const QString AppItem::appId() const
{
    return m_id;
}

const bool AppItem::isValid() const
{
    return m_itemEntryInter->isValid() && !m_itemEntryInter->id().isEmpty();
}

void AppItem::updateWindowInfos(const WindowInfoMap &info)
{
    Q_EMIT windowInfoChanged();
}

void AppItem::windowChanged(uint windowId) {
    Q_EMIT windowInfoChanged();
}
