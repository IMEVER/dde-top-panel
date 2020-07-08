/*
 * Copyright (C) 2016 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     sbw <sbw@sbw.so>
 *
 * Maintainer: sbw <sbw@sbw.so>
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

#ifndef DBUSTopPanelADAPTORS_H
#define DBUSTopPanelADAPTORS_H

#include <QtDBus/QtDBus>
#include "window/MainWindow.h"
class MainWindow;
/*
 * Adaptor class for interface com.deepin.dde.Dock
 */

class DBusTopPanelAdaptors: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.deepin.dde.TopPanel")
    Q_CLASSINFO("D-Bus Introspection", ""
                                       "  <interface name=\"com.deepin.dde.TopPanel\">\n"
                                       "  <method name=\"ActivateWindow\"></method>\n"
                                       "  <method name=\"Activate\">\n"
                                       "  <arg type=\"b\" direction=\"out\"></arg>\n"
                                       "  </method>\n"
                                       "  </interface>\n"
                                       "")

public:
    DBusTopPanelAdaptors(MainWindow *parent);
    virtual ~DBusTopPanelAdaptors();

    MainWindow *parent() const;

public Q_SLOTS: // PROPERTIES
    void ActivateWindow();
    bool Activate();
};

#endif //DBUSTopPanelADAPTORS
