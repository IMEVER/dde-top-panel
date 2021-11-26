#include "CustomizeMenubar.h"

#include <QStyleOptionMenuItem>
#include <QPainter>
#include <QPaintEvent>
#include <QDebug>
#include <QtWidgets/private/qmenubar_p.h>

QT_FORWARD_DECLARE_CLASS(QMenuBarPrivate)
CustomizeMenubar::CustomizeMenubar(QWidget *parent) : QMenuBar(parent)
{
    // setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
    // setContentsMargins(0, 0, 0, 0);
    // setFixedHeight(24);

    // QFont f = font();
    // f.setPointSizeF(9.3);
    // setFont(f);

    // qInfo()<<"menubar height: "<<height()<<"font height: "<<fontMetrics().height();
}

CustomizeMenubar::~CustomizeMenubar()
{}

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

        if (adjustedActionRect.isEmpty() || !action->isVisible())
            continue;

        if(i == 0)
        {
            if(firstWidth != adjustedActionRect.width())
            {
                if(firstWidth == -1)
                    firstWidth = adjustedActionRect.width() + 12;

                adjustedActionRect.setWidth(firstWidth);
                needFix = true;
            }
        }
        else if(needFix)
        {
            adjustedActionRect.setWidth(adjustedActionRect.width() - 1);
            adjustedActionRect.moveLeft(adjustedActionRect.x() + 12 - (i - 1));
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
            // opt.palette.setColor(QPalette::ButtonText, Qt::black);
            font.setBold(true);
            p.setFont(font);
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
