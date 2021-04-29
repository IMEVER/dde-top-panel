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

#include "config-X11.h"

#if HAVE_X11
#include <QX11Info>
#include <xcb/xcb.h>
#endif

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
#include "../frame/util/XUtils.h"

#include "dbusmenuimporter.h"
// #include <dbusmenu-qt5/dbusmenuimporter.h>

#include "menuimporter.h"

static const QByteArray s_x11AppMenuServiceNamePropertyName = QByteArrayLiteral("_KDE_NET_WM_APPMENU_SERVICE_NAME");
static const QByteArray s_x11AppMenuObjectPathPropertyName = QByteArrayLiteral("_KDE_NET_WM_APPMENU_OBJECT_PATH");

#if HAVE_X11
static QHash<QByteArray, xcb_atom_t> s_atoms;
#endif

class KDBusMenuImporter : public DBusMenuImporter
{

public:
    KDBusMenuImporter(const QString &service, const QString &path, QObject *parent)
        : DBusMenuImporter(service, path, parent) {

    }

protected:
    QIcon iconForName(const QString &name) override {
        return QIcon::fromTheme(name);
    }
};

AppMenuModel::AppMenuModel(QObject *parent) : QObject(parent), m_serviceWatcher(new QDBusServiceWatcher(this))
{
    if (!KWindowSystem::isPlatformX11()) {
        return;
    }

    // connect(KWindowSystem::self(), &KWindowSystem::activeWindowChanged, this, &AppMenuModel::onActiveWindowChanged);
    // onActiveWindowChanged(KWindowSystem::activeWindow());

    m_serviceWatcher->setConnection(QDBusConnection::sessionBus());
    //if our current DBus connection gets lost, close the menu
    //we'll select the new menu when the focus changes
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString & serviceName) {
        if (serviceName == m_serviceName) {
            clearMenuImporter();
        }
    });

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
}

AppMenuModel::~AppMenuModel() = default;

bool AppMenuModel::menuAvailable() const
{
    return m_menu && m_menu->actions().count() > 0;
}

void AppMenuModel::clearMenuImporter()
{
    if (m_importer) {
        emit clearMenu();
        disconnect(m_importer.data());
        m_importer->deleteLater();
        m_serviceWatcher->removeWatchedService(m_serviceName);
        m_menu = nullptr;
        m_importer = nullptr;
    }
}

void AppMenuModel::setWinId(const WId &id, bool isDesktop)
{
    if (m_winId == id)
    {
        return;
    }

    m_winId = id;
    clearMenuImporter();

    if(id > 0 && isDesktop == false)
        onActiveWindowChanged();
}

void AppMenuModel::onActiveWindowChanged()
{
    qApp->removeNativeEventFilter(this);

    int currScreenNum = QApplication::desktop()->screenNumber(qobject_cast<QWidget*>(this->parent()));
    int activeWinScreenNum = XUtils::getWindowScreenNum(m_winId);

    if (activeWinScreenNum >= 0 && activeWinScreenNum != currScreenNum) {
        return;
    }

#if HAVE_X11

    if (KWindowSystem::isPlatformX11()) {

        KWindowInfo info(m_winId, NET::WMState | NET::WMWindowType | NET::WMGeometry, NET::WM2TransientFor);

        if (info.hasState(NET::SkipTaskbar) || info.windowType(NET::UtilityMask) == NET::Utility || info.windowType(NET::DesktopMask) == NET::Desktop) {
            return;
        }

        auto *c = QX11Info::connection();

        auto getWindowPropertyString = [c](WId id, const QByteArray & name) -> QByteArray {
            QByteArray value;

            if (!s_atoms.contains(name))
            {
                const xcb_intern_atom_cookie_t atomCookie = xcb_intern_atom(c, false, name.length(), name.constData());
                QScopedPointer<xcb_intern_atom_reply_t, QScopedPointerPodDeleter> atomReply(xcb_intern_atom_reply(c, atomCookie, nullptr));

                if (atomReply.isNull()) {
                    return value;
                }

                s_atoms[name] = atomReply->atom;

                if (s_atoms[name] == XCB_ATOM_NONE) {
                    return value;
                }
            }

            static const long MAX_PROP_SIZE = 10000;
            auto propertyCookie = xcb_get_property(c, false, id, s_atoms[name], XCB_ATOM_STRING, 0, MAX_PROP_SIZE);
            QScopedPointer<xcb_get_property_reply_t, QScopedPointerPodDeleter> propertyReply(xcb_get_property_reply(c, propertyCookie, nullptr));

            if (propertyReply.isNull())
            {
                return value;
            }

            if (propertyReply->type == XCB_ATOM_STRING && propertyReply->format == 8 && propertyReply->value_len > 0)
            {
                const char *data = (const char *) xcb_get_property_value(propertyReply.data());
                int len = propertyReply->value_len;

                if (data) {
                    value = QByteArray(data, data[len - 1] ? len : len - 1);
                }
            }

            return value;
        };

        auto updateMenuFromWindowIfHasMenu = [this, &getWindowPropertyString](WId id) {
            const QString serviceName = QString::fromUtf8(getWindowPropertyString(id, s_x11AppMenuServiceNamePropertyName));
            const QString menuObjectPath = QString::fromUtf8(getWindowPropertyString(id, s_x11AppMenuObjectPathPropertyName));

            if (!serviceName.isEmpty() && !menuObjectPath.isEmpty()) {
                updateApplicationMenu(serviceName, menuObjectPath);
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

        if (updateMenuFromWindowIfHasMenu(m_winId)) {
            return;
        }

        // monitor whether an app menu becomes available later
        // this can happen when an app starts, shows its window, and only later announces global menu (e.g. Firefox)
        qApp->installNativeEventFilter(this);
    }

#endif

}

QMenu *AppMenuModel::menu() const
{
    return m_menu.data();
}

void AppMenuModel::updateApplicationMenu(const QString &serviceName, const QString &menuObjectPath)
{
    m_serviceName = serviceName;
    m_serviceWatcher->setWatchedServices(QStringList({m_serviceName}));

    m_importer = new KDBusMenuImporter(serviceName, menuObjectPath, this);
    QMetaObject::invokeMethod(m_importer, "updateMenu", Qt::QueuedConnection);

    connect(m_importer.data(), &DBusMenuImporter::clearMenu, this, &AppMenuModel::clearMenu);

    connect(m_importer.data(), &DBusMenuImporter::menuUpdated, this, [ = ]() {
        m_menu = m_importer->menu();

        if (m_menu)
        {
            emit modelNeedsUpdate();
        }
    });

    connect(m_importer.data(), &DBusMenuImporter::actionActivationRequested, this, [this](QAction * action) {
        if (m_menu) {
            const auto actions = m_menu->actions();
            auto it = std::find(actions.begin(), actions.end(), action);

            if (it != actions.end()) {
                requestActivateIndex(it - actions.begin());
            }
        }
    });
}

bool AppMenuModel::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(result);

    if (!KWindowSystem::isPlatformX11() || eventType != "xcb_generic_event_t") {
        return false;
    }

#if HAVE_X11
    auto e = static_cast<xcb_generic_event_t *>(message);
    const uint8_t type = e->response_type & ~0x80;

    if (type == XCB_PROPERTY_NOTIFY) {
        auto *event = reinterpret_cast<xcb_property_notify_event_t *>(e);

        if (event->window == m_winId) {

            auto serviceNameAtom = s_atoms.value(s_x11AppMenuServiceNamePropertyName);
            auto objectPathAtom = s_atoms.value(s_x11AppMenuObjectPathPropertyName);

            if (serviceNameAtom != XCB_ATOM_NONE && objectPathAtom != XCB_ATOM_NONE) { // shouldn't happen
                if (event->atom == serviceNameAtom || event->atom == objectPathAtom) {
                    // see if we now have a menu
                    onActiveWindowChanged();
                }
            }
        }
    }

#else
    Q_UNUSED(message);
#endif

    return false;
}

