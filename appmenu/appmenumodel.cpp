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

#include "../frame/util/XUtils.h"

#include "dbusmenuimporter.h"
// #include <dbusmenu-qt5/dbusmenuimporter.h>

#include "menuimporter.h"
// #include "registrar_proxy.h"
#include "dbus_registrar.h"

class KDBusMenuImporter : public DBusMenuImporter
{

public:
    KDBusMenuImporter(const QString &service, const QString &path, QObject *parent)
        : KDBusMenuImporter(service, path, DBusMenuImporterType::ASYNCHRONOUS, parent) {
    }
    KDBusMenuImporter(const QString &service, const QString &path, DBusMenuImporterType type, QObject *parent)
        : DBusMenuImporter(service, path, type, parent) {
            this->_service = service;
            this->_path = path;
    }
    QString serviceName()
    {
        return _service;
    }
    QString pathName()
    {
        return _path;
    }

protected:
    QIcon iconForName(const QString &name) override {
        return QIcon::fromTheme(name);
    }


private:
    QString _service;
    QString _path;

};

AppMenuModel::AppMenuModel(QObject *parent) : QObject(parent), m_serviceWatcher(new QDBusServiceWatcher(this))
{
    if (!KWindowSystem::isPlatformX11()) {
        return;
    }

    connect(KWindowSystem::self(), &KWindowSystem::windowRemoved, [ this ](WId wId){
        QTimer::singleShot(5000, [ this, wId ]{
            KDBusMenuImporter *importer = this->cachedImporter.take(wId);
            delete importer;
        });
    });

    m_serviceWatcher->setConnection(QDBusConnection::sessionBus());
    //if our current DBus connection gets lost, close the menu
    //we'll select the new menu when the focus changes
    /*
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString & serviceName) {
        QTimer::singleShot(5000, [ this, serviceName ](){
            WId delWId;
            QPointer<KDBusMenuImporter> delImporter;
            QMap<WId, QPointer<KDBusMenuImporter>>::iterator iter = this->cachedImporter.begin();
            while (iter != this->cachedImporter.end())
            {
                if(iter.value().data()->serviceName() == serviceName)
                {
                    delWId = iter.key();
                    delImporter = iter.value();
                    break;
                }
                iter ++;
            }
            if(delWId > 0)
            {
                this->cachedImporter.remove(delWId);
                delImporter.data()->deleteLater();
            }
        });
    });
    */
    // connect(KWindowSystem::self(), qOverload<WId, NET::Properties, NET::Properties2>(&KWindowSystem::windowChanged),
    //         this, [this](WId id, NET::Properties properties, NET::Properties2 properties2) {
    //     if (KWindowSystem::activeWindow() != id)
    //         return;

    //     if (properties.testFlag(NET::WMGeometry)) {
    //         int currScreenNum = QApplication::desktop()->screenNumber(qobject_cast<QWidget *>(this->parent()));
    //         int activeWinScreenNum = XUtils::getWindowScreenNum(id);
    //         if (activeWinScreenNum >= 0 && activeWinScreenNum != currScreenNum) {
    //             if (XUtils::checkIfBadWindow(this->m_winId) || this->m_winId == id) {
    //                 this->m_winId = -1;
    //                 emit modelNeedsUpdate();
    //             }
    //         } else {
    //             KWindowInfo info(id, NET::WMState | NET::WMGeometry);
    //             if (XUtils::checkIfWinMinimun(this->m_winId)) {
    //                 this->m_winId = -1;
    //                 emit modelNeedsUpdate();
    //             }
    //         }
    //     }
    // });

    auto *menuImporter = new MenuImporter(this);
    menuImporter->connectToBus();

    m_importer = nullptr;
    m_menu = nullptr;
    // registrarProxy = new RegistrarProxy(this);
    // registrarProxy->Reference();
    dbusRegistrar = new DBusRegistrar(this);

    connect(dbusRegistrar, &DBusRegistrar::WindowRegistered, [ this ](uint wId, const QString &service, const QDBusObjectPath &path){
        QTimer::singleShot(1000, [this, wId, service, path]{
            // qInfo()<<"windowRegistered ... wid: "<<wId<<", has importer: "<<(this->m_importer ? "true" : "false");
            if (this->m_winId == wId && this->m_importer == nullptr)
            {
                KDBusMenuImporter *importer = new KDBusMenuImporter(service, path.path(), this);
                this->cachedImporter.insert(wId, importer);
                this->updateApplicationMenu(importer);
            }
        });
    });

    initDesktopMenu();
    m_appMenuMap = new QMap<QAction::MenuRole, QAction *>();
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
    fileMenu->addAction("新建窗口", [](){
        QProcess::startDetached("/usr/bin/dde-file-manager", {"-n"});
    });
    fileMenu->addAction("新建管理员窗口", [](){
        QProcess::startDetached("/usr/bin/dde-file-manager", {"-r"});
    });
    fileMenu->addAction("打开终端", [](){
        QProcess::startDetached("/usr/bin/deepin-terminal", {});
    });
    fileMenu->addAction(QIcon::fromTheme("folder_new"), "新建文件夹", [](){
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
    createMenu->addAction(QIcon::fromTheme("text-plain"), "文本文件", [ = ](){
        createNewFile("新建文本文件");
    });

    QDir dir;
    dir.setCurrent(QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + QDir::separator() + ".Templates");
    if(dir.exists() && dir.isReadable())
    {
        createMenu->addSeparator();

        QFileInfoList list = dir.entryInfoList(QDir::Files);
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
    this->desktopMenu->addMenu(fileMenu);


    QMenu *editMenu = new QMenu("编辑");
    editMenu->addAction("刷新", [ = ](){
        QProcess::startDetached("/usr/bin/qdbus", {"com.deepin.dde.desktop", "/com/deepin/dde/desktop", "com.deepin.dde.desktop.Refresh" });
    });
    editMenu->addAction("设置壁纸", [ = ](){
        QProcess::startDetached("/usr/bin/qdbus", {"com.deepin.dde.desktop", "/com/deepin/dde/desktop", "com.deepin.dde.desktop.ShowWallpaperChooser" });
    });
    editMenu->addAction("设置屏保", [=](){
        QProcess::startDetached("/usr/bin/qdbus", {"com.deepin.dde.desktop", "/com/deepin/dde/desktop", "com.deepin.dde.desktop.ShowScreensaverChooser" });
    });
    this->desktopMenu->addMenu(editMenu);

    QMenu *gotoMenu = new QMenu("转到");
    gotoMenu->addAction(QIcon::fromTheme("folder_home"), "家目录", [](){
        QProcess::startDetached("/usr/bin/dde-file-manager", {"-O"});
    });
    gotoMenu->addAction(QIcon::fromTheme("folder-documents-symbolic"), "文档", [](){
        QProcess::startDetached("/usr/bin/dde-file-manager", {"~/Documents"});
    });
    gotoMenu->addAction(QIcon::fromTheme("folder-pictures-symbolic"), "图库", [](){
        QProcess::startDetached("/usr/bin/dde-file-manager", {"~/Pictures"});
    });
    gotoMenu->addAction(QIcon::fromTheme("folder-music-symbolic"), "音乐", [](){
        QProcess::startDetached("/usr/bin/dde-file-manager", {"~/Music"});
    });
    gotoMenu->addAction(QIcon::fromTheme("folder-videos-symbolic"), "视频", [](){
        QProcess::startDetached("/usr/bin/dde-file-manager", {"~/Videos"});
    });
    gotoMenu->addAction(QIcon::fromTheme("folder-downloads-symbolic"), "下载", [](){
        QProcess::startDetached("/usr/bin/dde-file-manager", {"~/Downloads"});
    });
    gotoMenu->addSeparator();
    gotoMenu->addAction(QIcon::fromTheme("user-trash-symbolic"), "垃圾桶", [](){
        QProcess::startDetached("/usr/bin/dde-file-manager", {"trash:///"});
    });
    gotoMenu->addAction(QIcon::fromTheme("computer-symbolic"), "计算机", [](){
        QProcess::startDetached("/usr/bin/dde-file-manager", {"computer:///"});
    });
    gotoMenu->addAction(QIcon::fromTheme("network-server-symbolic"), "网上邻居", [](){
        QProcess::startDetached("/usr/bin/dde-file-manager", {"network:///"});
    });
    this->desktopMenu->addMenu(gotoMenu);

    QMenu *helpMenu = new QMenu("帮助");
    helpMenu->addAction("帮助手册", [](){
        QProcess::startDetached("/usr/bin/dman", {});
    });
    helpMenu->addAction("官网", [](){
        QProcess::startDetached("/usr/bin/xdg-open", {"https://www.deepin.org"});
    });
    helpMenu->addAction("论坛", [](){
        QProcess::startDetached("/usr/bin/xdg-open", {"https://bbs.deepin.org"});
    });
    helpMenu->addAction("维基百科", [](){
        QProcess::startDetached("/usr/bin/xdg-open", {"https://wiki.deepin.org/"});
    });

    this->desktopMenu->addMenu(helpMenu);
}

void AppMenuModel::clearMenuImporter()
{
    m_appMenuMap->clear();
    emit clearMenu();
    if (m_importer)
    {
        disconnect(m_importer, &DBusMenuImporter::menuUpdated, this, nullptr);
        m_serviceWatcher->removeWatchedService(m_importer->serviceName());
        m_importer = nullptr;

        if(m_menu && m_menu != desktopMenu)
        {
            m_menu->removeEventFilter(this);
        }
    }

    m_menu = nullptr;
}

void AppMenuModel::setWinId(const WId &id, bool isDesktop)
{
    if (m_winId == id)
    {
        return;
    }

    clearMenuImporter();
    m_winId = id;

    if(id > 0 && isDesktop == false)
        onActiveWindowChanged();
    else {
        m_menu = desktopMenu;
        emit modelNeedsUpdate();
    }
}

void AppMenuModel::onActiveWindowChanged()
{
    int currScreenNum = QApplication::desktop()->screenNumber(qobject_cast<QWidget*>(this->parent()));
    int activeWinScreenNum = XUtils::getWindowScreenNum(m_winId);

    if (activeWinScreenNum >= 0 && activeWinScreenNum != currScreenNum) {
        return;
    }

    if (KWindowSystem::isPlatformX11()) {
        if (this->cachedImporter.contains(m_winId))
        {
            return updateApplicationMenu(this->cachedImporter[m_winId]);
        }

        KWindowInfo info(m_winId, NET::WMState | NET::WMWindowType | NET::WMGeometry, NET::WM2TransientFor);

        if (info.hasState(NET::SkipTaskbar) || info.windowType(NET::UtilityMask) == NET::Utility || info.windowType(NET::DesktopMask) == NET::Desktop) {
            return;
        }

        auto updateMenuFromWindowIfHasMenu = [this](WId id) {
            if(this->cachedImporter.contains(id))
            {
                this->m_winId = id;
                updateApplicationMenu(this->cachedImporter.value(id));
                return true;
            }

            QDBusObjectPath tmpPath;
            QString serviceName = this->dbusRegistrar->GetMenuForWindow(id, tmpPath);
            QString menuObjectPath = tmpPath.path();

            if (!serviceName.isEmpty() && !menuObjectPath.isEmpty()) {
                KDBusMenuImporter *importer = new KDBusMenuImporter(serviceName, menuObjectPath, this);
                this->m_winId = id;
                this->cachedImporter.insert(id, importer);
                updateApplicationMenu(importer);
                return true;
            }

            return false;
        };

        KWindowInfo transientInfo = KWindowInfo(info.transientFor(), NET::WMState | NET::WMWindowType | NET::WMGeometry, NET::WM2TransientFor);
        while (transientInfo.win()) {
            if (updateMenuFromWindowIfHasMenu(transientInfo.win())) {
                return;
            }

            transientInfo = KWindowInfo(transientInfo.transientFor(), NET::WMState | NET::WMWindowType | NET::WMGeometry, NET::WM2TransientFor);
        }

        updateMenuFromWindowIfHasMenu(m_winId);
    }
}

QMenu *AppMenuModel::menu() const
{
    return m_menu.data();
}

QAction * AppMenuModel::getAppMenu(QAction::MenuRole role)
{
    return m_appMenuMap->value(role, nullptr);
}

void AppMenuModel::updateApplicationMenu(KDBusMenuImporter *importer)
{
    m_serviceWatcher->setWatchedServices(QStringList({importer->serviceName()}));

    m_importer = importer;
    m_menu = m_importer->menu();
    if(m_menu->isEmpty())
    {
        QMetaObject::invokeMethod(m_importer, "updateMenu", Qt::QueuedConnection);
        // qInfo()<<"before update ......";
        connect(m_importer, &DBusMenuImporter::menuUpdated, this, [ = ]() {
            // qInfo()<<"after update ......";
            m_menu->installEventFilter(this);
            parseAppMenu(m_menu);
            emit modelNeedsUpdate();
        });
    } else {
        m_menu->installEventFilter(this);
        parseAppMenu(m_menu);
        emit modelNeedsUpdate();
    }

    connect(m_importer, &DBusMenuImporter::actionActivationRequested, this, [this](QAction * action) {
        if (m_menu) {
            const auto actions = m_menu->actions();
            auto it = std::find(actions.begin(), actions.end(), action);

            if (it != actions.end()) {
                requestActivateIndex(it - actions.begin());
            }
        }
    });
}

void AppMenuModel::parseAppMenu(QMenu *menu)
{
    QList<QAction::MenuRole> roles{QAction::AboutRole, QAction::QuitRole, QAction::PreferencesRole};
    auto parser = [ this, roles ](QAction *action) {
        QAction::MenuRole role = action->menuRole();
        if(roles.contains(role))
        {
            this->m_appMenuMap->insert(role, action);
            return true;
        }
        return false;
    };

    if (!menu->isEmpty() && menu != desktopMenu)
    {
        for(auto *action : menu->actions())
        {
            if(action->menu())
            {
                for(auto *subAction : action->menu()->actions())
                {
                    if(subAction->menu())
                    {
                        for(auto lastAction : subAction->menu()->actions())
                        {
                            if(!lastAction->menu())
                            {
                                if(parser(lastAction) && m_appMenuMap->size() == 3)
                                    return;
                            }
                        }
                    }
                    else
                    {
                        if(parser(subAction) && m_appMenuMap->size() == 3)
                            return;
                    }
                }
            }
            else
            {
                if(parser(action) && m_appMenuMap->size() == 3)
                    return;
            }
        }
    }
}

void AppMenuModel::deleAppMenu(QMenu *menu)
{
    if(m_appMenuMap->size() > 0)
    {
        for(auto *action : menu->actions())
        {
            if(!action->menu())
            {
                if(m_appMenuMap->remove(action->menuRole()) == 1 && m_appMenuMap->size() == 0)
                    return;
            }
            else
            {
                for(auto *subAction : action->menu()->actions())
                {
                    if(!subAction->menu())
                    {
                        if(m_appMenuMap->remove(subAction->menuRole()) == 1 && m_appMenuMap->size() == 0)
                            return;
                    }
                    else
                    {
                        for(auto lastAction : subAction->menu()->actions())
                        {
                            if (!lastAction->menu())
                            {
                                if(m_appMenuMap->remove(lastAction->menuRole()) == 1 && m_appMenuMap->size() == 0)
                                    return;
                            }
                        }
                    }

                }
            }
        }
    }
}

bool AppMenuModel::eventFilter(QObject *watched, QEvent *event)
{
    if(watched == m_menu)
    {
        QActionEvent *actionEvent;
        QMenu *menu;
        switch (event->type())
        {
        case QEvent::ActionAdded:
            actionEvent = static_cast<QActionEvent *>(event);
            menu = new QMenu;
            menu->addAction(actionEvent->action());
            parseAppMenu(menu);
            menu->deleteLater();
            emit actionAdded(actionEvent->action(), actionEvent->before());
            // QMetaObject::invokeMethod(m_menu, "aboutToShow");
            // if(actionEvent->action()->menu())
            // {
                // QMetaObject::invokeMethod(actionEvent->action()->menu(), "aboutToShow");
                // actionEvent->action()->menu()->aboutToShow();
            // }
            break;
        case QEvent::ActionRemoved:
            actionEvent = static_cast<QActionEvent *>(event);
            menu = new QMenu;
            menu->addAction(actionEvent->action());
            deleAppMenu(menu);
            menu->deleteLater();
            emit actionRemoved(actionEvent->action());
            break;
        default:
            break;
        }
    }

    return false;
}
