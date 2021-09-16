//
// Created by septemberhx on 2020/5/23.
//

#include "MainPanelControl.h"
#include <QApplication>
#include <QDrag>
#include "ShowDesktopWidget.h"

MainPanelControl::MainPanelControl(QWidget *parent)
    : QWidget(parent)
    , m_mainPanelLayout(new QBoxLayout(QBoxLayout::LeftToRight, this))
    , m_trayAreaWidget(new QWidget(this))
    , m_trayAreaLayout(new QBoxLayout(QBoxLayout::LeftToRight))
    , m_pluginAreaWidget(new QWidget(this))
    , m_pluginLayout(new QHBoxLayout())
    , m_fixedPluginWidget(new QWidget(this))
    , m_fixedPluginLayout(new QHBoxLayout())
    , activeWindowControlWidget(new ActiveWindowControlWidget(this))
{
    this->setFixedHeight(DEFAULT_HEIGHT);
    this->setMouseTracking(true);
    this->setAcceptDrops(true);

    this->init();

    QPalette palette = this->palette();
    palette.setColor(QPalette::Background, Qt::transparent);
    this->setPalette(palette);
}

void MainPanelControl::init()
{
    this->activeWindowControlWidget->setFixedHeight(DEFAULT_HEIGHT);

    this->m_mainPanelLayout->addWidget(this->activeWindowControlWidget);
    // this->m_mainPanelLayout->addStretch();
    this->m_mainPanelLayout->addWidget(this->m_trayAreaWidget);
    this->m_mainPanelLayout->addWidget(this->m_pluginAreaWidget);
    this->m_mainPanelLayout->addWidget(this->m_fixedPluginWidget);
    this->m_mainPanelLayout->addWidget(new ShowDesktopWidget(this));

    m_mainPanelLayout->setMargin(0);
    m_mainPanelLayout->setSpacing(5);

    // 托盘
    m_trayAreaWidget->setLayout(m_trayAreaLayout);
    m_trayAreaLayout->setSpacing(0);
    m_trayAreaLayout->setContentsMargins(0, 2, 0, 2);

    // 插件
    m_pluginAreaWidget->setLayout(m_pluginLayout);
    m_pluginAreaWidget->setAcceptDrops(true);
    m_pluginLayout->setMargin(0);
    m_pluginLayout->setSpacing(5);

    activeWindowControlWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_pluginAreaWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_trayAreaWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    m_fixedPluginWidget->setLayout(m_fixedPluginLayout);
    m_fixedPluginLayout->setMargin(0);
    m_fixedPluginLayout->setSpacing(5);
    m_fixedPluginWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    this->activeWindowControlWidget->activeWindowInfoChanged();
}

void MainPanelControl::insertItem(int index, DockItem *item)
{
    item->installEventFilter(this);

    switch (item->itemType()) {
        case DockItem::TrayPlugin:
            addTrayAreaItem(index, item);
            break;
        case DockItem::Plugins:
            addPluginAreaItem(index, item);
            break;
        case DockItem::FixedPlugin:
            // addPluginAreaItem(index, item);
            addFixedPluginAreaItem(index, item);
            break;
        default:
            break;
    }
}

void MainPanelControl::removeItem(DockItem *item)
{
    switch (item->itemType()) {
        case DockItem::TrayPlugin:
            removeTrayAreaItem(item);
            break;
        case DockItem::Plugins:
            removePluginAreaItem(item);
            break;
        case DockItem::FixedPlugin:
            removeFixedPluginAreaItem(item);
            break;
        default:
            break;
    }
}
void MainPanelControl::addPluginAreaItem(int index, QWidget *wdg) {
    PluginsItem *item = qobject_cast<PluginsItem *>(wdg);
    if(item->pluginSizePolicy() == PluginsItemInterface::Custom)
        wdg->setMaximumHeight(height());
    else
        wdg->setFixedSize(height() - 4, height() - 4);
    m_pluginLayout->insertWidget(index, wdg, 0, Qt::AlignCenter);
    m_pluginAreaWidget->adjustSize();
}
void MainPanelControl::removePluginAreaItem(QWidget *wdg)
{
    m_pluginLayout->removeWidget(wdg);
}

void MainPanelControl::itemUpdated(DockItem *item)
{
    item->parentWidget()->adjustSize();
}

// ----------> For tray area begin <----------
void MainPanelControl::addTrayAreaItem(int index, QWidget *wdg)
{
    m_tray = static_cast<TrayPluginItem *>(wdg);
    m_trayAreaLayout->insertWidget(index, wdg);
}

void MainPanelControl::removeTrayAreaItem(QWidget *wdg)
{
    m_trayAreaLayout->removeWidget(wdg);
}

void MainPanelControl::addFixedPluginAreaItem(int index, QWidget *wdg)
{
    PluginsItem *item = qobject_cast<PluginsItem *>(wdg);
    if(item->pluginSizePolicy() == PluginsItemInterface::Custom)
        wdg->setMaximumHeight(height());
    else
        wdg->setFixedSize(height() - 4, height() - 4);

    m_fixedPluginLayout->insertWidget(index, wdg, 0, Qt::AlignCenter);
    m_fixedPluginWidget->adjustSize();
}

void MainPanelControl::removeFixedPluginAreaItem(QWidget *wdg)
{
    m_fixedPluginLayout->removeWidget(wdg);
}

void MainPanelControl::toggleMenu(int id) {
    this->activeWindowControlWidget->toggleMenu(id);
}

void MainPanelControl::dragMoveEvent(QDragMoveEvent *e) {
    DockItem *sourceItem = qobject_cast<DockItem *>(e->source());
    if (sourceItem) {
        handleDragMove(e);
        return;
    }
}

void MainPanelControl::handleDragMove(QDragMoveEvent *e) {
    DockItem *sourceItem = qobject_cast<DockItem *>(e->source());

    if (!sourceItem) {
        e->ignore();
        return;
    }

    DockItem *targetItem = dropTargetItem(sourceItem, e->pos());

    if (!targetItem) {
        e->ignore();
        return;
    }

    e->accept();

    if (targetItem == sourceItem)
        return;

    moveItem(sourceItem, targetItem);
    emit itemMoved(sourceItem, targetItem);
}

DockItem *MainPanelControl::dropTargetItem(DockItem *sourceItem, QPoint point) {
    QWidget *parentWidget = m_pluginAreaWidget;

    point = parentWidget->mapFromParent(point);
    QLayout *parentLayout = parentWidget->layout();

    DockItem *targetItem = nullptr;

    for (int i = 0 ; i < parentLayout->count(); ++i) {
        QLayoutItem *layoutItem = parentLayout->itemAt(i);
        DockItem *dockItem = qobject_cast<DockItem *>(layoutItem->widget());
        if (!dockItem)
            continue;

        QRect rect;

        rect.setTopLeft(dockItem->pos());
        if (dockItem->itemType() == DockItem::Plugins) {
            rect.setSize(QSize(DOCK_MAX_SIZE, height()));
        } else {
            rect.setSize(dockItem->size());
        }
        if (rect.contains(point)) {
            targetItem = dockItem;
            break;
        }
    }

    return targetItem;
}

void MainPanelControl::moveItem(DockItem *sourceItem, DockItem *targetItem) {
    // get target index
    int idx = -1;
    if (targetItem->itemType() == DockItem::Plugins)
        idx = m_pluginLayout->indexOf(targetItem);
    else
        return;

    // remove old item
    removeItem(sourceItem);

    // insert new position
    insertItem(idx, sourceItem);
}

bool MainPanelControl::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() != QEvent::MouseMove)
        return false;

    QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
    if (!mouseEvent || mouseEvent->buttons() != Qt::LeftButton)
        return false;

    DockItem *item = qobject_cast<DockItem *>(watched);
    if (!item)
        return false;

    if (item->itemType() != DockItem::Plugins && item->itemType() != DockItem::FixedPlugin)
        return false;

    const QPoint pos = mouseEvent->globalPos();
    const QPoint distance = pos - m_mousePressPos;
    if (distance.manhattanLength() < QApplication::startDragDistance())
        return false;

    startDrag(item);

    return QWidget::eventFilter(watched, event);
}

void MainPanelControl::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        m_mousePressPos = e->globalPos();
    }

    QWidget::mousePressEvent(e);
}

void MainPanelControl::startDrag(DockItem *item) {
    const QPixmap pixmap = item->grab();

    item->setDraging(true);
    item->update();

    QDrag *drag = new QDrag(item);
    drag->setPixmap(pixmap);
    drag->setHotSpot(pixmap.rect().center() / pixmap.devicePixelRatioF());
    drag->setMimeData(new QMimeData);
    drag->exec(Qt::MoveAction);

    item->setDraging(false);
    item->update();
}

void MainPanelControl::dragEnterEvent(QDragEnterEvent *e) {
    QRect rect = QRect();
    if (!rect.contains(e->pos()))
        e->accept();
}

void MainPanelControl::dragLeaveEvent(QDragLeaveEvent *event) {
    QWidget::dragLeaveEvent(event);
}

void MainPanelControl::applyCustomSettings(const CustomSettings &customSettings) {
}
