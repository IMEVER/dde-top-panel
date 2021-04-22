#ifndef APPINFO_H
#define APPINFO_H

#include <QString>
#include <QStringList>
#include <QMap>

struct Package
{
    public:
    Package(){}
    Package(QString str)
    {
        QStringList infos = str.split("\n");
        QString desc;
        for(QString info : infos)
        {
            if (desc.isEmpty() && info.contains(": "))
            {
                QStringList items = info.split(": ");
                data.insert(items.at(0), items.at(1));
            }
            else
            {
                desc.append(info);
            }
        }
        data.insert("Desc", desc);
    }

    int count() const
    {
        return data.count();
    }

    QMap<QString, QString> data;
};

// using Package = struct Package;

struct AppInfo
{
    public:
    QString m_title;
    QString m_desktopFile;
    QString m_machine;
    QString m_cmdline;
    QString m_className;
    int m_desktop;
    int m_pid;
    struct Package m_package;
    QStringList m_fileList;
};

// Q_DECLARE_METATYPE(Package)
// Q_DECLARE_METATYPE(AppInfo)

#endif