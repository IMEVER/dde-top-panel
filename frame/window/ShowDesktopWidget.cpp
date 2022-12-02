#include "ShowDesktopWidget.h"

#include <QProcess>
#include <QSize>

ShowDesktopWidget::ShowDesktopWidget(QWidget *parent) : QWidget(parent)
    , timer(new QTimer(this))
{
    this->setToolTip("显示桌面");
    this->setMouseTracking(true);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    timer->setSingleShot(true);
    timer->setInterval(1500);
    connect(timer, &QTimer::timeout, [ this ]{
        toggleDesktop();
     });
}

ShowDesktopWidget::~ShowDesktopWidget()
{
}

void ShowDesktopWidget::toggleDesktop()
{
    QProcess::startDetached("/usr/lib/deepin-daemon/desktop-toggle", {});
}

QSize ShowDesktopWidget::sizeHint() const
{
    return QSize(10, height());
}

void ShowDesktopWidget::enterEvent(QEvent *event)
{
    this->m_isHover = true;
    update();
    timer->start();
}

void ShowDesktopWidget::leaveEvent(QEvent *event)
{
    this->m_isHover = false;
    update();
    if(timer->isActive())
        timer->stop();
}

void ShowDesktopWidget::mousePressEvent(QMouseEvent *event)
{
    if(timer->isActive())
        timer->stop();

    toggleDesktop();
}

void ShowDesktopWidget::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);

    painter.setOpacity(1);
    painter.setPen(QPen(QColor(0,0,0, 50), 2));
    painter.drawRect(e->rect());
    painter.fillRect(e->rect(), QColor(255, 255, 255, m_isHover ? 50 : 20));
}