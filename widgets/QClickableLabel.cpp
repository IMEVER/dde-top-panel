//
// Created by septemberhx on 2020/6/2.
//

#include "QClickableLabel.h"
#include <QDebug>

QClickableLabel::QClickableLabel(QWidget *parent)
    : QLabel(parent)
{
    // QPalette palette = this->palette();
    // palette.setColor(QPalette::WindowText, Qt::black);
    // palette.setColor(QPalette::Background, Qt::transparent);
    // this->setPalette(palette);
    this->setMouseTracking(true);
    this->setAutoFillBackground(true);
}

void QClickableLabel::enterEvent(QEvent *event) {
    QWidget::enterEvent(event);
}

void QClickableLabel::leaveEvent(QEvent *event) {
    this->repaint();

    QWidget::leaveEvent(event);
}

void QClickableLabel::mousePressEvent(QMouseEvent *ev) {
    Q_EMIT clicked();
    QLabel::mousePressEvent(ev);
}

void QClickableLabel::mouseReleaseEvent(QMouseEvent *ev) {
    QLabel::mouseReleaseEvent(ev);
}
