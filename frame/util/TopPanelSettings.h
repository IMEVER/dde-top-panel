//
// Created by septemberhx on 2020/5/23.
//

#ifndef DDE_TOP_PANEL_TOPPANELSETTINGS_H
#define DDE_TOP_PANEL_TOPPANELSETTINGS_H

#include "constants.h"
#include <QObject>
#include <QWidget>
#include <QMenu>

class TopPanelSettings : public QObject
{
    Q_OBJECT

public:
    explicit TopPanelSettings(QScreen *screen, QWidget *parent = 0);

    inline const QRect primaryRawRect() const { return m_primaryRawRect; }
    QSize windowSize();
    const QRect windowRect() const;
    const QRect primaryRect() const;
    void showDockSettingsMenu();
    void moveToScreen(QScreen *screen);
    inline bool autoHide() { return m_autoHide; };

signals:
    void settingActionClicked();
    void aboutActionClicked();
    void autoHideChanged(bool autoHide);

public slots:
    void setAutoHide(bool autoHide);

private:
    TopPanelSettings(TopPanelSettings const &) = delete;
    TopPanelSettings operator =(TopPanelSettings const &) = delete;
    qreal dockRatio() const;

private:
    QScreen *m_screen;
    QRect m_primaryRawRect;
    bool m_autoHide;
};

#endif //DDE_TOP_PANEL_TOPPANELSETTINGS_H
