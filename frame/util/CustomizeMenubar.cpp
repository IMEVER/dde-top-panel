#include "CustomizeMenubar.h"

#include <QStyleOptionMenuItem>
#include <QPainter>
#include <QPaintEvent>
#include <QDebug>
#include <QProxyStyle>
#include <QStyleOption>
#include <QPalette>
#include <QFontMetrics>
#include <QtWidgets/private/qmenubar_p.h>

class MenuProxyStyle: public QProxyStyle{
public:
    using QProxyStyle::QProxyStyle;
    int pixelMetric(PixelMetric m, const QStyleOption *option, const QWidget *widget) const override{
        if(m == PM_SmallIconSize) {
            // if(auto opt = qstyleoption_cast<const QStyleOptionMenuItem*>(option)) {
                // if(opt->text == "_开始_") {
                    // int ret = QProxyStyle::pixelMetric(m, option, widget);
                    // return ret + 2;
                //}
            // }
        } else if(m == QStyle::PM_MenuBarPanelWidth)
            return 0;
        else if(m == QStyle::PM_MenuBarItemSpacing)
            return 0;
        else if(m == QStyle::PM_MenuBarVMargin)
            return 0;
        else if(m == QStyle::PM_MenuBarHMargin)
            return 0;

        return QProxyStyle::pixelMetric(m, option, widget);
    }

    int styleHint(StyleHint stylehint, const QStyleOption *opt = nullptr, const QWidget *widget = nullptr, QStyleHintReturn* returnData = nullptr) const override {
        if(stylehint == QStyle::SH_FocusFrame_Mask) {
            return 0;
        }
        return QProxyStyle::styleHint(stylehint, opt, widget, returnData);
    }

    QSize sizeFromContents(QStyle::ContentsType ct, const QStyleOption * option, const QSize & contentsSize, const QWidget * w = nullptr) const override {
        QSize sz = QProxyStyle::sizeFromContents(ct, option, contentsSize, w);
        if(ct == QStyle::CT_MenuBarItem) {
            if(auto opt = qstyleoption_cast<const QStyleOptionMenuItem*>(option)) {
                if(opt->text == QString("_开始_"))
                    sz += QSize(14, 0);
                else if(!opt->icon.isNull())
                    sz += QSize(4, 0);
                else {
                    QFont font = opt->font;
                    font.setPixelSize(15);
                    font.setBold(opt->text.endsWith("__"));
                    QFontMetrics fm(font);
                    sz = fm.size(Qt::TextShowMnemonic, opt->text);
                    if(font.bold())
                        sz -= QSize(4, 0);
                    else
                        sz += QSize(10, 0);
                }
            }
        }

        sz.setHeight(24);
        return sz;
    }

    void drawControl(ControlElement element, const QStyleOption *option, QPainter *p, const QWidget *w) const override {
        if(element == QStyle::CE_MenuBarItem){
            if(auto opt = qstyleoption_cast<const QStyleOptionMenuItem *>(option)){
                if(opt->state & QStyle::State_Selected) {
                    p->save();
                    p->setBrush(opt->palette.color(QPalette::Highlight).lighter(90));
                    p->setPen(Qt::transparent);
                    p->drawRoundedRect(opt->rect, 4, 4, Qt::AbsoluteSize);
                    p->restore();
                }

                if(opt->font.bold()) {
                    QFont font = p->font();
                    font.setBold(true);
                    p->setFont(font);
                    QProxyStyle::drawControl(element, opt, p, w);
                    font.setBold(false);
                    p->setFont(font);
                    return;
                }
                // QCommonStyle::drawControl(element, opt, p, w);
                // return;
            }
        }
        QProxyStyle::drawControl(element, option, p, w);
    }
};

QT_FORWARD_DECLARE_CLASS(QMenuBarPrivate)
CustomizeMenubar::CustomizeMenubar(QWidget *parent) : QMenuBar(parent)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
    setStyle(new MenuProxyStyle(style()));
    setAttribute(Qt::WA_TranslucentBackground);
}

void CustomizeMenubar::setColor(QColor color) {
    if(m_color != color) {
        m_color = color;
        update();
    }
}

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

    QPainter p(this);
    QFont font = p.font();font.setPixelSize(15);p.setFont(font);
    QRegion emptyArea(rect());

    for (int i = 0; i < d->actions.count(); ++i) {
        QAction *action = d->actions.at(i);
        QRect adjustedActionRect = d->actionRects[i];

        if (adjustedActionRect.isEmpty() || d->hiddenActions.contains(action) || !e->rect().intersects(adjustedActionRect))
            continue;

        emptyArea -= adjustedActionRect;
        QStyleOptionMenuItem opt;
        initStyleOption(&opt, action);
        opt.palette.setColor(QPalette::Highlight, palette().color(QPalette::Highlight));
        opt.palette.setColor(QPalette::ButtonText, m_color);
        opt.rect = adjustedActionRect;
        // if(opt.state & QStyle::State_Selected) opt.palette.setColor(QPalette::Window, palette().color(QPalette::Window));
        p.setClipRect(adjustedActionRect);

        if(action->objectName() == "appMenu") {
            opt.font.setBold(true);
            opt.text.replace("__", "");
        }

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
    menuOpt.palette = palette();menuOpt.palette.setColor(QPalette::Window, QColor(0, 0, 0, 0));
    menuOpt.state = QStyle::State_None;
    menuOpt.menuItemType = QStyleOptionMenuItem::EmptyArea;
    menuOpt.checkType = QStyleOptionMenuItem::NotCheckable;
    menuOpt.rect = rect();
    menuOpt.menuRect = rect();
    style()->drawControl(QStyle::CE_MenuBarEmptyArea, &menuOpt, &p, this);
}
