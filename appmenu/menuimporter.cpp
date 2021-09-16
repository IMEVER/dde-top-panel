/*
  This file is part of the KDE project.

  Copyright (c) 2011 Lionel Chauvin <megabigbug@yahoo.fr>
  Copyright (c) 2011,2012 Cédric Bellegarde <gnumdk@gmail.com>
  Copyright (c) 2016 Kai Uwe Broulik <kde@privat.broulik.de>

  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
*/

#include "menuimporter.h"
#include "menuimporteradaptor.h"
#include "dbusmenutypes_p.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusServiceWatcher>
#include <QX11Info>

#include <KWindowSystem>
#include <KWindowInfo>

MenuImporter::MenuImporter(QObject* parent)
: QObject(parent)
, m_serviceWatcher(new QDBusServiceWatcher(this))
{
    qDBusRegisterMetaType<DBusMenuLayoutItem>();
    m_serviceWatcher->setConnection(QDBusConnection::sessionBus());
    m_serviceWatcher->setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, &MenuImporter::slotServiceUnregistered);
}

MenuImporter::~MenuImporter()
{
    QDBusConnection::sessionBus().unregisterService(DBusRegistrar::staticServiceName());
    delete m_serviceWatcher;
}

bool MenuImporter::connectToBus()
{
    if (!QDBusConnection::sessionBus().registerService(DBusRegistrar::staticServiceName())) {
        return false;
    }
    new MenuImporterAdaptor(this);
    QDBusConnection::sessionBus().registerObject(DBusRegistrar::staticPathName(), this);

    return true;
}

void MenuImporter::RegisterWindow(WId id, const QDBusObjectPath& path)
{
    KWindowInfo info(id, NET::WMWindowType, NET::WM2WindowClass);
    NET::WindowTypes mask = NET::AllTypesMask;

    // Menu can try to register, right click in gimp for example
    if (info.windowType(mask) & (NET::Menu|NET::DropdownMenu|NET::PopupMenu)) {
        return;
    }

    if (info.windowType(mask) != NET::Normal) {
        qDebug() << "Ignoring window class name: "<<info.windowClassName()<<", id: " << id << ", type: " << info.windowType(mask);
        return;
    }

    if (path.path().isEmpty()) //prevent bad dbusmenu usage
        return;

    QString service = message().service();

    QString classClass = info.windowClassClass();
    m_windowClasses.insert(id, classClass);
    m_menuServices.insert(id, service);
    m_menuPaths.insert(id, path);

    if (!m_serviceWatcher->watchedServices().contains(service)) {
        m_serviceWatcher->addWatchedService(service);
    }

    emit WindowRegistered(id, service, path);
}

void MenuImporter::UnregisterWindow(WId id)
{
    m_menuServices.remove(id);
    m_menuPaths.remove(id);
    m_windowClasses.remove(id);

    emit WindowUnregistered(id);
}

QString MenuImporter::GetMenuForWindow(WId id, QDBusObjectPath& path)
{
    path = m_menuPaths.value(id);
    return m_menuServices.value(id);
}

MenuList MenuImporter::GetMenus()
{
    MenuList list;
    foreach(WId id, m_menuServices.keys())
        list.append(MenuStruct(id, m_menuServices.value(id), m_menuPaths.value(id)));
    return list;
}

void MenuImporter::slotServiceUnregistered(const QString& service)
{
    QList<WId> ids = m_menuServices.keys(service);
    for(WId id : ids)
        UnregisterWindow(id);

    m_serviceWatcher->removeWatchedService(service);
}
