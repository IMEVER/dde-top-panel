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

#include "../frame/util/XUtils.h"

#include "dbusmenuimporter.h"
// #include <dbusmenu-qt5/dbusmenuimporter.h>

// #include "registrar_proxy.h"
#include "dbus_registrar.h"

class KDBusMenuImporter : public DBusMenuImporter
{

public:
    KDBusMenuImporter(const QString &service, const QString &path, const QString title, QObject *parent)
        : KDBusMenuImporter(service, path, title, DBusMenuImporterType::ASYNCHRONOUS, parent) {
    }
    KDBusMenuImporter(const QString &service, const QString &path, const QString title, DBusMenuImporterType type, QObject *parent)
        : DBusMenuImporter(service, path, title, type, parent) {
    }

protected:
    QIcon iconForName(const QString &name) override {
        return QIcon::fromTheme(name);
    }
};

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
                KDBusMenuImporter *importer = this->cachedImporter.take(wId);
                if(importer == m_importer)
                    clearMenuImporter();
                importer->deleteLater();
            }
        });
    });

    connect(dbusRegistrar, &DBusRegistrar::WindowRegistered, [ this ](uint wId, const QString &service, const QDBusObjectPath &path){
        // QTimer::singleShot(1000, [this, wId]{
            if (this->m_winId == wId && this->m_importer == nullptr)
            {
                // QDBusObjectPath tmpPath;
                // QString serviceName = this->dbusRegistrar->GetMenuForWindow(wId, tmpPath);
                // QString menuObjectPath = tmpPath.path();

                // if(!serviceName.isEmpty() && !menuObjectPath.isEmpty()) {
                    KDBusMenuImporter *importer = new KDBusMenuImporter(service, path.path(), QString(), this);
                    this->cachedImporter.insert(wId, importer);
                    this->updateApplicationMenu(importer);
                // }
            }
        // });
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

    QMenu *fileMenu = new QMenu("??????");
    fileMenu->addAction("????????????", [](){
        QProcess::startDetached("/usr/bin/dde-file-manager", {"-n"});
    });
    fileMenu->addAction("?????????????????????", [](){
        QProcess::startDetached("/usr/bin/dde-file-manager", {"-r"});
    });
    fileMenu->addAction("????????????", [](){
        QProcess::startDetached("/usr/bin/deepin-terminal", {});
    });
    fileMenu->addAction(QIcon::fromTheme("folder_new"), "???????????????", [](){
        QDir desktopDir;
        desktopDir.setCurrent(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
        QString dir = "???????????????";
        int i = 0;
        while(desktopDir.exists(dir))
        {
            dir = "???????????????" + QString::number(++i);
        }
        desktopDir.mkdir(dir);
    });
    QMenu *createMenu = new QMenu("????????????");
    createMenu->setIcon(QIcon::fromTheme("filenew"));
    createMenu->addAction(QIcon::fromTheme("text-plain"), "????????????", [ = ](){
        createNewFile("??????????????????");
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

    QMenu *recentMenu = new QMenu("????????????");
    recentMenu->setToolTipsVisible(true);
    auto createRecentItem = [ recentMenu ] {
        recentMenu->clear();
        QFile *xmlFile = new QFile(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation).append(QDir::separator()).append("recently-used.xbel"));

        if (xmlFile->open(QIODevice::ReadOnly | QIODevice::Text)) {
            QXmlStreamReader *xmlReader = new QXmlStreamReader(xmlFile);

            //Parse the XML until we reach end of it
            while(!xmlReader->atEnd() && !xmlReader->hasError()) {
                // Read next element
                if(xmlReader->readNextStartElement() && xmlReader->name() == "bookmark")
                {
                        QStringRef filePath = xmlReader->attributes().value("href");
                        if(filePath.startsWith("file:///") && !filePath.startsWith("file:///run/user"))
                        {
                            QFileInfo item(QUrl::fromPercentEncoding(filePath.toLocal8Bit()).remove("file://"));
                            if(item.exists() && item.isFile())
                            {
                                QAction *action = recentMenu->addAction(QFileIconProvider().icon(item), item.fileName(), [item]{
                                    QDesktopServices::openUrl(QUrl::fromLocalFile(item.absoluteFilePath()));
                                });

                                action->setToolTip(item.absolutePath());
                            }
                        }
                }
            }

            //close reader and flush file
            xmlReader->clear();delete xmlReader;
            xmlFile->close();
        }
        xmlFile->deleteLater();

        if(recentMenu->isEmpty())
            recentMenu->addAction("???????????????????????????");
    };
    fileMenu->addMenu(recentMenu);
    QFileSystemWatcher *watcher = new QFileSystemWatcher({ QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation).append(QDir::separator()).append("recently-used.xbel") }, this);
    connect(watcher, &QFileSystemWatcher::fileChanged, [ createRecentItem ](const QString filePath){
        createRecentItem();
    });
    createRecentItem();
    this->desktopMenu->addMenu(fileMenu);


    QMenu *editMenu = new QMenu("??????");
    editMenu->addAction("??????", [ = ](){
        QProcess::startDetached("/usr/bin/qdbus", {"com.deepin.dde.desktop", "/com/deepin/dde/desktop", "com.deepin.dde.desktop.Refresh" });
    });
    editMenu->addAction("????????????", [ = ](){
        QProcess::startDetached("/usr/bin/qdbus", {"com.deepin.dde.desktop", "/com/deepin/dde/desktop", "com.deepin.dde.desktop.ShowWallpaperChooser" });
    });
    editMenu->addAction("????????????", [=](){
        QProcess::startDetached("/usr/bin/qdbus", {"com.deepin.dde.desktop", "/com/deepin/dde/desktop", "com.deepin.dde.desktop.ShowScreensaverChooser" });
    });
    this->desktopMenu->addMenu(editMenu);

    QMenu *gotoMenu = new QMenu("??????");
    gotoMenu->addAction(QIcon::fromTheme("folder_home"), "?????????", [](){
        QProcess::startDetached("/usr/bin/dde-file-manager", {"-O"});
    });
    gotoMenu->addAction(QIcon::fromTheme("folder-documents-symbolic"), "??????", [](){
        QProcess::startDetached("/usr/bin/dde-file-manager", {"~/Documents"});
    });
    gotoMenu->addAction(QIcon::fromTheme("folder-pictures-symbolic"), "??????", [](){
        QProcess::startDetached("/usr/bin/dde-file-manager", {"~/Pictures"});
    });
    gotoMenu->addAction(QIcon::fromTheme("folder-music-symbolic"), "??????", [](){
        QProcess::startDetached("/usr/bin/dde-file-manager", {"~/Music"});
    });
    gotoMenu->addAction(QIcon::fromTheme("folder-videos-symbolic"), "??????", [](){
        QProcess::startDetached("/usr/bin/dde-file-manager", {"~/Videos"});
    });
    gotoMenu->addAction(QIcon::fromTheme("folder-downloads-symbolic"), "??????", [](){
        QProcess::startDetached("/usr/bin/dde-file-manager", {"~/Downloads"});
    });
    gotoMenu->addSeparator();
    gotoMenu->addAction(QIcon::fromTheme("user-trash-symbolic"), "?????????", [](){
        QProcess::startDetached("/usr/bin/dde-file-manager", {"trash:///"});
    });
    gotoMenu->addAction(QIcon::fromTheme("computer-symbolic"), "?????????", [](){
        QProcess::startDetached("/usr/bin/dde-file-manager", {"computer:///"});
    });
    gotoMenu->addAction(QIcon::fromTheme("network-server-symbolic"), "????????????", [](){
        QProcess::startDetached("/usr/bin/dde-file-manager", {"network:///"});
    });
    this->desktopMenu->addMenu(gotoMenu);

    QMenu *helpMenu = new QMenu("??????");
    helpMenu->addAction("????????????", [](){
        QProcess::startDetached("/usr/bin/dman", {});
    });
    helpMenu->addAction("??????", [](){
        QProcess::startDetached("/usr/bin/xdg-open", {"https://www.deepin.org"});
    });
    helpMenu->addAction("??????", [](){
        QProcess::startDetached("/usr/bin/xdg-open", {"https://bbs.deepin.org"});
    });
    helpMenu->addAction("????????????", [](){
        QProcess::startDetached("/usr/bin/xdg-open", {"https://wiki.deepin.org/"});
    });

    this->desktopMenu->addMenu(helpMenu);
}

void AppMenuModel::clearMenuImporter()
{
    if (m_importer)
    {
        disconnect(m_importer, &DBusMenuImporter::menuUpdated, this, nullptr);
        m_importer->menu()->removeEventFilter(this);
        m_importer = nullptr;
    }

    emit clearMenu();
}

void AppMenuModel::setWinId(const WId &id, bool isDesktop, const QString title)
{
    if (m_winId == id)
    {
        return;
    }

    clearMenuImporter();
    m_winId = id;

    if(id > 0 && isDesktop == false)
        switchApplicationMenu(title);
    else {
        emit modelNeedsUpdate();
    }
}

void AppMenuModel::switchApplicationMenu(const QString title)
{
        if (this->cachedImporter.contains(m_winId))
        {
            return updateApplicationMenu(this->cachedImporter[m_winId]);
        }

        KWindowInfo info(m_winId, NET::WMState | NET::WMWindowType, NET::WM2TransientFor);
        if(!info.valid())
            return;

        if (info.hasState(NET::SkipTaskbar) || info.windowType(NET::UtilityMask) == NET::Utility || info.windowType(NET::DesktopMask) == NET::Desktop) {
            return;
        }

        auto updateMenuFromWindowIfHasMenu = [this, title](WId id) {
            if(this->cachedImporter.contains(id))
            {
                updateApplicationMenu(this->cachedImporter.value(id));
                return true;
            }

            QDBusObjectPath tmpPath;
            QString serviceName = this->dbusRegistrar->GetMenuForWindow(id, tmpPath);
            QString menuObjectPath = tmpPath.path();

            if (!serviceName.isEmpty() && !menuObjectPath.isEmpty()) {
                KDBusMenuImporter *importer = new KDBusMenuImporter(serviceName, menuObjectPath, title, this);
                this->cachedImporter.insert(id, importer);
                updateApplicationMenu(importer);
                return true;
            }

            return false;
        };

        while (info.valid() && info.transientFor()) {
            if (updateMenuFromWindowIfHasMenu(info.transientFor())) {
                return;
            }
            info = KWindowInfo(info.transientFor(), NET::WMState, NET::WM2TransientFor);
        }

        updateMenuFromWindowIfHasMenu(m_winId);
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

void AppMenuModel::updateApplicationMenu(KDBusMenuImporter *importer)
{
    m_importer = importer;
    QMenu *m_menu = m_importer->menu();
    if(m_importer->isFirstShow())
    {
        connect(m_importer, &DBusMenuImporter::menuUpdated, this, [ = ] {
            // qInfo()<<"after update ......";
            m_menu->installEventFilter(this);
            emit modelNeedsUpdate();
        });

        QTimer::singleShot(100, importer, [ importer ] { QMetaObject::invokeMethod(importer, "updateMenu", Qt::QueuedConnection); });
    } else {
        m_menu->installEventFilter(this);
        emit modelNeedsUpdate();
    }

    connect(m_importer, &DBusMenuImporter::actionActivationRequested, this, [this, m_menu](QAction * action) {
        if (m_menu) {
            const auto actions = m_menu->actions();
            auto it = std::find(actions.begin(), actions.end(), action);

            if (it != actions.end()) {
                requestActivateIndex(it - actions.begin());
            }
        }
    });
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
