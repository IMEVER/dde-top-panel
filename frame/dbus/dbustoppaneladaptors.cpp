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

#include "dbustoppaneladaptors.h"
#include <QScreen>
#include <QDebug>
#include <QApplication>

DBusTopPanelAdaptors::DBusTopPanelAdaptors(MainWindow* parent): QDBusAbstractAdaptor(parent)
{
}

DBusTopPanelAdaptors::~DBusTopPanelAdaptors()
{

}

MainWindow *DBusTopPanelAdaptors::parent() const
{
    return static_cast<MainWindow *>(QObject::parent());
}

void DBusTopPanelAdaptors::ActivateWindow()
{
    parent()->showOverFullscreen();
}

bool DBusTopPanelAdaptors::Activate()
{
    qDebug()<<"Activate";
    return true;
}

void DBusTopPanelAdaptors::ToggleStartMenu()
{
    parent()->toggleStartMenu();
}

void DBusTopPanelAdaptors::ToggleMenu()
{
    parent()->toggleMenu();
}