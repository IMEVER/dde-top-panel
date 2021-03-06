#include "AboutWindow.h"

#include <QTabWidget>
#include <QTableView>
#include <QListWidget>
#include <QGridLayout>
#include <QProcess>
#include <QAbstractItemModel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QSizePolicy>
#include <QFile>
#include <QFileInfo>

#include "../util/desktop_entry_stat.h"


class PackageModel : public QAbstractTableModel
{
    Q_OBJECT
private:
    Package m_package;
public:
    PackageModel(Package package, QObject *parent = nullptr){
        m_package = package;
    }
    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return m_package.count();
    }
    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return 2;
    }
    // QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
    // {
    //     if (Qt::SizeHintRole == role && Qt::Vertical)
    //     {
    //         return 1;
    //     }
    // }

    QString formatByteSize(int size) const
    {
        if(size < 1024)
            return QString("%1 KB").arg(size);
        else if(size > 1024 * 1024)
            return QString("%1 GB").arg(QString::number(size / 1024.0 / 1024.0, 'f', 2));
        else
            return QString("%1 MB").arg(QString::number(size / 1024.0, 'f', 2));
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        const int row = index.row();
        const int column = index.column();
        switch(role){
            case Qt::DisplayRole:
                if (row < m_package.count() && column < 2)
                {
                    QPair<QString, QString> pair = m_package.data.at(row);
                    if(column == 0)
                        return translate(pair.first);
                    else if(pair.first == "Installed-Size")
                        return formatByteSize(pair.second.toInt());
                    else
                        return QStringList({"Depends", "Pre-Depends", "Build-Depends", "Replaces", "Breaks" , "Conflicts", "Suggests", "Provides", "Recommends"}).contains(pair.first) ? pair.second.replace(",", "\n") : pair.second.trimmed();
                }
                break;
            case Qt::FontRole:
                if (column == 0)
                {
                    QFont font;
                    font.setBold(true);
                    return font;
                }
                break;
            // case Qt::ForegroundRole:
                // return column == 1 && m_package.data.at(row).first == "Homepage" ? QColor(Qt::blue) : QPalette::Foreground;
            case Qt::TextAlignmentRole:
                return column == 0 ? Qt::AlignCenter : QVariant(Qt::AlignLeft|Qt::AlignVCenter);
            // case Qt::CheckStateRole:
                // break;
        }
        return QVariant();
    }

    QVariant translate(QString field) const
    {
        if(field == "Package")
            return "??????";
        else if(field == "Status")
            return "??????";
        else if(field == "Priority")
            return "?????????";
        else if(field == "Section")
            return "??????";
        else if(field == "Installed-Size")
            return "????????????";
        else if(field == "Download-Size")
            return "????????????";
        else if(field == "Maintainer")
            return "?????????";
        else if(field == "Architecture")
            return "??????";
        else if(field == "Version")
            return "??????";
        else if(field == "Depends")
            return "??????";
        else if(field == "Description")
            return "??????";
        else if(field == "Homepage")
            return "??????";
        else if(field == "Replaces")
            return "??????";
        else if(field == "Provides")
            return "??????";
        else if(field == "Conflicts")
            return "??????";
        else if(field == "Recommends")
            return "??????";
        else if(field == "Breaks")
            return "??????";
        else if(field == "Suggests")
            return "??????";
        else if(field == "Source")
            return "??????";
        else if(field == "Multi-Arch")
            return "?????????";
        else if(field == "License")
            return "??????";
        else if(field == "Vendor")
            return "?????????";
        else if(field == "Build-Depends")
            return "????????????";
        else if(field == "Standards-Version")
            return "????????????";
        else if(field == "Pre-Depends")
            return "????????????";
        else
            return field;
    }
};


AboutWindow::AboutWindow(KWindowInfo kwin, QWidget *parent) : QDialog(parent)
    , tabWidget(new QTabWidget(this))
{
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->addWidget(tabWidget);

    initData(kwin);

    setAttribute(Qt::WA_ShowModal, true);
    setFixedSize(640, 480);
    setWindowFlags(windowFlags() & ~Qt::WindowMinMaxButtonsHint);
    setAttribute(Qt::WA_QuitOnClose, false);
    setWindowTitle(QString("?????? - ").append(appInfo.m_title));

    tabWidget->addTab(createStatusWidget(), "??????");
    tabWidget->addTab(createPackageInfoWidget(), "?????????");
    tabWidget->addTab(createFileListWidget(), "????????????");
}

AboutWindow::~AboutWindow()
{
}

QWidget* AboutWindow::createStatusWidget()
{
    QWidget *statusWidget = new QWidget(this);//statusWidget->setFixedSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    QVBoxLayout * vbox = new QVBoxLayout(statusWidget);//vbox->setContentsMargins(2, 2, 2, 2);

    vbox->addWidget(createWidget("??????", appInfo.m_title));
    vbox->addWidget(createWidget("??????", appInfo.m_packageName));
    vbox->addWidget(createWidget("??????", appInfo.m_desktopFile));
    vbox->addWidget(createWidget("??????", appInfo.m_className));
    vbox->addWidget(createWidget("??????", appInfo.m_cmdline));
    vbox->addWidget(createWidget("??????", QString::number(appInfo.m_desktop)));
    vbox->addWidget(createWidget("??????", QString::number(appInfo.m_pid)));

    return statusWidget;
}

QWidget *AboutWindow::createWidget(QString name, QString value)
{
    QWidget *widget = new QWidget(this);
    QHBoxLayout *hbox = new QHBoxLayout(widget);

    widget->setFixedSize(QWIDGETSIZE_MAX, 30);

    QLabel *title = new QLabel(name, this);
    title->setFixedWidth(100);
    // title->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    title->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

    title->setStyleSheet("font-weight: bold;");
    hbox->addWidget(title);

    QLabel *content = new QLabel(value.trimmed(), this);
    // content->setWordWrap(true);
    // content->setFixedWidth(500);
    // content->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    content->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    hbox->addWidget(content);

    return widget;
}

QWidget* AboutWindow::createPackageInfoWidget()
{
    QTableView *packageInfoWidget = new QTableView(this);
    // packageInfoWidget->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    packageInfoWidget->setColumnWidth(0, 100);
    packageInfoWidget->horizontalHeader()->setStretchLastSection(true);
    packageInfoWidget->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    // packageInfoWidget->verticalHeader()->setDefaultSectionSize(1);
    packageInfoWidget->setWordWrap(true);
    // packageInfoWidget->setTextElideMode(Qt::ElideMiddle);
    packageInfoWidget->setModel(new PackageModel(appInfo.m_package, this));
    // packageInfoWidget->resizeRowsToContents();

    packageInfoWidget->verticalHeader()->hide();
    packageInfoWidget->horizontalHeader()->hide();

    packageInfoWidget->setStyleSheet("QTableView::item {padding-top: 10px; padding-bottom: 10px;}");

    return packageInfoWidget;
}

QWidget* AboutWindow::createFileListWidget()
{
    QListWidget *fileListWidget = new QListWidget(this);
    fileListWidget->addItems(appInfo.m_fileList);

    return fileListWidget;
}

void AboutWindow::initData(KWindowInfo kwin)
{
    appInfo.m_title = kwin.name();
    appInfo.m_desktopFile = kwin.desktopFileName();
    appInfo.m_className = kwin.windowClassName();
    appInfo.m_desktop = kwin.desktop();
    appInfo.m_pid = kwin.pid();

    if (appInfo.m_className == "dde-desktop")
    {
        return;
    }

    if (appInfo.m_title.contains(QRegExp("[??????-]")))
    {
        appInfo.m_title = appInfo.m_title.split(QRegExp("[??????-]")).last().trimmed();
    }

    auto getExecFile = [](QString cmdline){
        cmdline = cmdline.replace("\"", "");
        QStringList fields = cmdline.split(" ", Qt::SkipEmptyParts);

        for (QString field : fields)
        {
            if(field == "bash" || field == "sh" || field == "python" || field == "gksu" || field == "pkexec" || field == "env" || field.contains("="))
                continue;
            else
                return field;
        }

        return QString();
    };

    DesktopEntry entry = DesktopEntryStat::instance()->getDesktopEntryByName(kwin.windowClassName());

    if(!entry)
    {
        entry = DesktopEntryStat::instance()->getDesktopEntryByName(appInfo.m_title);
    }

    if (!entry)
    {
        entry = DesktopEntryStat::instance()->getDesktopEntryByPid(kwin.pid());
    }

    if (entry)
    {
        appInfo.m_title = entry->displayName;
        appInfo.m_cmdline = entry->exec.join(" ");
        if (kwin.desktopFileName().isEmpty())
        {
            appInfo.m_desktopFile = entry->desktopFile;
        }
        QString searchPath;
        if(!appInfo.m_desktopFile.isEmpty() && !appInfo.m_desktopFile.contains(".local/share/applications/"))
        {
            QFileInfo file(appInfo.m_desktopFile);
            if(file.exists())
            {
                if(file.isSymbolicLink())
                    searchPath = file.symLinkTarget();
                else
                    searchPath = appInfo.m_desktopFile;
            }
        }
        if(searchPath.isEmpty())
            searchPath = getExecFile(appInfo.m_cmdline);

        initPackageInfo(searchPath);
    }
    else
    {
        QFile cmdFile(QString("/proc/%1/cmdline").arg(appInfo.m_pid));
        if(cmdFile.isReadable())
        {
            appInfo.m_cmdline = cmdFile.readAll();
            initPackageInfo(getExecFile(appInfo.m_cmdline));
        }
    }
}

void AboutWindow::initPackageInfo(QString cmdline)
{
    if(cmdline.isEmpty())
        return;

    QProcess *process = new QProcess(this);
        process->start("dpkg", QStringList()<<"-S"<<cmdline);
        if(process->waitForStarted())
        {
            if (process->waitForFinished())
            {
                QByteArray reply = process->readLine();
                if (reply.isEmpty() == false)
                {
                    QString packageName = QString(reply).split(":").first();
                    appInfo.m_packageName = packageName;
                    process->close();
                    process->start("dpkg", {"--status", packageName});
                    if (process->waitForStarted())
                    {
                        if (process->waitForFinished())
                        {
                            reply = process->readAll();
                            if (!reply.isEmpty())
                            {
                                appInfo.m_package = Package(reply);
                            }
                        }
                    }

                    process->close();
                    process->start("dpkg", {"--listfiles", packageName});
                    if (process->waitForStarted())
                    {
                        if (process->waitForFinished())
                        {
                            reply = process->readAll();
                            if (!reply.isEmpty())
                            {
                                appInfo.m_fileList = QString(reply).split("\n");
                            }
                        }
                    }
                }
            }
        }
        process->close();
        process->deleteLater();
}

void AboutWindow::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    QTableView *view =  qobject_cast<QTableView *>(tabWidget->widget(1));
    view->resizeRowsToContents();
}

 #include "AboutWindow.moc"