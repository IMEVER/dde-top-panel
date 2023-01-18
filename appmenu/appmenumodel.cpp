/******************************************************************
 * Copyright 2016 Kai Uwe Broulik <kde@privat.broulik.de>
 * Copyright 2016 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************/

#include "appmenumodel.h"

#include <QAction>
#include <QMenu>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>
#include <QGuiApplication>
#include <QDebug>
#include <QWindow>
#include <QApplication>
#include <QDesktopWidget>
#include <QTimer>
#include <QProcess>
#include <QFile>
#include <QDir>
#include <QFileIconProvider>
#include <QStandardPaths>
#include <QTimer>
#include <QActionEvent>
#include <QFileSystemWatcher>
#include <QXmlStreamReader>
#include <QDesktopServices>
#include <QUrl>

#include "dbusmenuimporter.h"
// #include <dbusmenu-qt5/dbusmenuimporter.h>

// #include "registrar_proxy.h"
#include "dbus_registrar.h"

AppMenuModel::AppMenuModel(QObject *parent) : QObject(parent)//, m_serviceWatcher(new QDBusServiceWatcher(this))
{
    m_importer = nullptr;
    // registrarProxy = new RegistrarProxy(this);
    // registrarProxy->Reference();
    dbusRegistrar = new DBusRegistrar(this);

    connect(dbusRegistrar, &DBusRegistrar::WindowUnregistered, [ this ](WId wId){
        QTimer::singleShot(0, [ this, wId ]{
            if(this->cachedImporter.contains(wId))
            {
                DBusMenuImporter *importer = this->cachedImporter.take(wId);
                if(importer == m_importer)
                    clearMenuImporter();
                importer->deleteLater();
            }
        });
    });

    connect(dbusRegistrar, &DBusRegistrar::WindowRegistered, [ this ](uint wId, const QString &service, const QDBusObjectPath &path){
        QTimer::singleShot(10, [=]{
            if (!this->cachedImporter.contains(wId))
            {
                DBusMenuImporter *importer = new DBusMenuImporter(service, path.path(), this);
                this->cachedImporter.insert(wId, importer);

                if (this->m_winId == wId)
                    this->updateApplicationMenu(importer);
            }
        });
    });

    initDesktopMenu();
}

AppMenuModel::~AppMenuModel()
{
    // registrarProxy->UnReference();
    delete desktopMenu;
}

void AppMenuModel::initDesktopMenu()
{
    this->desktopMenu = new QMenu();

    auto createNewFile = [](QString fileName, QString subfix="txt") {
        QDir::setCurrent(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
        QFile file(fileName + "." + subfix);
        int i = 0;
        while (file.exists())
        {
            file.setFileName(QString("%1 %2.%3").arg(fileName).arg(++i).arg(subfix));
        }
        file.open(QIODevice::WriteOnly);
        file.close();
    };

    QMenu *fileMenu = new QMenu("文件");
    fileMenu->addAction("新建窗口", []{
        QProcess::startDetached("/usr/bin/dde-file-manager", {"-n"});
    });
    fileMenu->addAction("新建管理员窗口", []{
        QProcess::startDetached("/usr/bin/dde-file-manager", {"-r"});
    });
    fileMenu->addAction("新建终端", []{
        QProcess::startDetached("/usr/bin/deepin-terminal", {"-w", QDir::homePath()});
    });
    fileMenu->addSeparator();
    fileMenu->addAction(QIcon::fromTheme("folder_new"), "新建文件夹", []{
        QDir desktopDir;
        desktopDir.setCurrent(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
        QString dir = "新建文件夹";
        int i = 0;
        while(desktopDir.exists(dir))
        {
            dir = "新建文件夹" + QString::number(++i);
        }
        desktopDir.mkdir(dir);
    });
    QMenu *createMenu = new QMenu("新建文档");
    createMenu->setIcon(QIcon::fromTheme("filenew"));
    createMenu->addAction(QIcon::fromTheme("text-plain"), "文本文件", [ createNewFile ]{
        createNewFile("新建文本文件");
    });

    QDir dir;
    dir.setCurrent(QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + QDir::separator() + ".Templates");
    if(dir.exists() && dir.isReadable() && dir != QDir::home())
    {
        createMenu->addSeparator();

        QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::Readable | QDir::NoSymLinks);
        for(QFileInfo fileInfo : list)
        {
            createMenu->addAction(QFileIconProvider().icon(fileInfo), fileInfo.baseName(), [ = ](){
                // createNewFile(fileInfo.baseName(), fileInfo.suffix());
                QDir::setCurrent(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
                QFile file(fileInfo.fileName());
                int i = 0;
                while (file.exists())
                {
                    file.setFileName(QString("%1 %2.%3").arg(fileInfo.baseName()).arg(++i).arg(fileInfo.suffix()));
                }
                QFile::copy(fileInfo.absoluteFilePath(), file.fileName());
            });
        }
    }
    fileMenu->addMenu(createMenu);
    fileMenu->addSeparator();

    QMenu *recentMenu = new QMenu("最近使用");
    recentMenu->setIcon(QIcon::fromTheme("document-open-recent"));
    recentMenu->setToolTipsVisible(true);
    recentMenu->addSeparator();
    QFileSystemWatcher *watcher = new QFileSystemWatcher({ QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation).append(QDir::separator()).append("recently-used.xbel") }, this);
    recentMenu->addAction(QIcon::fromTheme("edit-clear"), "清除记录", [watcher, recentMenu]{
        watcher->blockSignals(true);
        const static QString xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
            <xbel version=\"1.0\"\n\
                xmlns:bookmark=\"http://www.freedesktop.org/standards/desktop-bookmarks\"\n\
                xmlns:mime=\"http://www.freedesktop.org/standards/shared-mime-info\"\n\
            ></xbel>\n";

        QFile file(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation).append(QDir::separator()).append("recently-used.xbel"));
        if(file.open(QIODevice::ExistingOnly | QIODevice::Truncate | QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            stream << xml;
            file.close();

            while(recentMenu->actions().count() > 2) {
                QAction *action = recentMenu->actions().first();
                recentMenu->removeAction(action);
                action->deleteLater();
            }
        }
        watcher->blockSignals(false);
    });
    auto createRecentItem = [ recentMenu ] {
        while(recentMenu->actions().count() > 2) {
            QAction *action = recentMenu->actions().first();
            recentMenu->removeAction(action);
            action->deleteLater();
        }

        QFile *xmlFile = new QFile(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation).append(QDir::separator()).append("recently-used.xbel"));

        if (xmlFile->open(QIODevice::ExistingOnly | QIODevice::ReadOnly | QIODevice::Text)) {
            QXmlStreamReader *xmlReader = new QXmlStreamReader(xmlFile);
            QList<QAction*> actions;
            //Parse the XML until we reach end of it
            while(!xmlReader->atEnd() && !xmlReader->hasError()) {
                // Read next element
                if(xmlReader->readNextStartElement() && xmlReader->name() == "bookmark")
                {
                    QStringRef filePath = xmlReader->attributes().value("href");
                    if(filePath.startsWith("file:///") && !filePath.startsWith("file:///run/user"))
                    {
                        QFileInfo item(QUrl::fromPercentEncoding(filePath.toLocal8Bit()).remove("file://"));
                        if(item.exists())
                        {
                            QAction *action = new QAction(QFileIconProvider().icon(item), item.fileName(), recentMenu);
                            action->setToolTip(item.absolutePath());
                            connect(action, &QAction::triggered, [action]{
                                if(Qt::ControlModifier == QApplication::keyboardModifiers())
                                    QDesktopServices::openUrl(action->toolTip());
                                else
                                    QDesktopServices::openUrl(QUrl::fromLocalFile(action->toolTip() + "/" + action->text()));
                            });
                            actions.append(action);
                        }
                    }
                }
            }
            if(!actions.isEmpty())
                recentMenu->insertActions(recentMenu->actions().first(), actions);

            //close reader and flush file
            xmlReader->clear();delete xmlReader;
            xmlFile->close();
        }
        xmlFile->deleteLater();
        recentMenu->actions().last()->setEnabled(recentMenu->actions().count() > 2);
    };
    fileMenu->addMenu(recentMenu);
    connect(watcher, &QFileSystemWatcher::fileChanged, this, createRecentItem);
    createRecentItem();
    this->desktopMenu->addMenu(fileMenu);

    QMenu *editMenu = new QMenu("编辑");
    editMenu->addAction("刷新", []{
        QProcess::startDetached("/usr/bin/qdbus", {"com.deepin.dde.desktop", "/com/deepin/dde/desktop", "com.deepin.dde.desktop.Refresh" });
    });
    editMenu->addSeparator();
    editMenu->addAction("设置壁纸", []{
        QProcess::startDetached("/usr/bin/qdbus", {"com.deepin.dde.desktop", "/com/deepin/dde/desktop", "com.deepin.dde.desktop.ShowWallpaperChooser" });
    });
    editMenu->addAction("设置屏保", []{
        QProcess::startDetached("/usr/bin/qdbus", {"com.deepin.dde.desktop", "/com/deepin/dde/desktop", "com.deepin.dde.desktop.ShowScreensaverChooser" });
    });
    this->desktopMenu->addMenu(editMenu);

    QMenu *gotoMenu = new QMenu("转到");
    gotoMenu->addAction(QIcon::fromTheme("folder_home"), "家目录", []{
        QProcess::startDetached("/usr/bin/dde-file-manager", {"-O"});
    });
    gotoMenu->addAction(QIcon::fromTheme("folder-documents-symbolic"), "文档", []{
        QProcess::startDetached("/usr/bin/dde-file-manager", {"~/Documents"});
    });
    gotoMenu->addAction(QIcon::fromTheme("folder-pictures-symbolic"), "图库", []{
        QProcess::startDetached("/usr/bin/dde-file-manager", {"~/Pictures"});
    });
    gotoMenu->addAction(QIcon::fromTheme("folder-music-symbolic"), "音乐", []{
        QProcess::startDetached("/usr/bin/dde-file-manager", {"~/Music"});
    });
    gotoMenu->addAction(QIcon::fromTheme("folder-videos-symbolic"), "视频", []{
        QProcess::startDetached("/usr/bin/dde-file-manager", {"~/Videos"});
    });
    gotoMenu->addAction(QIcon::fromTheme("folder-downloads-symbolic"), "下载", []{
        QProcess::startDetached("/usr/bin/dde-file-manager", {"~/Downloads"});
    });
    gotoMenu->addSeparator();
    gotoMenu->addAction(QIcon::fromTheme("user-trash-symbolic"), "垃圾桶", []{
        QProcess::startDetached("/usr/bin/dde-file-manager", {"trash:///"});
    });
    gotoMenu->addAction(QIcon::fromTheme("computer-symbolic"), "计算机", []{
        QProcess::startDetached("/usr/bin/dde-file-manager", {"computer:///"});
    });
    gotoMenu->addAction(QIcon::fromTheme("network-server-symbolic"), "网上邻居", []{
        QProcess::startDetached("/usr/bin/dde-file-manager", {"network:///"});
    });
    if(QFile("/usr/lib/dde-file-manager/addons/libdde-appview-plugin.so").exists())
        gotoMenu->addAction(QIcon::fromTheme("app-launcher"), "应用", []{
            QProcess::startDetached("/usr/bin/dde-file-manager", {"plugin://app"});
        });
    this->desktopMenu->addMenu(gotoMenu);

    QMenu *helpMenu = new QMenu("帮助");
    helpMenu->addAction("帮助手册", []{
        QProcess::startDetached("/usr/bin/dman", {});
    })->setEnabled(QFile("/usr/bin/dman").exists());
    helpMenu->addAction("官网", []{
        QProcess::startDetached("/usr/bin/xdg-open", {"https://www.deepin.org"});
    });
    helpMenu->addAction("论坛", []{
        QProcess::startDetached("/usr/bin/xdg-open", {"https://bbs.deepin.org"});
    });
    helpMenu->addAction("维基百科", []{
        QProcess::startDetached("/usr/bin/xdg-open", {"https://wiki.deepin.org/"});
    });

    this->desktopMenu->addMenu(helpMenu);
}

void AppMenuModel::clearMenuImporter()
{
    if (m_importer)
    {
        disconnect(m_importer, &DBusMenuImporter::menuUpdated, this, nullptr);
        disconnect(m_importer, &DBusMenuImporter::actionActivationRequested, this, nullptr);
        m_importer->menu()->removeEventFilter(this);
        m_importer = nullptr;
    }

    emit clearMenu();
}

void AppMenuModel::setWinId(const WId &id, const QString title)
{
    if (m_winId == id)
        return;

    clearMenuImporter();
    m_winId = id;
    m_title = title;

    KWindowInfo info(m_winId, NET::WMState | NET::WMWindowType);

    if(!info.valid() || info.windowType(NET::DesktopMask) == NET::Desktop)
        emit modelNeedsUpdate();
    else if (this->cachedImporter.contains(m_winId))
        updateApplicationMenu(this->cachedImporter.value(m_winId));
}

QMenu *AppMenuModel::menu() const
{
    return m_importer ? m_importer->menu() : desktopMenu.data();
}

QAction * AppMenuModel::getAction(QAction::MenuRole role)
{
    if(m_importer)
        return m_importer->getAction(role);

    return nullptr;
}

void AppMenuModel::updateApplicationMenu(DBusMenuImporter *importer)
{
    m_importer = importer;
    connect(importer, &DBusMenuImporter::menuUpdated, this, [ = ] {
        // if(m_importer != nullptr) clearMenuImporter();
        m_importer->menu()->installEventFilter(this);
        connect(m_importer, &DBusMenuImporter::actionActivationRequested, this, [this](QAction * action) {
            if (QMenu *m_menu = m_importer->menu()) {
                const auto actions = m_menu->actions();
                auto it = std::find(actions.begin(), actions.end(), action);

                if (it != actions.end()) {
                    requestActivateIndex(it - actions.begin());
                }
            }
        });
        emit modelNeedsUpdate();
    });

    QTimer::singleShot(10, importer, [this, importer ] { if(m_importer == importer) QMetaObject::invokeMethod(importer, "updateMenu", Qt::QueuedConnection); });
}

bool AppMenuModel::eventFilter(QObject *watched, QEvent *event)
{
    if(m_importer && watched == m_importer->menu())
    {
        QActionEvent *actionEvent;
        switch (event->type())
        {
        case QEvent::ActionAdded:
            actionEvent = static_cast<QActionEvent *>(event);
            emit actionAdded(actionEvent->action(), actionEvent->before());
            break;
        case QEvent::ActionRemoved:
            actionEvent = static_cast<QActionEvent *>(event);
            emit actionRemoved(actionEvent->action());
            break;
        default:
            break;
        }
    }

    return false;
}
