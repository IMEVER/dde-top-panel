/* This file is part of the dbusmenu-qt library
   Copyright 2009 Canonical
   Author: Aurelien Gateau <aurelien.gateau@canonical.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License (LGPL) as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later
   version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#include "dbusmenuimporter.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusVariant>
#include <QFont>
#include <QPointer>
#include <QTime>
#include <QTimer>
#include <QToolButton>
#include <QWidgetAction>
#include <QSet>
#include <QDebug>

// Local
#include "dbusmenutypes_p.h"
#include "dbusmenushortcut_p.h"
#include "utils_p.h"

// Generated
#include "dbusmenu_interface.h"

static const char *DBUSMENU_PROPERTY_ID = "_dbusmenu_id";
static const char *DBUSMENU_PROPERTY_ICON_NAME = "_dbusmenu_icon_name";
static const char *DBUSMENU_PROPERTY_ICON_DATA_HASH = "_dbusmenu_icon_data_hash";
static const QList<QAction::MenuRole> appRoles = {QAction::QuitRole, QAction::AboutRole, QAction::PreferencesRole};

class DBusMenuImporterPrivate
{
public:
    DBusMenuImporter *q;

    ComCanonicalDbusmenuInterface *m_interface;
    QMenu *m_menu;
    QActionGroup *group;
    QString title;
    using ActionForId = QMap<int, QAction *>;
    QMap<QAction::MenuRole, QAction *> m_appMenus;
    ActionForId m_actionForId;

    QSet<int> m_pendingLayoutUpdates;
    QTimer *m_pendingLayoutUpdateTimer;

    DBusMenuImporterType m_type;

    QDBusPendingCallWatcher *refresh(int id) {
        // auto call = m_interface->GetLayout(id, 1, QStringList());
        QDBusPendingCall call = m_interface->asyncCall("GetLayout", id, 1, QStringList());
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, q);
        watcher->setProperty(DBUSMENU_PROPERTY_ID, id);
        QObject::connect(watcher, &QDBusPendingCallWatcher::finished, q, &DBusMenuImporter::slotGetLayoutFinished);

        return watcher;
    }

    QMenu *createMenu(QWidget *parent) {
        QMenu *menu = new QMenu(parent);
        QObject::connect(menu, SIGNAL(aboutToShow()), q, SLOT(slotMenuAboutToShow()));
        QObject::connect(menu, SIGNAL(aboutToHide()), q, SLOT(slotMenuAboutToHide()));
        return menu;
    }

    /**
     * Init all the immutable action properties here
     * TODO: Document immutable properties?
     *
     * Note: we remove properties we handle from the map (using QMap::take()
     * instead of QMap::value()) to avoid warnings about these properties in
     * updateAction()
     */
    QAction *createAction(int id, const QVariantMap &_map, QWidget *parent) {
        bool isRadio = false;
        QVariantMap map = _map;
        QAction *action = new QAction(parent);
        action->setProperty(DBUSMENU_PROPERTY_ID, id);

        QString type = map.take(QStringLiteral("type")).toString();

        if (type == QLatin1String("separator")) {
            action->setSeparator(true);
            if(map.contains(QLatin1String("visible")))
                updateActionVisible(action, map.value(QLatin1String("visible")));
        }
        else
        {
            if (map.take(QStringLiteral("children-display")).toString() == QLatin1String("submenu")) {
                QMenu *menu = createMenu(parent);
                action->setMenu(menu);
                QObject::connect(action, &QAction::destroyed, menu, &QMenu::deleteLater);
            }
            else
            {
                QString toggleType = map.take(QStringLiteral("toggle-type")).toString();

                if (!toggleType.isEmpty()) {
                    action->setCheckable(true);

                    if (toggleType == QLatin1String("radio")) {
                        if(group == nullptr) group = new QActionGroup(action);
                        group->addAction(action);
                        isRadio = true;
                    }
                }
            }

            updateAction(action, map, map.keys());

            QObject::connect(action, &QAction::triggered, [this, id] {
                this->sendEvent(id, QStringLiteral("clicked"));
            });

            QObject::connect(action, &QAction::hovered, [this, id] {
                this->sendEvent(id, QStringLiteral("hovered"));
            });

            const QAction::MenuRole role = action->menuRole();
            QObject::connect(action, &QAction::destroyed, q, [this, id, role] {
                m_actionForId.remove(id);

                switch (role)
                {
                case QAction::AboutRole:
                case QAction::QuitRole:
                case QAction::PreferencesRole:
                    m_appMenus.remove(role);
                default:
                    break;
                }
            });
        }

        if(isRadio == false) group = nullptr;
        return action;
    }

    /**
     * Update mutable properties of an action. A property may be listed in
     * requestedProperties but not in map, this means we should use the default value
     * for this property.
     *
     * @param action the action to update
     * @param map holds the property values
     * @param requestedProperties which properties has been requested
     */
    void updateAction(QAction *action, const QVariantMap &map, const QStringList &requestedProperties) {
        Q_FOREACH (const QString &key, requestedProperties) {
            updateActionProperty(action, key, map.value(key));
        }
    }

    void updateActionProperty(QAction *action, const QString &key, const QVariant &value) {
        if (key == QLatin1String("label")) {
            updateActionLabel(action, value);
        } else if (key == QLatin1String("enabled")) {
            updateActionEnabled(action, value);
        } else if (key == QLatin1String("toggle-state")) {
            updateActionChecked(action, value);
        } else if (key == QLatin1String("icon-name")) {
            updateActionIconByName(action, value);
        } else if (key == QLatin1String("icon-data")) {
            updateActionIconByData(action, value);
        } else if (key == QLatin1String("visible")) {
            updateActionVisible(action, value);
        } else if (key == QLatin1String("shortcut")) {
            updateActionShortcut(action, value);
        } else {
//            qDebug() << "Unhandled property update" << key;
        }
    }

    void updateActionLabel(QAction *action, const QVariant &value) {
        QString text = swapMnemonicChar(value.toString(), '_', '&');
        action->setText(text);

        if(!action->menu())
        {
            QAction::MenuRole role = action->menuRole();
            if(role == QAction::TextHeuristicRole)
            {
                const QString txt = text.remove(QRegExp("\\(.*\\)|\\[.*\\]")).toLower().remove(title.toLower()).remove(QChar('.')).remove(QChar('&')).trimmed();
                //if(text.toLower().contains("about")) qInfo()<<"About : "<<txt<<", origin: "<<text;
                if(txt.length() < 12)
                {
                    if(txt.startsWith("退出") || txt.startsWith("quit") || txt.startsWith("exit"))
                    {
                        if(!txt.contains("playlist") && !txt.contains("列表") && !txt.contains("登录") && !txt.contains("login") && !txt.contains("account"))
                        {
                            role = QAction::QuitRole;
                        }
                    }
                    else if(txt == "关于" || txt == "about")
                    {
                        role = QAction::AboutRole;
                    }
                    else if(txt == "设置" || txt.startsWith("系统设置") || txt.startsWith("偏好设置") || txt.startsWith("常规设置") || txt.startsWith("首选项") || txt.startsWith("选项") || txt.startsWith("preferences") || txt.startsWith("options") || txt.startsWith("settings") || txt.startsWith("config") || txt.startsWith("setup"))
                    {
                        role = QAction::PreferencesRole;
                    }
                }
            }
            if(appRoles.contains(role))
            {
                action->setMenuRole(role);
                if(m_appMenus.contains(role))
                    m_appMenus.remove(role);
                m_appMenus.insert(role, action);
            }
        }
    }

    void updateActionEnabled(QAction *action, const QVariant &value) {
        action->setEnabled(value.isValid() ? value.toBool() : true);
    }

    void updateActionChecked(QAction *action, const QVariant &value) {
        if (action->isCheckable() && value.isValid()) {
            action->setChecked(value.toInt() == 1);
        }
    }

    void updateActionIconByName(QAction *action, const QVariant &value) {
        const QString iconName = value.toString();
        const QString previous = action->property(DBUSMENU_PROPERTY_ICON_NAME).toString();

        if (previous == iconName) {
            return;
        }

        action->setProperty(DBUSMENU_PROPERTY_ICON_NAME, iconName);

        if (iconName.isEmpty()) {
            action->setIcon(QIcon());
            return;
        }

        action->setIcon(QIcon::fromTheme(iconName));
    }

    void updateActionIconByData(QAction *action, const QVariant &value) {
        const QByteArray data = value.toByteArray();
        uint dataHash = qHash(data);
        uint previousDataHash = action->property(DBUSMENU_PROPERTY_ICON_DATA_HASH).toUInt();

        if (previousDataHash == dataHash) {
            return;
        }

        action->setProperty(DBUSMENU_PROPERTY_ICON_DATA_HASH, dataHash);
        QPixmap pix;

        if (!pix.loadFromData(data)) {
            qDebug() << "Failed to decode icon-data property for action" << action->text();
            action->setIcon(QIcon());
            return;
        }

        action->setIcon(QIcon(pix));
    }

    void updateActionVisible(QAction *action, const QVariant &value) {
        action->setVisible(value.isValid() ? value.toBool() : true);
    }

    void updateActionShortcut(QAction *action, const QVariant &value) {
        QDBusArgument arg = value.value<QDBusArgument>();
        DBusMenuShortcut dmShortcut;
        arg >> dmShortcut;
        QKeySequence keySequence = dmShortcut.toKeySequence();
        action->setShortcut(keySequence);
    }

    QMenu *menuForId(int id) const {
        if (id == 0) {
            return q->menu();
        }

        QAction *action = m_actionForId.value(id);

        if (!action) {
            return nullptr;
        }

        return action->menu();
    }

    void slotItemsPropertiesUpdated(const DBusMenuItemList &updatedList, const DBusMenuItemKeysList &removedList)
    {
        Q_FOREACH (const DBusMenuItem &item, updatedList) {
            if(QAction *action = m_actionForId.value(item.id)) {
                QVariantMap::ConstIterator
                it = item.properties.constBegin(),
                end = item.properties.constEnd();

                for (; it != end; ++it) {
                    updateActionProperty(action, it.key(), it.value());
                }
            }
        }

        Q_FOREACH (const DBusMenuItemKeys &item, removedList) {
            if(QAction *action = m_actionForId.value(item.id)) {
                Q_FOREACH (const QString &key, item.properties) {
                    updateActionProperty(action, key, QVariant());
                }
            }
        }
    }

    void sendEvent(int id, const QString &eventId) {
        // m_interface->Event(id, eventId, QDBusVariant(QString()), 0u);
        m_interface->asyncCall("Event", id, eventId, QVariant::fromValue(QDBusVariant(QString())), 0u);
    }

    bool waitForWatcher(QDBusPendingCallWatcher * _watcher, int maxWait)
    {
        QPointer<QDBusPendingCallWatcher> watcher(_watcher);

        if(m_type == ASYNCHRONOUS) {
            QTimer timer;
            timer.setSingleShot(true);
            QEventLoop loop;
            loop.connect(&timer, SIGNAL(timeout()), SLOT(quit()));
            loop.connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher *)), SLOT(quit()));
            timer.start(maxWait);
            loop.exec();
            timer.stop();

            if (!watcher) {
                // Watcher died. This can happen if importer got deleted while we were
                // waiting. See:
                // https://bugs.kde.org/show_bug.cgi?id=237156
                return false;
            }

            if(!watcher->isFinished()) {
                // Timed out
                return false;
            }
        } else {
            watcher->waitForFinished();
        }

        if (watcher->isError()) {
            qDebug() << watcher->error().message();
            return false;
        }

        return true;
    }

    void delAction(QMenu *menu, QAction *action)
    {
        if(QMenu *subMenu = action->menu())
        {
            for(auto *subAction : subMenu->actions())
                delAction(subMenu, subAction);
            subMenu->deleteLater();
        } else {
            switch (action->menuRole())
            {
            case QAction::AboutRole:
            case QAction::QuitRole:
            case QAction::PreferencesRole:
                if(m_appMenus.contains(action->menuRole()) && m_appMenus.value(action->menuRole()) == action)
                    m_appMenus.remove(action->menuRole());
            default:
                break;
            }
        }

        m_actionForId.remove(action->property(DBUSMENU_PROPERTY_ID).toInt());

        menu->removeAction(action);
        action->deleteLater();
    }
};

DBusMenuImporter::DBusMenuImporter(const QString &service, const QString &path, QObject *parent)
: DBusMenuImporter(service, path, ASYNCHRONOUS, parent)
{
}

DBusMenuImporter::DBusMenuImporter(const QString &service, const QString &path, DBusMenuImporterType type, QObject *parent)
    : QObject(parent)
    , d(new DBusMenuImporterPrivate)
{
    DBusMenuTypes_register();

    d->q = this;
    d->m_interface = new ComCanonicalDbusmenuInterface(service, path, QDBusConnection::sessionBus(), this);
    d->m_menu = nullptr;
    d->group = nullptr;
    d->title = QString();
    d->m_pendingLayoutUpdateTimer = new QTimer(this);
    d->m_pendingLayoutUpdateTimer->setSingleShot(true);
    d->m_pendingLayoutUpdateTimer->setInterval(500);
    connect(d->m_pendingLayoutUpdateTimer, &QTimer::timeout, [ this ]{
        QSet<int> ids = d->m_pendingLayoutUpdates;
        d->m_pendingLayoutUpdates.clear();
        if(ids.contains(0))
            d->refresh(0);
        else
            Q_FOREACH(int id, ids)
                d->refresh(id);
    });

    d->m_type = type;
}

DBusMenuImporter::~DBusMenuImporter()
{
    // Do not use "delete d->m_menu": even if we are being deleted we should
    // leave enough time for the menu to finish what it was doing, for example
    // if it was being displayed.
    if(d->m_menu)
    {
        d->m_menu->deleteLater();
        d->m_menu = nullptr;
    }
    delete d;
}

void DBusMenuImporter::setTitle(QString title) {
    d->title = title;
}

void DBusMenuImporter::slotLayoutUpdated(uint revision, int parentId)
{
    Q_UNUSED(revision)
    // qInfo()<<"Vscode layoutUpdated signal parentId: "<<parentId;
    d->m_pendingLayoutUpdates << parentId;
    d->m_pendingLayoutUpdateTimer->start();
}

QMenu *DBusMenuImporter::menu() const
{
    if (!d->m_menu) {
        d->m_menu = d->createMenu(nullptr);
    }

    return d->m_menu;
}

QAction *DBusMenuImporter::getAction(QAction::MenuRole role)
{
    return d->m_appMenus.value(role, nullptr);
}

void DBusMenuImporter::slotItemActivationRequested(int id, uint /*timestamp*/)
{
    if(QAction *action = d->m_actionForId.value(id))
        actionActivationRequested(action);
}

void DBusMenuImporter::slotGetLayoutFinished(QDBusPendingCallWatcher *watcher)
{
    const int parentId = watcher->property(DBUSMENU_PROPERTY_ID).toInt();
    watcher->deleteLater();

    QDBusPendingReply<uint, DBusMenuLayoutItem> reply = *watcher;

    if (!reply.isValid())
        return;

    DBusMenuLayoutItem rootItem = reply.argumentAt<1>();

    QMenu *menu = d->menuForId(parentId);
    if (!menu)
        return;

    if(!menu->actions().isEmpty())
    {
        QSet<int> newDBusMenuItemIds;
        newDBusMenuItemIds.reserve(rootItem.children.count());

        for (const DBusMenuLayoutItem &item : rootItem.children) {
            newDBusMenuItemIds << item.id;
        }

        int index=0;
        while (index < menu->actions().count()) {
            QAction *action = menu->actions().at(index);
            int id = action->property(DBUSMENU_PROPERTY_ID).toInt();

            if (! newDBusMenuItemIds.contains(id)) {
                // d->delAction(menu, action);
                menu->removeAction(action);
                action->deleteLater();
            }
            else
            {
                index++;
            }
        }
    }

    //insert or update new actions into our menu
    d->group = nullptr;
    int pos=-1;
    for (const DBusMenuLayoutItem &dbusMenuItem : rootItem.children) {
        pos++;
        QAction *action = nullptr;

        const int id = dbusMenuItem.id;
        if (!d->m_actionForId.contains(id))
        {
            action = d->createAction(id, dbusMenuItem.properties, menu);
            d->m_actionForId.insert(id, action);

            if(pos < menu->actions().count())
                menu->insertAction(menu->actions().at(pos), action);
            else
                menu->addAction(action);

            if (action->menu())
                slotMenuAboutToShow(action->menu());
        } else {
            action = d->m_actionForId[id];
            QStringList filteredKeys = dbusMenuItem.properties.keys();
            filteredKeys.removeOne("type");
            filteredKeys.removeOne("toggle-type");
            filteredKeys.removeOne("children-display");
            d->updateAction(action, dbusMenuItem.properties, filteredKeys);
            if(QMenu *subMenu = action->menu())
                d->refresh(id);
        }
    }
    d->group = nullptr;
}

void DBusMenuImporter::updateMenu()
{
    if(!d->m_menu) {
        slotMenuAboutToShow(menu());

        connect(d->m_interface, &ComCanonicalDbusmenuInterface::LayoutUpdated, this, &DBusMenuImporter::slotLayoutUpdated);
        connect(d->m_interface, &ComCanonicalDbusmenuInterface::ItemActivationRequested, this, &DBusMenuImporter::slotItemActivationRequested);
        connect(d->m_interface, &ComCanonicalDbusmenuInterface::ItemsPropertiesUpdated, this, [ = ](const DBusMenuItemList itemList, const DBusMenuItemKeysList keyList) {
            d->slotItemsPropertiesUpdated(itemList, keyList);
        });
    }

    emit menuUpdated();
}

void DBusMenuImporter::slotMenuAboutToShow(QMenu *menu)
{
    if(!menu)
        menu = qobject_cast<QMenu *>(sender());

    Q_ASSERT(menu);

    QAction *action = menu->menuAction();
    Q_ASSERT(action);

    int id = action->property(DBUSMENU_PROPERTY_ID).toInt();

    // auto call = d->m_interface->AboutToShow(id);
    auto call = d->m_interface->asyncCall("AboutToShow", id);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
    watcher->setProperty(DBUSMENU_PROPERTY_ID, id);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &DBusMenuImporter::slotAboutToShowDBusCallFinished);

    // QPointer<QObject> guard(this);

    // if (!d->waitForWatcher(watcher, ABOUT_TO_SHOW_TIMEOUT)) {
    //     qDebug() << "Application did not answer to AboutToShow() before timeout";
    // }

    // // "this" got deleted during the call to waitForWatcher(), get out
    // if (!guard) {
    //     return;
    // }

    // Firefox deliberately ignores "aboutToShow" whereas Qt ignores" opened", so we'll just send both all the time...
    d->sendEvent(id, QStringLiteral("opened"));
}

void DBusMenuImporter::slotAboutToShowDBusCallFinished(QDBusPendingCallWatcher *watcher)
{
    int id = watcher->property(DBUSMENU_PROPERTY_ID).toInt();
    watcher->deleteLater();

    QMenu *menu = d->menuForId(id);

    if (!menu) {
        return;
    }

    QDBusPendingReply<bool> reply = *watcher;

    if (reply.isError()) {
        qDebug() << "Call to AboutToShow() failed:" << reply.error().message();
        return;
    }

    //Note, this isn't used by Qt's QPT - but we get a LayoutChanged emitted before
    //this returns, which equates to the same thing
    bool needRefresh = reply.argumentAt<0>();
    // qInfo()<<"AboutToShow finish with menu id: "<<id<<", need refresh: "<<needRefresh<<", child count: "<<menu->actions().count();
    if (needRefresh || menu->actions().isEmpty()) {
        d->refresh(id);
    }
}

void DBusMenuImporter::slotMenuAboutToHide()
{
    QMenu *menu = qobject_cast<QMenu*>(sender());
    Q_ASSERT(menu);

    QAction *action = menu->menuAction();
    Q_ASSERT(action);

    int id = action->property(DBUSMENU_PROPERTY_ID).toInt();
    d->sendEvent(id, QString("closed"));
}

#include "moc_dbusmenuimporter.cpp"
