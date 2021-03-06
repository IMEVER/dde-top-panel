/*
 * Copyright (C) 2011 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     sbw <sbw@sbw.so>
 *
 * Maintainer: sbw <sbw@sbw.so>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "diskpluginitem.h"

#include <QPainter>
#include <QDebug>
#include <QMouseEvent>
#include <QIcon>

DiskPluginItem::DiskPluginItem(QWidget *parent)
    : QWidget(parent)
{
//    QIcon::setThemeName("deepin");
}

void DiskPluginItem::paintEvent(QPaintEvent *e)
{
    QWidget::paintEvent(e);

    QPainter painter(this);
    const QRectF &rf = QRectF(rect());
    const QRectF &rfp = QRectF(m_icon.rect());
    painter.drawPixmap(rf.center() - rfp.center(), m_icon);
}

void DiskPluginItem::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);

    updateIcon();
}
/*
void DiskPluginItem::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::RightButton)
        return QWidget::mousePressEvent(e);

    const QPoint p(e->pos() - rect().center());
    if (p.manhattanLength() < std::min(width(), height()) * 0.8 * 0.5)
    {
        emit requestContextMenu();
        return;
    }

    QWidget::mousePressEvent(e);
}
QSize DiskPluginItem::sizeHint() const
{
    return QSize(26, 26);
}
*/

void DiskPluginItem::updateIcon()
{
//        m_icon = ImageUtil::loadSvg(":/icons/resources/icon.svg", std::min(width(), height()) * 0.8);
        m_icon = QIcon::fromTheme("drive-removable-dock").pixmap(std::min(width(), height()) * 0.8, std::min(width(), height()) * 0.8);

    update();
}
