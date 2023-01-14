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
        QString name;
        QStringList desc;
        for(QString info : infos)
        {
            if (!info.at(0).isSpace() && info.contains(":"))
            {
                int index = info.indexOf(":");
                QString key = info.mid(0, index);
                QString value = info.mid(index+1);
                if(key == "Conffiles") {
                    if(!name.isEmpty()) {
                        data << QPair<QString, QString>(name, desc.join("\n"));
                        desc.clear();
                    }
                    name = "Conffiles";
                    desc.append(value.trimmed());
                } else if(key == "Description") {
                    if(!name.isEmpty()) {
                        data << QPair<QString, QString>(name, desc.join("\n"));
                        desc.clear();
                    }
                    name = "Description";
                    desc.append(value.trimmed());
                }
                else
                    data.append(QPair<QString, QString>(key, value.trimmed()));
            }
            else
                desc.append(info.trimmed());
        }
        if(!desc.isEmpty())
            data << QPair<QString, QString>(name, desc.join("\n"));
    }

    int count() const
    {
        return data.count();
    }

    QVector<QPair<QString, QString>> data;
};

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

#endif