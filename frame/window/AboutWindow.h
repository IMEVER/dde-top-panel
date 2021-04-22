#ifndef ABOUTWINDOW_H
#define ABOUTWINDOW_H

#include <QDialog>
#include <QWidget>
#include <QTabWidget>
#include <KWindowInfo>

#include "../util/AppInfo.h"

class AboutWindow : public QDialog
{
    Q_OBJECT
public:
    AboutWindow(KWindowInfo kwin, QWidget *parent=0);
    ~AboutWindow();
    void initData(KWindowInfo kwin);

private:
    void initStatusWidget();
    void initPackageInfoWidget();
    void initFileListWidget();
    QWidget *createWidget(QString title, QString value);

private:
    QTabWidget *tabWidget;
    AppInfo appInfo;
};

#endif