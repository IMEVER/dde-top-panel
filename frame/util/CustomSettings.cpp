//
// Created by septemberhx on 2020/6/5.
//

#include "CustomSettings.h"
#include <QSettings>
#include <QStandardPaths>

CustomSettings::CustomSettings() {
    this->setDefaultActiveCloseIconPath();
    this->setDefaultActiveMinimizedIconPath();
    this->setDefaultActiveUnmaximizedIconPath();

    this->readSettings();
    connect(this, &CustomSettings::settingsChanged, this, &CustomSettings::saveSettings);
}

CustomSettings *CustomSettings::instance() {
    static CustomSettings customSettings;
    return &customSettings;
}

bool CustomSettings::getPopupHover()
{
    return m_popupHover;
}

void CustomSettings::setPopupHover(bool popupHover)
{
    if(m_popupHover != popupHover)
    {
        m_popupHover = popupHover;
        saveSettings();
        emit popupHoverChanged(m_popupHover);
    }
}

bool CustomSettings::isIconThemeFollowSystem()
{
    return m_iconThemeFollowSystem;
}

void CustomSettings::setIconThemeFollowSystem(bool follow)
{
    if(m_iconThemeFollowSystem != follow)
    {
        m_iconThemeFollowSystem = follow;
        emit settingsChanged();
    }
}

bool CustomSettings::isPanelCustom() const {
    return m_panelCustom;
}

void CustomSettings::setPanelCustom(bool panelCustom) {
    this->m_panelCustom = panelCustom;
    emit settingsChanged();
    emit panelChanged();
}

qreal CustomSettings::getPanelOpacity() const {
    return panelOpacity;
}

void CustomSettings::setPanelOpacity(qreal panelOpacity) {
    CustomSettings::panelOpacity = panelOpacity;
    emit settingsChanged();
    emit panelChanged();
}

const QColor &CustomSettings::getPanelBgColor() const {
    return panelBgColor;
}

void CustomSettings::setPanelBgColor(const QColor &panelBgColor) {
    CustomSettings::panelBgColor = panelBgColor;
    emit settingsChanged();
    emit panelChanged();
}

QString CustomSettings::getPanelBgImg()
{
    return m_panelBgImg.isNull() ? m_panelBgImg : QString(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/icons/" + m_panelBgImg);
}

QString CustomSettings::getLogoImg()
{
    return m_logo.isNull() ? m_logo : QString(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/icons/" + m_logo);
}

const QColor &CustomSettings::getActiveFontColor() const {
    return activeFontColor;
}

void CustomSettings::setActiveFontColor(const QColor &activeFontColor) {
    CustomSettings::activeFontColor = activeFontColor;
    emit settingsChanged();
}

const QString CustomSettings::getActiveCloseIconPath() const {
    return activeCloseIconPath.startsWith(":/") ? activeCloseIconPath : QString(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/icons/" + activeCloseIconPath);
}

void CustomSettings::setActiveCloseIconPath(const QString &activeCloseIconPath) {
    CustomSettings::activeCloseIconPath = activeCloseIconPath;
    emit settingsChanged();
}

const QString CustomSettings::getActiveUnmaximizedIconPath() const {
    return activeUnmaximizedIconPath.startsWith(":/") ? activeUnmaximizedIconPath : QString(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/icons/" + activeUnmaximizedIconPath);
}

void CustomSettings::setActiveUnmaximizedIconPath(const QString &activeUnmaximizedIconPath) {
    CustomSettings::activeUnmaximizedIconPath = activeUnmaximizedIconPath;
    emit settingsChanged();
}

const QString CustomSettings::getActiveMinimizedIconPath() const {
    return activeMinimizedIconPath.startsWith(":/") ? activeMinimizedIconPath : QString(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/icons/" + activeMinimizedIconPath);
}

void CustomSettings::setActiveMinimizedIconPath(const QString &activeMinimizedIconPath) {
    CustomSettings::activeMinimizedIconPath = activeMinimizedIconPath;
    emit settingsChanged();
}

void CustomSettings::setDefaultActiveCloseIconPath() {
    this->activeCloseIconPath = ":/icons/close.svg";
}

void CustomSettings::setDefaultActiveUnmaximizedIconPath() {
    this->activeUnmaximizedIconPath = ":/icons/maximum.svg";
}

void CustomSettings::setDefaultActiveMinimizedIconPath() {
    this->activeMinimizedIconPath = ":/icons/minimum.svg";
}

void CustomSettings::setDefaultActiveFontColor() {
    this->activeFontColor = Qt::black;
}

void CustomSettings::resetCloseIconPath() {
    this->setDefaultActiveCloseIconPath();
    emit settingsChanged();
}

void CustomSettings::resetUnmaxIconPath() {
    this->setDefaultActiveUnmaximizedIconPath();
    emit settingsChanged();
}

void CustomSettings::resetMinIconPath() {
    this->setDefaultActiveMinimizedIconPath();
    emit settingsChanged();
}

void CustomSettings::saveSettings() {
    QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/dde-top-panel.ini", QSettings::IniFormat);

    settings.setValue("popupHover", m_popupHover);
    settings.setValue("iconThemeFollowSystem", m_iconThemeFollowSystem);
    settings.setValue("panel/panelCustom", m_panelCustom);
    settings.setValue("panel/bgColor", this->panelBgColor);
    settings.setValue("panel/opacity", this->panelOpacity);
    settings.setValue("windowControl/fontColor", this->activeFontColor);
    settings.setValue("windowControl/closeIcon", activeCloseIconPath);
    settings.setValue("windowControl/unmaxIcon", activeUnmaximizedIconPath);
    settings.setValue("windowControl/minIcon", this->activeMinimizedIconPath);
    settings.setValue("windowControl/buttonCustom", this->m_buttonCustom);
}

void CustomSettings::readSettings() {
    QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/dde-top-panel.ini", QSettings::IniFormat);
    this->m_popupHover = settings.value("popupHover", false).toBool();
    this->m_iconThemeFollowSystem = settings.value("iconThemeFollowSystem", false).toBool();
    this->m_panelCustom = settings.value("panel/panelCustom", false).toBool();
    this->panelBgColor = settings.value("panel/bgColor", (QColor)Qt::white).value<QColor>();
    this->m_panelBgImg = settings.value("panel/bgImg", QString()).toString();
    this->m_logo = settings.value("logo", QString()).toString();
    this->panelOpacity = settings.value("panel/opacity", 50).toUInt();
    this->activeFontColor = settings.value("windowControl/fontColor", (QColor)Qt::black).value<QColor>();
    this->activeCloseIconPath = settings.value("windowControl/closeIcon", this->activeCloseIconPath).toString();
    this->activeUnmaximizedIconPath = settings.value("windowControl/unmaxIcon", this->activeUnmaximizedIconPath).toString();
    this->activeMinimizedIconPath = settings.value("windowControl/minIcon", this->activeMinimizedIconPath).toString();
    this->m_buttonCustom = settings.value("windowControl/buttonCustom", false).toBool();
}

bool CustomSettings::isButtonCustom() const {
    return this->m_buttonCustom;
}

void CustomSettings::setButtonCustom(bool buttonCustom) {
    this->m_buttonCustom = buttonCustom;
    emit settingsChanged();
}