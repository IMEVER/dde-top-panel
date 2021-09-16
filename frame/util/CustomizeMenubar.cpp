#include "CustomizeMenubar.h"

#include <QStyleOptionMenuItem>
#include <QPainter>
#include <QPaintEvent>
#include <QDebug>

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
{//return QMenuBar::paintEvent(e);
    QPainter p(this);
    QRegion emptyArea(rect());

    int hmargin = style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, 0, this);
    QRect result = rect();
    result.adjust(hmargin, 0, -hmargin, 0);
    
    // if (extVisible) {
    //     if (isRightToLeft())
    //         result.setLeft(result.left() + extension->sizeHint().width());
    //     else
    //         result.setWidth(result.width() - extension->sizeHint().width());
    // }

    //draw the items
    for (int i = 0; i < actions().count(); ++i) {
        QAction *action = actions().at(i);
        QRect adjustedActionRect = actionGeometry(action);

        if (adjustedActionRect.isEmpty())
            continue;
        if (adjustedActionRect.isValid() && !result.contains(adjustedActionRect))
            continue;
        if(!e->rect().intersects(adjustedActionRect))
            continue;

        emptyArea -= adjustedActionRect;
        QStyleOptionMenuItem opt;
        initStyleOption(&opt, action);
        opt.rect = adjustedActionRect;
        p.setClipRect(adjustedActionRect);

        QFont font = p.font();
        opt.palette.setColor(QPalette::ButtonText, Qt::black);
        font.setBold(action->objectName() == "appMenu");
        p.setFont(font);
        style()->drawControl(QStyle::CE_MenuBarItem, &opt, &p, this);
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
