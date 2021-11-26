//
// Created by septemberhx on 2020/6/5.
//

#ifndef DDE_TOP_PANEL_CUSTOMSETTINGS_H
#define DDE_TOP_PANEL_CUSTOMSETTINGS_H

#include <QObject>
#include <QColor>

class CustomSettings : public QObject {

    Q_OBJECT

public:
    static CustomSettings* instance();

    bool getPopupHover();
    void setPopupHover(bool popupHover);

    qreal getPanelOpacity() const;
    void setPanelOpacity(qreal panelOpacity);

    const QColor &getPanelBgColor() const;
    void setPanelBgColor(const QColor &panelBgColor);

    QString getPanelBgImg();
    QString getLogoImg();

    const QColor &getActiveFontColor() const;
    void setActiveFontColor(const QColor &activeFontColor);

    const QString getActiveCloseIconPath() const;
    void setActiveCloseIconPath(const QString &activeCloseIconPath);

    const QString getActiveUnmaximizedIconPath() const;
    void setActiveUnmaximizedIconPath(const QString &activeUnmaximizedIconPath);

    const QString getActiveMinimizedIconPath() const;
    void setActiveMinimizedIconPath(const QString &activeMinimizedIconPath);

    void saveSettings();
    void readSettings();

    void resetCloseIconPath();
    void resetUnmaxIconPath();
    void resetMinIconPath();
    void resetDefaultIconPath();

    void setDefaultActiveFontColor();
    void setDefaultActiveCloseIconPath();
    void setDefaultActiveUnmaximizedIconPath();
    void setDefaultActiveMinimizedIconPath();

    bool isPanelCustom() const;
    void setPanelCustom(bool panelCustom);

    bool isButtonCustom() const;
    void setButtonCustom(bool buttonCustom);

    bool isIconThemeFollowSystem();
    void setIconThemeFollowSystem(bool follow);

signals:
    void settingsChanged();
    void popupHoverChanged(bool popupHover);
    void panelChanged();

private:
    CustomSettings();

private:
    bool m_popupHover;
    // panel
    quint8 panelOpacity;
    QColor panelBgColor;
    QString m_panelBgImg;
    QString m_logo;

    bool m_panelCustom;
    bool m_buttonCustom;
    bool m_iconThemeFollowSystem;

    // active window control
    QColor activeFontColor;
    QString activeCloseIconPath;
    QString activeUnmaximizedIconPath;
    QString activeMinimizedIconPath;
};


#endif //DDE_TOP_PANEL_CUSTOMSETTINGS_H
