/*
 * This file was generated by qdbusxml2cpp version 0.8
 * Command line was: qdbusxml2cpp -m -a menuimporteradaptor -c MenuImporterAdaptor -i menuimporter.h -l MenuImporter /data/code/c/dde-top-panel/appmenu/com.canonical.AppMenu.Registrar.xml
 *
 * qdbusxml2cpp is Copyright (C) 2017 The Qt Company Ltd.
 *
 * This is an auto-generated file.
 * This file may have been hand-edited. Look for HAND-EDIT comments
 * before re-generating it.
 */

#ifndef MENUIMPORTERADAPTOR_H
#define MENUIMPORTERADAPTOR_H

#include <QtCore/QObject>
#include <QtDBus/QtDBus>
#include "menuimporter.h"
QT_BEGIN_NAMESPACE
class QByteArray;
template<class T> class QList;
template<class Key, class Value> class QMap;
class QString;
class QStringList;
class QVariant;
QT_END_NAMESPACE

/*
 * Adaptor class for interface com.canonical.AppMenu.Registrar
 */
class MenuImporterAdaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.canonical.AppMenu.Registrar")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"com.canonical.AppMenu.Registrar\">\n"
"    <method name=\"RegisterWindow\">\n"
"      <arg direction=\"in\" type=\"u\" name=\"windowId\"/>\n"
"      <arg direction=\"in\" type=\"o\" name=\"menuObjectPath\"/>\n"
"    </method>\n"
"    <method name=\"UnregisterWindow\">\n"
"      <arg direction=\"in\" type=\"u\" name=\"windowId\"/>\n"
"    </method>\n"
"    <method name=\"GetMenuForWindow\">\n"
"      <arg direction=\"in\" type=\"u\" name=\"windowId\"/>\n"
"      <arg direction=\"out\" type=\"s\" name=\"service\"/>\n"
"      <arg direction=\"out\" type=\"o\" name=\"menuObjectPath\"/>\n"
"    </method>\n"
"  </interface>\n"
        "")
public:
    MenuImporterAdaptor(MenuImporter *parent);
    virtual ~MenuImporterAdaptor();

    inline MenuImporter *parent() const
    { return static_cast<MenuImporter *>(QObject::parent()); }

public: // PROPERTIES
public Q_SLOTS: // METHODS
    QString GetMenuForWindow(uint windowId, QDBusObjectPath &menuObjectPath);
    void RegisterWindow(uint windowId, const QDBusObjectPath &menuObjectPath);
    void UnregisterWindow(uint windowId);
Q_SIGNALS: // SIGNALS
};

#endif
