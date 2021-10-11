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

    qreal getPanelOpacity() const;

    void setPanelOpacity(qreal panelOpacity);

    const QColor &getPanelBgColor() const;

    void setPanelBgColor(const QColor &panelBgColor);

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


signals:
    void settingsChanged();

private:
    CustomSettings();

private:
    // panel
    quint8 panelOpacity;
    QColor panelBgColor;

    bool m_panelCustom;
    bool m_buttonCustom;

    // active window control
    QColor activeFontColor;
    QString activeCloseIconPath;
    QString activeUnmaximizedIconPath;
    QString activeMinimizedIconPath;
};


#endif //DDE_TOP_PANEL_CUSTOMSETTINGS_H
