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
#include <QDirIterator>
#include <QFileInfo>
#include <QDebug>
#include <DDesktopEntry>
#include <QStandardPaths>
#include <QFileSystemWatcher>

#include "desktop_entry_stat.h"

DCORE_USE_NAMESPACE

auto getExecName = [](QStringList execs) {
    for(auto exec : execs)
    {
        exec = exec.remove("\"").trimmed();
        if(exec == "/bin/bash" || exec == "bash" || exec == "sh" || exec == "python" || exec == "gksu" || exec == "pkexec" || exec == "env" || exec.contains("="))
            continue;
        return QFileInfo(exec).baseName();
    }
    return QString();
};

DesktopEntryStat *DesktopEntryStat::instance()
{
    static DesktopEntryStat instancePtr;
    return &instancePtr;
}

DesktopEntryStat::DesktopEntryStat(QObject *parent) : QObject(parent)
{
    QFileSystemWatcher *watcher = new QFileSystemWatcher(this);
    watcher->addPaths(QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation));
    connect(watcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString &path) {
        cache.clear();
        categoryCache.clear();
        refresh();
        emit appsChanged();
    });
    refresh();
}

const QList<Category> &DesktopEntryStat::categories() const {
    static const QList<Category> _categories {
        Category{ "网络通讯", "applications-internet", { "Network", "Chat", "Email", "WebBrowser" } },
        Category{ "影音娱乐", "applications-multimedia", { "Video", "Audio", "AudioVideo", "Music", "Player", "TV" } },
        Category{ "图形图像", "applications-graphics", { "Graphics", "Viewer" } },
        Category{ "编程开发", "applications-development", { "Development", "IDE", "TerminalEmulator", "TextEditor" } },
        Category{ "游戏娱乐", "applications-games", { "Game" } },
        Category{ "办公学习", "applications-education", { "Education", "Office", "Documentation", "Math", "Ocr", "Calculator" } },
        Category{ "系统工具", "applications-system", { "System", "Utility", "Settings", "Archiving", "Compression", "X-GNOME-NetworkSettings", "X-XFCE-SystemSettings" } },
        Category{ "其他应用", "applications-other", {} },
    };

    return _categories;
}

void DesktopEntryStat::createDesktopEntry(const QString desktopFile)
{
    DDesktopEntry entry(desktopFile);

    if (entry.status() != DDesktopEntry::NoError || (entry.contains("NoDisplay") && entry.stringValue("NoDisplay") == "true"))
        return;

    DesktopEntry re;

    re.desktopFile = desktopFile;
    re.displayName = entry.ddeDisplayName();
    re.icon = entry.stringValue("Icon");

    auto tryExec = entry.stringValue("TryExec");
    auto exec = entry.stringValue("Exec");

    if (!tryExec.isEmpty() && !tryExec.contains("AppRun")) {
        re.exec = tryExec.split(' ');
        re.name = getExecName(re.exec);
    } else if (!exec.isEmpty() && !exec.contains("AppRun")) {
        re.exec = exec.split(' ');
        re.name = getExecName(re.exec);
    } else {
        if(!tryExec.isEmpty())
            re.exec = tryExec.split(' ');
        else if(!exec.isEmpty())
            re.exec = exec.split(' ');

        re.name = entry.name();
    }

    cache[desktopFile] = re;
    cache[re.name] = re;

    auto wmclass = entry.stringValue("StartupWMClass").toLower();
    if (!wmclass.isEmpty()) {
        re.startup_wm_class = wmclass;
        cache[wmclass] = re;
    }

    auto &cs = categories();
    auto appCategories = entry.stringListValue("Categories");
    bool checked = false;
    if(!appCategories.isEmpty()) {
        for(auto c : cs) {
            for(auto category : appCategories) {
                checked = c.categories.contains(category);
                if(checked) {
                    categoryCache[c.name].append(re);
                    break;
                }
            }
            if(checked) break;
        }
    }
    if(!checked) categoryCache[cs.last().name].append(re);
}

void DesktopEntryStat::refresh()
{
    for(auto &path :  QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation)) {
        QDirIterator it(path, QStringList("*.desktop"), QDir::Files | QDir::NoDotAndDotDot, QDirIterator::NoIteratorFlags);
        while (it.hasNext()) {
            it.next();
            QString current = it.filePath();
            QFileInfo fileInfo(current);
            while(fileInfo.exists() && fileInfo.isSymLink()) {
                current = fileInfo.symLinkTarget();
                fileInfo = QFileInfo(current);
            }

            if(fileInfo.exists()) createDesktopEntry(current);
        }
    }
}

const QMap<QString, QList<DesktopEntry>> &DesktopEntryStat::categoryMap() const {
    return categoryCache;
}

DesktopEntry DesktopEntryStat::getDesktopEntryByName(const QString name)
{
    return cache.value(name);
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
    return {};
}

DesktopEntry DesktopEntryStat::getDesktopEntryByDesktopfile(const QString desktopFile)
{
    if (!cache.contains(desktopFile))
        createDesktopEntry(desktopFile);

    if (cache.contains(desktopFile))
        return cache[desktopFile];

    return {};
}

DesktopEntry DesktopEntryStat::getDesktopEntryByPid(int pid)
{
    DesktopEntry entry;
    QFile cmdFile(QString("/proc/%1/cmdline").arg(pid));
    if(cmdFile.isReadable())
    {
        QString name = getExecName(QString(cmdFile.readAll()).split(" "));

        if (!cache.contains(name))
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
                            entry = getDesktopEntryByDesktopfile(desktopFile);
                        break;
                    }

                }
            }
        } else
            entry = cache[name];
    }
    return entry;
}