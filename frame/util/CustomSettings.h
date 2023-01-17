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
    bool isPanelBgImgRepeat();
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

    void setShowDesktop(bool showDesktop);
    bool isShowDesktop();

    void setMenuApp(bool show);
    bool isShowMenuApp();
    void setMenuLaunch(bool show);
    bool isShowMenuLaunch();
    void setMenuFileManager(bool show);
    bool isShowMenuFileManager();
    void setMenuTheme(bool show);
    bool isShowMenuTheme();
    void setMenuAboutPackage(bool show);
    bool isShowMenuAboutPackage();
    void setMenuSearch(bool show);
    bool isShowMenuSearch();

signals:
    void settingsChanged();
    void popupHoverChanged(bool popupHover);
    void panelChanged();
    void showDesktopChanged(bool show);
    void menuChanged(const QString menu, const bool show);

private:
    CustomSettings();

private:
    bool m_popupHover;
    // panel
    quint8 panelOpacity;
    QColor panelBgColor;
    QString m_panelBgImg;
    bool m_panelBgImgRepeat;
    QString m_logo;

    bool m_panelCustom;
    bool m_buttonCustom;
    bool m_iconThemeFollowSystem;
    bool m_showDesktop;

    // active window control
    QColor activeFontColor;
    QString activeCloseIconPath;
    QString activeUnmaximizedIconPath;
    QString activeMinimizedIconPath;

    // custom menu
    bool m_menuApp;
    bool m_menuLaunch;
    bool m_menuFileManager;
    bool m_menuTheme;
    bool m_menuAboutPackage;
    bool m_menuSearch;
};


#endif //DDE_TOP_PANEL_CUSTOMSETTINGS_H
