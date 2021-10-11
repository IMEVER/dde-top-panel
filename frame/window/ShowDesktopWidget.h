#ifndef SHOWDESKTOPWIDGET_H
#define SHOWDESKTOPWIDGET_H

#include <QWidget>
#include <QPaintEvent>
#include <QPainter>
#include <QTimer>

class ShowDesktopWidget : public QWidget
{
    Q_OBJECT

private:
    bool m_isHover = false;
    QTimer *timer;

private:
    void toggleDesktop();

public:
    explicit ShowDesktopWidget(QWidget *parent=0);
    ~ShowDesktopWidget();
    QSize sizeHint() const override;
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *e) override;
};

#endif