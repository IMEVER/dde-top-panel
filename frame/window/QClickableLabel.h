//
// Created by septemberhx on 2020/6/2.
//

#ifndef DDE_TOP_PANEL_QCLICKABLELABEL_H
#define DDE_TOP_PANEL_QCLICKABLELABEL_H


#include <QLabel>
#include "../util/CustomSettings.h"
#include "../item/hoverhighlighteffect.h"

class QClickableLabel : public QLabel {
    Q_OBJECT

public:
    explicit QClickableLabel(QWidget *parent);
    ~QClickableLabel();

    void setDefaultFontColor(const QColor &defaultFontColor);
    void setSelectedColor();
    void setNormalColor();
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;

signals:
    void clicked();

private:
    void mousePressEvent(QMouseEvent *ev) override;

private:
    QColor defaultFontColor = CustomSettings::instance()->getActiveFontColor();
    HoverHighlightEffect *hoverHighlightEffect;
};


#endif //DDE_TOP_PANEL_QCLICKABLELABEL_H
