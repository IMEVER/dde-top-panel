//
// Created by septemberhx on 2020/5/26.
//

#ifndef DDE_TOP_PANEL_ACTIVEWINDOWCONTROLWIDGET_H
#define DDE_TOP_PANEL_ACTIVEWINDOWCONTROLWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <DGuiApplicationHelper>
#include <QMenuBar>
#include <QWidgetAction>
#include <com_deepin_wm.h>
#include <NETWM>

class ActiveWindowControlWidgetPrivate;
class ActiveWindowControlWidget : public QWidget {
    Q_OBJECT

public:
    explicit ActiveWindowControlWidget(QWidget *parent = 0);

public slots:
    void toggleMenu(int id);

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:

    ActiveWindowControlWidgetPrivate *d_ptr;
    Q_DECLARE_PRIVATE(ActiveWindowControlWidget)
};

#endif //DDE_TOP_PANEL_ACTIVEWINDOWCONTROLWIDGET_H
