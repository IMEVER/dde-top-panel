#ifndef CUSTOMIZE_MENUBAR_H
#define CUSTOMIZE_MENUBAR_H

#include <QMenuBar>
#include <QColor>

class CustomizeMenubar : public QMenuBar
{

    public:
        explicit CustomizeMenubar(QWidget *parent);
        ~CustomizeMenubar() = default;
        void setColor(QColor color);
        QMenu *addMenu(const QString &title);
        QMenu *addMenu(const QIcon &icon, const QString &title);

    protected:
        void paintEvent(QPaintEvent *e) override;

    private:
        QColor m_color;
};

#endif