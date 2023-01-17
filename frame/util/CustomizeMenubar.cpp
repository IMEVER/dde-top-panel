#include "CustomizeMenubar.h"

#include <QStyleOptionMenuItem>
#include <QPainter>
#include <QPaintEvent>
#include <QDebug>
#include <QtWidgets/private/qmenubar_p.h>

QT_FORWARD_DECLARE_CLASS(QMenuBarPrivate)
CustomizeMenubar::CustomizeMenubar(QWidget *parent) : QMenuBar(parent)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
    // setContentsMargins(0, 0, 0, 0);
}

CustomizeMenubar::~CustomizeMenubar()
{}

/*!
  Appends a new QMenu with \a title to the menu bar. The menu bar
  takes ownership of the menu. Returns the new menu.
  \sa QWidget::addAction(), QMenu::menuAction()
*/
QMenu *CustomizeMenubar::addMenu(const QString &title)
{
    QMenu *menu = new QMenu(title);
    addAction(menu->menuAction());
    return menu;
}
/*!
  Appends a new QMenu with \a icon and \a title to the menu bar. The menu bar
  takes ownership of the menu. Returns the new menu.
  \sa QWidget::addAction(), QMenu::menuAction()
*/
QMenu *CustomizeMenubar::addMenu(const QIcon &icon, const QString &title)
{
    QMenu *menu = new QMenu(title);
    menu->setIcon(icon);
    addAction(menu->menuAction());
    return menu;
}


void CustomizeMenubar::paintEvent(QPaintEvent *e)
{
    QMenuBarPrivate *d = reinterpret_cast<QMenuBarPrivate *>(QMenuBar::d_ptr.data());
    static int firstWidth = -1;

    QPainter p(this);
    QRegion emptyArea(rect());

    // int hmargin = style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, 0, this);
    // QRect result = rect();
    // result.adjust(hmargin, 0, -hmargin, 0);

    // if (extVisible) {
    //     if (isRightToLeft())
    //         result.setLeft(result.left() + extension->sizeHint().width());
    //     else
    //         result.setWidth(result.width() - extension->sizeHint().width());
    // }

    //draw the items
    bool needFix = false;// d->actionRect(d->actions.at(0));
    for (int i = 0; i < d->actions.count(); ++i) {
        QAction *action = d->actions.at(i);
        QRect &adjustedActionRect = d->actionRects[i]; // actionGeometry(action);

        if (adjustedActionRect.isEmpty() || !action->isVisible() || d->hiddenActions.contains(action))
            continue;

        if(i == 0)
        {
            if(firstWidth == -1)
                firstWidth = adjustedActionRect.width() + 10;

            if(firstWidth != adjustedActionRect.width()) {
                adjustedActionRect.setWidth(firstWidth);
                needFix = true;
            }
        }
        else if((i ==1 || i==2 || i==3) && needFix)
            adjustedActionRect.moveLeft(adjustedActionRect.x() + 10);
        else if(i == 4 && needFix) {
            adjustedActionRect.setWidth(adjustedActionRect.width() - 10);
            adjustedActionRect.moveLeft(adjustedActionRect.x() + 10);
        }

        // if (adjustedActionRect.isValid() && !result.contains(adjustedActionRect))
        //     continue;

        if(!e->rect().intersects(adjustedActionRect))
            continue;

        emptyArea -= adjustedActionRect;
        QStyleOptionMenuItem opt;
        initStyleOption(&opt, action);
        opt.rect = adjustedActionRect;
        p.setClipRect(adjustedActionRect);

        QFont font;

        if(action->objectName() == "appMenu")
        {
            font = p.font();
            font.setBold(true);
            p.setFont(font);

            opt.text = opt.text.trimmed();
        }
        style()->drawControl(QStyle::CE_MenuBarItem, &opt, &p, this);

        if(action->objectName() == "appMenu")
        {
            font.setBold(false);
            p.setFont(font);
        }
    }

     //draw border
    if(int fw = style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, 0, this)) {
        QRegion borderReg;
        borderReg += QRect(0, 0, fw, height()); //left
        borderReg += QRect(width()-fw, 0, fw, height()); //right
        borderReg += QRect(0, 0, width(), fw); //top
        borderReg += QRect(0, height()-fw, width(), fw); //bottom
        p.setClipRegion(borderReg);
        emptyArea -= borderReg;
        QStyleOptionFrame frame;
        frame.rect = rect();
        frame.palette = palette();
        frame.state = QStyle::State_None;
        frame.lineWidth = style()->pixelMetric(QStyle::PM_MenuBarPanelWidth);
        frame.midLineWidth = 0;
        style()->drawPrimitive(QStyle::PE_PanelMenuBar, &frame, &p, this);
    }
    p.setClipRegion(emptyArea);
    QStyleOptionMenuItem menuOpt;
    menuOpt.palette = palette();
    menuOpt.state = QStyle::State_None;
    menuOpt.menuItemType = QStyleOptionMenuItem::EmptyArea;
    menuOpt.checkType = QStyleOptionMenuItem::NotCheckable;
    menuOpt.rect = rect();
    menuOpt.menuRect = rect();
    style()->drawControl(QStyle::CE_MenuBarEmptyArea, &menuOpt, &p, this);
}
