//
// Created by septemberhx on 2020/6/2.
//

#include "QClickableLabel.h"
#include <QMouseEvent>

QClickableLabel::QClickableLabel(QWidget *parent)
    : QLabel(parent)
{
    // QPalette palette = this->palette();
    // palette.setColor(QPalette::WindowText, Qt::black);
    // palette.setColor(QPalette::Background, Qt::transparent);
    // this->setPalette(palette);
    this->setMouseTracking(true);
    this->setAutoFillBackground(true);

    this->hoverHighlightEffect = new HoverHighlightEffect(this);
    this->setGraphicsEffect(this->hoverHighlightEffect);
}

QClickableLabel::~QClickableLabel()
{
    delete hoverHighlightEffect;
}

void QClickableLabel::enterEvent(QEvent *event)
{
    this->hoverHighlightEffect->setHighlighting(true);
}

void QClickableLabel::leaveEvent(QEvent *event)
{
    this->hoverHighlightEffect->setHighlighting(false);
}

void QClickableLabel::mousePressEvent(QMouseEvent *ev) {
    if (ev->button() != Qt::LeftButton) {
        return;
    }

    Q_EMIT clicked();
    QLabel::mousePressEvent(ev);
}

void QClickableLabel::setDefaultFontColor(const QColor &defaultFontColor) {
    this->defaultFontColor = defaultFontColor;
    QPalette palette = this->palette();
    palette.setColor(QPalette::WindowText, this->defaultFontColor);
    this->setPalette(palette);
    this->repaint();
}

void QClickableLabel::setSelectedColor() {
    QPalette palette = this->palette();
    palette.setColor(QPalette::Background, QColor("#0081FF"));
    palette.setColor(QPalette::WindowText, Qt::white);
    this->setPalette(palette);
    this->repaint();
}

void QClickableLabel::setNormalColor() {
    QPalette palette = this->palette();
    palette.setColor(QPalette::Background, Qt::transparent);
    palette.setColor(QPalette::WindowText, this->defaultFontColor);
    this->setPalette(palette);
    this->repaint();
}
