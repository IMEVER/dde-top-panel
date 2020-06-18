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

#ifndef APPITEM_H
#define APPITEM_H

#include "dockitem.h"
#include <com_deepin_dde_daemon_dock_entry.h>

using DockEntryInter = com::deepin::dde::daemon::dock::Entry;

class AppItem : public QObject
{
    Q_OBJECT

public:
    explicit AppItem(const QDBusObjectPath &entry, QWidget *parent = nullptr);
    ~AppItem();

    const QString appId() const;
    const bool isValid() const;

signals:
    void windowInfoChanged() const;

private slots:
    void updateWindowInfos(const WindowInfoMap &info);
    void windowChanged(uint windowId=0);

private:
    DockEntryInter *m_itemEntryInter;
    QString m_id;
};

#endif // APPITEM_H
