#include "ShowDesktopWidget.h"

#include <QProcess>
#include <QSize>

ShowDesktopWidget::ShowDesktopWidget(QWidget *parent) : QWidget(parent)
{
    this->setToolTip("显示桌面");
    this->setMouseTracking(true);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

ShowDesktopWidget::~ShowDesktopWidget()
{
}

QSize ShowDesktopWidget::sizeHint() const
{
    return QSize(10, height());
}

void ShowDesktopWidget::enterEvent(QEvent *event)
{
    this->m_isHover = true;
    update();
}

void ShowDesktopWidget::leaveEvent(QEvent *event)
{
    this->m_isHover = false;
    update();
}

void ShowDesktopWidget::mousePressEvent(QMouseEvent *event)
{
    QProcess::startDetached("/usr/lib/deepin-daemon/desktop-toggle", {});
}

void ShowDesktopWidget::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);

    painter.setOpacity(1);
    painter.setPen(QPen(QColor(0,0,0, 50), 2));
    painter.drawRect(e->rect());
    painter.fillRect(e->rect(), QColor(255, 255, 255, m_isHover ? 50 : 20));
}