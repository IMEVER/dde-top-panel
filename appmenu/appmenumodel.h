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

#include <QAbstractNativeEventFilter>
#include <QStringList>
#include <KWindowSystem>
#include <QPointer>
#include <QWidget>

class QMenu;
class QDBusServiceWatcher;
class KDBusMenuImporter;

class AppMenuModel : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

    // Q_PROPERTY(bool menuAvailable READ menuAvailable WRITE setMenuAvailable NOTIFY menuAvailableChanged)

public:
    explicit AppMenuModel(QObject *parent = nullptr);
    ~AppMenuModel() override;

    QMenu *menu() const;

    void updateApplicationMenu(const QString &serviceName, const QString &menuObjectPath);

    bool menuAvailable() const;
    void setWinId(const WId &id, bool isDesktop = false);

protected:
    bool nativeEventFilter(const QByteArray &eventType, void *message, long int *result) override;

private:
    void clearMenuImporter();

private Q_SLOTS:
    void onActiveWindowChanged();

signals:
    void requestActivateIndex(int index);
    void modelNeedsUpdate();
    void clearMenu();

private:
    WId m_winId = 0;
    QPointer<QMenu> m_menu;
    QDBusServiceWatcher *m_serviceWatcher;
    QString m_serviceName;
    QPointer<KDBusMenuImporter> m_importer;
};

#endif
