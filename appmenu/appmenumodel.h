/******************************************************************
 * Copyright 2016 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>
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

#ifndef APPMENUMODEL_H
#define APPMENUMODEL_H

#include <QStringList>
#include <KWindowSystem>
#include <QPointer>
#include <QWidget>
#include <QMap>
#include <QMenu>
#include <QAction>

class QDBusServiceWatcher;
class KDBusMenuImporter;
// class RegistrarProxy;
class DBusRegistrar;

class AppMenuModel : public QObject
{
    Q_OBJECT

    // Q_PROPERTY(bool menuAvailable READ menuAvailable WRITE setMenuAvailable NOTIFY menuAvailableChanged)

public:
    explicit AppMenuModel(QObject *parent = nullptr);
    ~AppMenuModel() override;

    QMenu *menu() const;
    QAction * getAction(QAction::MenuRole role);
    void updateApplicationMenu(KDBusMenuImporter *importer);
    void setWinId(const WId &id, bool isDesktop = false);
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void clearMenuImporter();
    void initDesktopMenu();

private Q_SLOTS:
    void onActiveWindowChanged();

signals:
    void requestActivateIndex(int index);
    void modelNeedsUpdate();
    void clearMenu();
    void actionRemoved(QAction *action);
    void actionAdded(QAction *action, QAction *before);

private:
    WId m_winId = 0;
    QPointer<QMenu> m_menu;
    QPointer<QMenu> desktopMenu;
    QDBusServiceWatcher *m_serviceWatcher;
    KDBusMenuImporter *m_importer;

    QMap<WId, KDBusMenuImporter *> cachedImporter;

    // RegistrarProxy *registrarProxy;
    DBusRegistrar *dbusRegistrar;
};

#endif
