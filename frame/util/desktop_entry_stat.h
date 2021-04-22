/*
 * Copyright (C) 2019 ~ 2019 Union Technology Co., Ltd.
 *
 * Author:     zccrs <zccrs@uniontech.com>
 *
 * Maintainer: zccrs <zhangjide@uniontech.com>
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
#ifndef DESKTOP_ENTRY_STAT_H
#define DESKTOP_ENTRY_STAT_H

#include <QHash>
#include <QMap>
#include <QIcon>

struct desktop_entry {
    QString     name;
    QString     displayName;
    QStringList exec;
    QString       icon;
    QString     startup_wm_class;
    QString     desktopFile;
    // QIcon getIcon() {
    //     if(!icon.isEmpty())
    //         return QIcon::fromTheme(icon);
    // }
};

using DesktopEntry = QSharedPointer<struct desktop_entry>;
using DesktopEntryCache = QHash<QString, DesktopEntry>;

class DesktopEntryStat : public QObject
{
    Q_OBJECT
public:
    static DesktopEntryStat *instance();
    ~DesktopEntryStat();
    DesktopEntry getDesktopEntryByName(const QString name);
    DesktopEntry getDesktopEntryByDesktopfile(const QString desktopFile);
    DesktopEntry getDesktopEntryByPid(int pid);
    DesktopEntry searchByName(QString name);

private:
    explicit DesktopEntryStat(QObject *parent = nullptr);
    void createDesktopEntry(const QString desktopFile);
    void refresh();
    void parseDir(QString path);

private:
    DesktopEntryCache cache;
    QMap<QString, QString> deskToNameMap;
};

#endif // DESKTOP_ENTRY_STAT_H
