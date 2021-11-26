#include "mainsettingwidget.h"
#include "ui_mainsettingwidget.h"
#include "../util/CustomSettings.h"
#include <QIcon>
#include <QColorDialog>
#include <QFileDialog>
#include <QMovie>
#include <QApplication>
#include <QStandardPaths>

MainSettingWidget::MainSettingWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainSettingWidget)
{
    ui->setupUi(this);

    ui->popupHoverCheckBox->setChecked(CustomSettings::instance()->getPopupHover());
    ui->iconThemeCheckBox->setChecked(CustomSettings::instance()->isIconThemeFollowSystem());

    ui->panelColorToolButton->setStyleSheet(QString("QToolButton {background: %1; border: none;}").arg(CustomSettings::instance()->getPanelBgColor().name()));
    ui->fontColorToolButton->setStyleSheet(QString("QToolButton {background: %1; border: none;}").arg(CustomSettings::instance()->getActiveFontColor().name()));

    ui->opacitySpinBox->setValue(CustomSettings::instance()->getPanelOpacity());

    ui->closeButtonLabel->setPixmap(QIcon(CustomSettings::instance()->getActiveCloseIconPath()).pixmap(QSize(20, 20)));
    ui->unmaxButtonLabel->setPixmap(QIcon(CustomSettings::instance()->getActiveUnmaximizedIconPath()).pixmap(QSize(20, 20)));
    ui->minButtonLabel->setPixmap(QIcon(CustomSettings::instance()->getActiveMinimizedIconPath()).pixmap(QSize(20, 20)));
    ui->panelCustomCheckBox->setChecked(CustomSettings::instance()->isPanelCustom());
    ui->buttonCustomCheckBox->setChecked(CustomSettings::instance()->isButtonCustom());

    ui->appNameLabel->setText(QApplication::applicationDisplayName());
    ui->appVersionLabel->setText(QApplication::applicationVersion());

    movie = new QMovie(":/icons/doge.gif", QByteArray(), this);
    ui->pMovieLabel->setMovie(movie);

    ui->minToolButton->setIcon(QIcon(":/icons/config.svg"));
    ui->minResetToolButton->setIcon(QIcon(":/icons/reset.svg"));
    ui->unmaxButtonToolButton->setIcon(QIcon(":/icons/config.svg"));
    ui->unmaxResetToolButton->setIcon(QIcon(":/icons/reset.svg"));
    ui->closeButtonToolButton->setIcon(QIcon(":/icons/config.svg"));
    ui->closeResetToolButton->setIcon(QIcon(":/icons/reset.svg"));

    connect(ui->popupHoverCheckBox, &QCheckBox::stateChanged, CustomSettings::instance(), &CustomSettings::setPopupHover);
    connect(ui->iconThemeCheckBox, &QCheckBox::stateChanged, CustomSettings::instance(), &CustomSettings::setIconThemeFollowSystem);
    connect(ui->opacitySpinBox, qOverload<int>(&QSpinBox::valueChanged), CustomSettings::instance(), &CustomSettings::setPanelOpacity);
    connect(ui->panelColorToolButton, &QToolButton::clicked, this, &MainSettingWidget::panelColorButtonClicked);
    connect(ui->fontColorToolButton, &QToolButton::clicked, this, &MainSettingWidget::fontColorButtonClicked);
    connect(ui->closeButtonToolButton, &QToolButton::clicked, this, &MainSettingWidget::closeButtonClicked);
    connect(ui->closeResetToolButton, &QToolButton::clicked, this, &MainSettingWidget::closeResetButtonClicked);
    connect(ui->unmaxButtonToolButton, &QToolButton::clicked, this, &MainSettingWidget::unmaxButtonClicked);
    connect(ui->unmaxResetToolButton, &QToolButton::clicked, this, &MainSettingWidget::unmaxResetButtonClicked);
    connect(ui->minToolButton, &QToolButton::clicked, this, &MainSettingWidget::minButtonClicked);
    connect(ui->minResetToolButton, &QToolButton::clicked, this, &MainSettingWidget::minResetButtonClicked);
    connect(ui->panelCustomCheckBox, &QCheckBox::stateChanged, CustomSettings::instance(), &CustomSettings::setPanelCustom);
    connect(ui->buttonCustomCheckBox, &QCheckBox::stateChanged, CustomSettings::instance(), &CustomSettings::setButtonCustom);
}

MainSettingWidget::~MainSettingWidget()
{
    delete ui;
}

void MainSettingWidget::panelColorButtonClicked() {
    QColorDialog dialog(CustomSettings::instance()->getPanelBgColor(), this);
    dialog.setOptions(QColorDialog::ShowAlphaChannel | QColorDialog::NoButtons);

    connect(&dialog, &QColorDialog::currentColorChanged, [ this ](const QColor &color){
        ui->panelColorToolButton->setStyleSheet(QString("QToolButton {background: %1; border: none;}").arg(color.name()));
        CustomSettings::instance()->setPanelBgColor(color);
     });

    dialog.exec();
    // QColor newPanelBgColor = QColorDialog::getColor(CustomSettings::instance()->getPanelBgColor());
    // ui->panelColorToolButton->setStyleSheet(QString("QToolButton {background: %1;}").arg(newPanelBgColor.name()));
    // CustomSettings::instance()->setPanelBgColor(newPanelBgColor);
}

void MainSettingWidget::fontColorButtonClicked() {
    QColorDialog dialog(CustomSettings::instance()->getPanelBgColor(), this);
    dialog.setOptions(QColorDialog::ShowAlphaChannel | QColorDialog::NoButtons);

    connect(&dialog, &QColorDialog::currentColorChanged, [ this ](const QColor &color){
        ui->fontColorToolButton->setStyleSheet(QString("QToolButton {background: %1; border: none;}").arg(color.name()));
        CustomSettings::instance()->setActiveFontColor(color);
     });

    dialog.exec();
    // QColor newFontColor = QColorDialog::getColor(CustomSettings::instance()->getActiveFontColor());
    // ui->fontColorToolButton->setStyleSheet(QString("QToolButton {background: %1;}").arg(newFontColor.name()));
    // CustomSettings::instance()->setActiveFontColor(newFontColor);
}

void MainSettingWidget::closeButtonClicked() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("选择关闭按钮图标"), "~", tr("Images (*.png *.jpg *.svg)"));
    if (!fileName.isNull()) {
        ui->closeButtonLabel->setPixmap(QIcon(fileName).pixmap(QSize(20, 20)));
        copyIcon(fileName, closeButton);
    }
}

void MainSettingWidget::closeResetButtonClicked() {
    CustomSettings::instance()->resetCloseIconPath();
    ui->closeButtonLabel->setPixmap(QIcon(CustomSettings::instance()->getActiveCloseIconPath()).pixmap(QSize(20, 20)));
}

void MainSettingWidget::unmaxButtonClicked() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("选择还原按钮图标"), "~", tr("Images (*.png *.jpg *.svg)"));
    if (!fileName.isNull()) {
        ui->unmaxButtonLabel->setPixmap(QIcon(fileName).pixmap(QSize(20, 20)));
        copyIcon(fileName, restoreButton);
    }
}

void MainSettingWidget::unmaxResetButtonClicked() {
    CustomSettings::instance()->resetUnmaxIconPath();
    ui->unmaxButtonLabel->setPixmap(QIcon(CustomSettings::instance()->getActiveUnmaximizedIconPath()).pixmap(QSize(20, 20)));
}

void MainSettingWidget::minButtonClicked() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("选择最小化按钮图标"), "~", tr("Images (*.png *.jpg *.svg)"));
    if (!fileName.isNull()) {
        ui->minButtonLabel->setPixmap(QIcon(fileName).pixmap(QSize(20, 20)));
        copyIcon(fileName, minimumButton);
    }
}

void MainSettingWidget::minResetButtonClicked() {
    CustomSettings::instance()->resetMinIconPath();
    ui->minButtonLabel->setPixmap(QIcon(CustomSettings::instance()->getActiveMinimizedIconPath()).pixmap(QSize(20, 20)));
}

void MainSettingWidget::showEvent(QShowEvent *event) {
    movie->start();
    QWidget::showEvent(event);
}

void MainSettingWidget::hideEvent(QHideEvent *event) {
    movie->stop();
    QWidget::hideEvent(event);
}

void MainSettingWidget::closeEvent(QCloseEvent *event) {
    movie->stop();
    QWidget::closeEvent(event);
}

void MainSettingWidget::copyIcon(const QString filePath, FileType type)
{
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    if(!dir.exists("icons"))
        dir.mkdir("icons");

    QFileInfo fileInfo(filePath);
    QFile destFile(dir.absoluteFilePath("icons/") + fileInfo.fileName());

    if(destFile.exists())
        destFile.remove();

    QFile::copy(filePath, destFile.fileName());

    switch (type)
    {
    case closeButton:
        CustomSettings::instance()->setActiveCloseIconPath(fileInfo.fileName());
        break;
    case restoreButton:
        CustomSettings::instance()->setActiveUnmaximizedIconPath(fileInfo.fileName());
        break;
    case minimumButton:
        CustomSettings::instance()->setActiveMinimizedIconPath(fileInfo.fileName());
        break;
    default:
        break;
    }
}
