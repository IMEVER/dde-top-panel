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

#ifndef PLUGINSITEM_H
#define PLUGINSITEM_H

#include "dockitem.h"
#include "pluginsiteminterface.h"

class PluginsItem : public DockItem
{
    Q_OBJECT

public:
    explicit PluginsItem(PluginsItemInterface *const pluginInter, const QString &itemKey, const QString &api, QWidget *parent = nullptr);
    ~PluginsItem() override;

    int itemSortKey() const;
    void setItemSortKey(const int order) const;
    void detachPluginWidget();

    void setInContainer(const bool container);

    QString pluginName() const;

    using DockItem::showContextMenu;
    using DockItem::hidePopup;

    ItemType itemType() const override;
    PluginsItemInterface::PluginSizePolicy pluginSizePolicy() const;
    QSize sizeHint() const override;

    QWidget *centralWidget() const;

    void setDraging(bool bDrag) override;

public slots:
    void refershIcon() override;

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

    void invokedMenuItem(const QString &itemId, const bool checked) override;
    const QString contextMenu() const override;
    QWidget *popupTips() override;
    QWidget *popupApplet() override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void mouseClicked();
    int comparePluginApi(QString pluginApi1, QString pluginApi2) const;

private:
    PluginsItemInterface *const m_pluginInter;
    QWidget *m_centralWidget;

    const QString m_itemKey;
    const QString m_api;

    static QPoint MousePressPoint;
};

#endif // PLUGINSITEM_H
