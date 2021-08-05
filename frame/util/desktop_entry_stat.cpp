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

#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>

#include <QFileInfo>
#include <QDebug>
#include <DDesktopEntry>
#include <QStandardPaths>

#include "desktop_entry_stat.h"

DCORE_USE_NAMESPACE

static DesktopEntryStat *instancePtr;

DesktopEntryStat *DesktopEntryStat::instance()
{
    if (!instancePtr)
    {
        instancePtr = new DesktopEntryStat();
    }
    return instancePtr;
}

auto print_err_entry = [](decltype(errno) e, const QString &msg)
{
    qDebug() << QString("Error: [%1] %2, ").arg(e).arg(strerror(e)) << msg;
};

DesktopEntryStat::DesktopEntryStat(QObject *parent) : QObject(parent)
{
    refresh();
}

void DesktopEntryStat::createDesktopEntry(const QString desktopFile)
{
    DDesktopEntry entry(desktopFile);

    if (entry.contains("NoDisplay") && entry.stringValue("NoDisplay") == "true")
    {
        return;
    }

    auto re = DesktopEntry(new desktop_entry {});

    re->desktopFile = desktopFile;

    re->displayName = entry.ddeDisplayName();
    re->icon = entry.stringValue("Icon");

    auto tryExec = entry.stringValue("TryExec");
    auto exec = entry.stringValue("Exec");
    if (!tryExec.isEmpty() && !tryExec.contains("AppRun")) {
        re->exec = tryExec.split(' ');
        re->name = QFileInfo(trim(re->exec[0])).baseName();
    } else if (!exec.isEmpty() && !exec.contains("AppRun")) {
        re->exec = exec.split(' ');
        re->name = QFileInfo(trim(re->exec[0])).baseName();
    } else {
        if(!tryExec.isEmpty())
            re->exec = tryExec.split(' ');
        else if(!exec.isEmpty())
            re->exec = exec.split(' ');

        re->name = entry.name();
    }

    auto wmclass = entry.stringValue("StartupWMClass").toLower();

    if (!cache.contains(re->name) && wmclass.isEmpty()) {
        cache[re->name] = re;
        deskToNameMap[desktopFile] = re->name;
    } else if (!wmclass.isEmpty()) {
        re->startup_wm_class = wmclass;

        if (!cache.contains(wmclass))
        {
            cache[wmclass] = re;
            deskToNameMap[desktopFile] = wmclass;
        }
    }
}

QString DesktopEntryStat::trim(QString str)
{
    if (str.at(0) == '"')
    {
        str = str.mid(1, str.length() -2);
    }
    return str;
}

DesktopEntryStat::~DesktopEntryStat()
{
}

void DesktopEntryStat::refresh()
{
    for(QString appPath :  QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation))
    {
        parseDir(appPath);
    }
}

void DesktopEntryStat::parseDir(QString path)
{
    DIR *dir {};
    struct dirent *dp;
    errno = 0;
    dir = opendir(path.toUtf8());
    if (!dir) {
        print_err_entry(errno, QString("open %1 failed").arg(path));
        return;
    }

    errno = 0;
    while ((dp = readdir(dir))) {
        if (dp->d_type == DT_REG || dp->d_type == DT_LNK) {
            QString name(dp->d_name);

            if (!name.endsWith(".desktop"))
                continue;

            auto file = QString("%1/%2").arg(path).arg(name);

            createDesktopEntry(file);

        } else if (dp->d_type == DT_DIR)
        {
            QString name(dp->d_name);
            if(strcmp(dp->d_name, ".") && strcmp(dp->d_name, ".."))
            {
                auto file = QString("%1/%2").arg(path).arg(name);
                parseDir(file);
            }
        }
    }
    if (errno && !cache.size()) {
        print_err_entry(errno, QString("read %1 failed").arg(path));
    }
    closedir(dir);
}

DesktopEntry DesktopEntryStat::getDesktopEntryByName(const QString name)
{
    return cache[name];
}

DesktopEntry DesktopEntryStat::searchByName(QString name)
{
    auto it = cache.constBegin();
    while (it != cache.constEnd())
    {
        if (it.key().contains(name))
        {
            return it.value();
        }
        it++;
    }
    return nullptr;
}

DesktopEntry DesktopEntryStat::getDesktopEntryByDesktopfile(const QString desktopFile)
{
    if (deskToNameMap.contains(desktopFile))
    {
        return cache[deskToNameMap.value(desktopFile)];
    }
    else
    {
        createDesktopEntry(desktopFile);
        if (deskToNameMap.contains(desktopFile))
        {
            return cache[deskToNameMap.value(desktopFile)];
        }
    }
    return nullptr;
}

DesktopEntry DesktopEntryStat::getDesktopEntryByPid(int pid)
{
    DesktopEntry entry;
    QFile cmdFile(QString("/proc/%1/cmdline").arg(pid));
    if(cmdFile.isReadable())
    {
        QString cmd = cmdFile.readAll();
        cmd = cmd.split(" ")[0];

        QString name = QFileInfo(cmd).baseName();
        entry = cache[name];
        if (!entry)
        {
            QFile environFile(QString("/proc/%1/environ").arg(pid));
            if (environFile.isReadable())
            {
                QByteArrayList environList = environFile.readAll().split('\0');
                for(QString tmp : environList)
                {
                    QStringList tmpList = tmp.split("=");
                    if (tmpList.first() == "GIO_LAUNCHED_DESKTOP_FILE")
                    {
                        QString desktopFile = tmpList.last();
                        if (desktopFile.size() > 0)
                        {
                            entry = getDesktopEntryByDesktopfile(desktopFile);
                        }
                        break;
                    }

                }
            }
        }
    }
    return entry;
}