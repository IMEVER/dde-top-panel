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
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        const int row = index.row();
        const int column = index.column();
        switch(role){
            case Qt::DisplayRole:
                if (row < m_package.count() && column < 2)
                {
                    if(column == 0)
                        return translate(m_package.data.keys().at(row));
                    else
                        return m_package.data.values().at(row);
                }
                break;
            case Qt::FontRole:
                break;
            case Qt::BackgroundColorRole:
            case Qt::TextAlignmentRole:
                return column == 0 ? Qt::AlignHCenter : Qt::AlignLeft;
            case Qt::CheckStateRole:
                break;
        }
        return QVariant();
    }

    QVariant translate(QString field) const
    {
        if(field == "Package")
            return "包名";
        else if(field == "Status")
            return "状态";
        else if(field == "Priority")
            return "优先级";
        else if(field == "Section")
            return "分类";
        else if(field == "Installed-Size")
            return "安装大小";
        else if(field == "Maintainer")
            return "维护者";
        else if(field == "Architecture")
            return "架构";
        else if(field == "Version")
            return "版本";
        else if(field == "Depends")
            return "依赖";
        else if(field == "Description")
            return "描述";
        else if(field == "desc")
            return "详细描述";
        else if(field == "Homepage")
            return "主页";
        else if(field == "Replaces")
            return "代替";
        else if(field == "Provides")
            return "提供";
        else if(field == "Conflicts")
            return "冲突";
        else if(field == "Recommends")
            return "推荐";
        else if(field == "Breaks")
            return "破坏";
        else if(field == "Suggests")
            return "建议";
        else if(field == "Source")
            return "源码";
        else if(field == "Multi-Arch")
            return "多架构";
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

    initStatusWidget();
    initPackageInfoWidget();
    initFileListWidget();

    setAttribute(Qt::WA_ShowModal, true);
    setFixedSize(640, 480);
    setWindowFlags(windowFlags() & ~Qt::WindowMinMaxButtonsHint);
    setAttribute(Qt::WA_QuitOnClose, false);
    setWindowTitle(QString("关于--").append(appInfo.m_title));
}

AboutWindow::~AboutWindow()
{
    tabWidget->deleteLater();
}

void AboutWindow::initStatusWidget()
{
    QWidget *statusWidget = new QWidget(this);
    QVBoxLayout * vbox = new QVBoxLayout(statusWidget);
    vbox->addSpacing(2);
    tabWidget->addTab(statusWidget, "状态");

    vbox->addWidget(createWidget("标题", appInfo.m_title));
    vbox->addWidget(createWidget("文件", appInfo.m_desktopFile));
    vbox->addWidget(createWidget("机器", appInfo.m_machine));
    vbox->addWidget(createWidget("类名", appInfo.m_className));
    vbox->addWidget(createWidget("程序", appInfo.m_cmdline));
    vbox->addWidget(createWidget("桌面", QString::number(appInfo.m_desktop)));
    vbox->addWidget(createWidget("进程", QString::number(appInfo.m_pid)));
}

QWidget *AboutWindow::createWidget(QString name, QString value)
{
    QWidget *widget = new QWidget(this);
    QHBoxLayout *hbox = new QHBoxLayout(widget);

    widget->setFixedHeight(40);

    QLabel *title = new QLabel(name, this);
    title->setFixedWidth(100);
    title->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    title->setAlignment(Qt::AlignHCenter);

    title->setStyleSheet("font-weight: bold;");
    hbox->addWidget(title);

    QLabel *content = new QLabel(value, this);
    content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    content->setAlignment(Qt::AlignLeft);
    hbox->addWidget(content);

    return widget;
}

void AboutWindow::initPackageInfoWidget()
{
    QTableView *packageInfoWidget = new QTableView(this);
    tabWidget->addTab(packageInfoWidget, "包信息");
    packageInfoWidget->setModel(new PackageModel(appInfo.m_package, this));
    packageInfoWidget->resizeColumnToContents(1);
    packageInfoWidget->verticalHeader()->hide();
    packageInfoWidget->horizontalHeader()->hide();
}

void AboutWindow::initFileListWidget()
{
    QListWidget *fileListWidget = new QListWidget(this);
    tabWidget->addTab(fileListWidget, "文件列表");
    fileListWidget->addItems(appInfo.m_fileList);
}

void AboutWindow::initData(KWindowInfo kwin)
{
    appInfo.m_title = kwin.name();
    appInfo.m_desktopFile = kwin.desktopFileName();
    appInfo.m_machine = kwin.clientMachine();
    appInfo.m_className = kwin.windowClassName();
    appInfo.m_desktop = kwin.desktop();
    appInfo.m_pid = kwin.pid();

    DesktopEntry entry = DesktopEntryStat::instance()->getDesktopEntryByName(kwin.windowClassName());
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
        QProcess *process = new QProcess(this);
        process->start("dpkg", QStringList()<<"-S"<<entry->exec.first());
        if(process->waitForStarted())
        {
            if (process->waitForFinished())
            {
                QByteArray reply = process->readLine();
                if (reply.isEmpty() == false)
                {
                    QString packageName = QString(reply).split(":").first();
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
}

 #include "AboutWindow.moc"