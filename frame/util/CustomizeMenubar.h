#ifndef CUSTOMIZE_MENUBAR_H
#define CUSTOMIZE_MENUBAR_H

#include <QMenuBar>

class CustomizeMenubar : public QMenuBar
{

    public:
        explicit CustomizeMenubar(QWidget *parent);
        ~CustomizeMenubar();

    protected:
        void paintEvent(QPaintEvent *e) override;

};

#endif