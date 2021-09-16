#ifndef APPINFO_H
#define APPINFO_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QPair>

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
            if (info.contains(": "))
            {
                QStringList items = info.split(": ");
                if(items.at(0).trimmed().contains(" "))
                    desc.append(info);
                else
                    data.append(QPair<QString, QString>(items.at(0), items.at(1)));
            }
            else
            {
                desc.append(info);
            }
        }
        if(!desc.isEmpty())
            data << QPair<QString, QString>("Desc", desc);
    }

    int count() const
    {
        return data.count();
    }

    QVector<QPair<QString, QString>> data;
};

// using Package = struct Package;

struct AppInfo
{
    public:
    QString m_title;
    QString m_desktopFile;
    QString m_packageName;
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