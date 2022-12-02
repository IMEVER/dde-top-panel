#include "mainsettingwidget.h"
#include "ui_mainsettingwidget.h"
#include "../util/CustomSettings.h"
#include "../controller/dockitemmanager.h"
#include <QIcon>
#include <QColorDialog>
#include <QFileDialog>
#include <QMovie>
#include <QApplication>
#include <QStandardPaths>
#include <QListWidgetItem>
#include <QItemDelegate>
#include <QAbstractItemModel>
#include <QStyleOptionViewItem>
#include <QModelIndex>
#include <QStyle>

class MyItemDelegate : public QItemDelegate
{
    public:

    explicit MyItemDelegate(QObject *parent = nullptr) :QItemDelegate(parent) {}
    ~MyItemDelegate(){}

protected:
    bool editorEvent(QEvent *event,
                                    QAbstractItemModel *model,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index)
    {
        Q_ASSERT(event);
        Q_ASSERT(model);
        // make sure that the item is checkable
        Qt::ItemFlags flags = model->flags(index);
        if (!(flags & Qt::ItemIsUserCheckable) || !(option.state & QStyle::State_Enabled)
            || !(flags & Qt::ItemIsEnabled))
            return false;
        // make sure that we have a check state
        QVariant value = index.data(Qt::CheckStateRole);
        if (!value.isValid())
            return false;
        // make sure that we have the right event type
        if ((event->type() == QEvent::MouseButtonRelease)
            || (event->type() == QEvent::MouseButtonDblClick)
            || (event->type() == QEvent::MouseButtonPress)) {
            QRect checkRect = doCheck(option, option.rect, Qt::Checked);
            QRect emptyRect;
            doLayout(option, &checkRect, &emptyRect, &emptyRect, false);
            QMouseEvent *me = static_cast<QMouseEvent*>(event);
            if (me->button() != Qt::LeftButton || !checkRect.contains(me->pos()))
                return false;
            // eat the double click events inside the check rect
            if ((event->type() == QEvent::MouseButtonPress)
                || (event->type() == QEvent::MouseButtonDblClick))
                {
                    Qt::CheckState state = static_cast<Qt::CheckState>(value.toInt());
                    if (flags & Qt::ItemIsUserTristate)
                        state = ((Qt::CheckState)((state + 1) % 3));
                    else
                        state = (state == Qt::Checked) ? Qt::Unchecked : Qt::Checked;
                    return model->setData(index, state, Qt::CheckStateRole);

                    return true;
                }
        } else if (event->type() == QEvent::KeyPress) {
            if (static_cast<QKeyEvent*>(event)->key() != Qt::Key_Space
            && static_cast<QKeyEvent*>(event)->key() != Qt::Key_Select)
                return false;
        }

        return false;
    }


    void doLayout_bk(const QStyleOptionViewItem &option,
                                QRect *checkRect, QRect *pixmapRect, QRect *textRect,
                                bool hint) const
    {
        Q_ASSERT(checkRect && pixmapRect && textRect);
        const bool hasCheck = checkRect->isValid();
        const int x = option.rect.left();
        int w;

        QItemDelegate::doLayout(option, checkRect, pixmapRect, textRect, hint);

        QRect check, decoration, display;

        if (hint) {
            if (option.decorationPosition == QStyleOptionViewItem::Left
                || option.decorationPosition == QStyleOptionViewItem::Right) {
                w = textRect->width() + pixmapRect->width();
            } else {
                w = qMax(textRect->width(), pixmapRect->width());
            }
        } else {
            w = option.rect.width();
        }

        if (hasCheck) {
            if(hint) w += checkRect->width();
            if (option.direction == Qt::RightToLeft) {
                check.setRect(x, checkRect->y(), checkRect->width(), checkRect->height());
            } else {
                check.setRect(x+w-checkRect->width(), checkRect->y(), checkRect->width(), checkRect->height());
            }
        }

        // at this point w should be the *total* width
        switch (option.decorationPosition) {
        case QStyleOptionViewItem::Top:
        case QStyleOptionViewItem::Bottom:
            if (option.direction == Qt::RightToLeft) {
                decoration.setRect(x+checkRect->width(), pixmapRect->y(), pixmapRect->width(), pixmapRect->height());
                display.setRect(x+checkRect->width(), textRect->y(), textRect->width(), textRect->height());
            } else {
                decoration.setRect(x, pixmapRect->y(), pixmapRect->width(), pixmapRect->height());
                display.setRect(x, textRect->y(), textRect->width(), textRect->height());
            }
            break;
        case QStyleOptionViewItem::Left: {
            if (option.direction == Qt::LeftToRight) {
                decoration.setRect(x, pixmapRect->y(), pixmapRect->width(), pixmapRect->height());
                display.setRect(decoration.right() + 1, textRect->y(), textRect->width(), textRect->height());
            } else {
                display.setRect(x+checkRect->width(), textRect->y(), textRect->width(), textRect->height());
                decoration.setRect(display.right() + 1, pixmapRect->y(), pixmapRect->width(), pixmapRect->height());
            }
            break; }
        case QStyleOptionViewItem::Right: {
            if (option.direction == Qt::LeftToRight) {
                display.setRect(x, textRect->y(), textRect->width(), textRect->height());
                decoration.setRect(display.right() + 1, pixmapRect->y(), pixmapRect->width(), pixmapRect->height());
            } else {
                decoration.setRect(x+checkRect->width(), pixmapRect->y(), pixmapRect->width(), pixmapRect->height());
                display.setRect(decoration.right() + 1, textRect->y(), textRect->width(), textRect->height());
            }
            break; }
        default:
            decoration = *pixmapRect;
            if(option.direction == Qt::LeftToRight)
            {
                display.setRect(x, textRect->y(), textRect->width(), textRect->height());
            }
            else
            {
                display.setRect(x+check.width(), textRect->y(), textRect->width(), textRect->height());
            }

            break;
        }

        *checkRect = check;
        *pixmapRect = decoration;
        *textRect = display;
    }
};


MainSettingWidget::MainSettingWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainSettingWidget)
{
    ui->setupUi(this);

    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint | Qt::WindowMinimizeButtonHint);
    setAttribute(Qt::WA_DeleteOnClose);

    ui->popupHoverCheckBox->setChecked(CustomSettings::instance()->getPopupHover());
    ui->iconThemeCheckBox->setChecked(CustomSettings::instance()->isIconThemeFollowSystem());
    ui->showDesktopCheckBox->setChecked(CustomSettings::instance()->isShowDesktop());

    ui->panelColorToolButton->setStyleSheet(QString("QToolButton {background: %1; border: none;}").arg(CustomSettings::instance()->getPanelBgColor().name()));
    ui->fontColorToolButton->setStyleSheet(QString("QToolButton {background: %1; border: none;}").arg(CustomSettings::instance()->getActiveFontColor().name()));
    ui->opacitySpinBox->setValue(CustomSettings::instance()->getPanelOpacity());

    ui->closeButtonLabel->setPixmap(QIcon(CustomSettings::instance()->getActiveCloseIconPath()).pixmap(QSize(20, 20)));
    ui->unmaxButtonLabel->setPixmap(QIcon(CustomSettings::instance()->getActiveUnmaximizedIconPath()).pixmap(QSize(20, 20)));
    ui->minButtonLabel->setPixmap(QIcon(CustomSettings::instance()->getActiveMinimizedIconPath()).pixmap(QSize(20, 20)));

    connect(ui->panelCustomCheckBox, &QCheckBox::toggled, [this](bool checked){
        ui->panelColorToolButton->setDisabled(!checked);
        ui->fontColorToolButton->setDisabled(!checked);
        ui->opacitySpinBox->setDisabled(!checked);
     });
     connect(ui->buttonCustomCheckBox, &QCheckBox::toggled,  [this](bool checked){
         ui->minToolButton->setDisabled(!checked);
         ui->minResetToolButton->setDisabled(!checked);
         ui->unmaxButtonToolButton->setDisabled(!checked);
         ui->unmaxResetToolButton->setDisabled(!checked);
         ui->closeButtonToolButton->setDisabled(!checked);
         ui->closeResetToolButton->setDisabled(!checked);
     });

    ui->panelCustomCheckBox->setChecked(CustomSettings::instance()->isPanelCustom());
    ui->buttonCustomCheckBox->setChecked(CustomSettings::instance()->isButtonCustom());

    //menu
    ui->menu_launch->setChecked(CustomSettings::instance()->isShowMenuLaunch());
    ui->menu_fileManager->setChecked(CustomSettings::instance()->isShowMenuFileManager());
    ui->menu_theme->setChecked(CustomSettings::instance()->isShowMenuTheme());
    ui->menu_aboutPackage->setChecked(CustomSettings::instance()->isShowMenuAboutPackage());
    ui->menu_search->setChecked(CustomSettings::instance()->isShowMenuSearch());

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
    connect(ui->showDesktopCheckBox, &QCheckBox::stateChanged, CustomSettings::instance(), &CustomSettings::setShowDesktop);
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

    //menu
    connect(ui->menu_launch, &QCheckBox::stateChanged, CustomSettings::instance(), &CustomSettings::setMenuLaunch);
    connect(ui->menu_fileManager, &QCheckBox::stateChanged, CustomSettings::instance(), &CustomSettings::setMenuFileManager);
    connect(ui->menu_theme, &QCheckBox::stateChanged, CustomSettings::instance(), &CustomSettings::setMenuTheme);
    connect(ui->menu_aboutPackage, &QCheckBox::stateChanged, CustomSettings::instance(), &CustomSettings::setMenuAboutPackage);
    connect(ui->menu_search, &QCheckBox::stateChanged, CustomSettings::instance(), &CustomSettings::setMenuSearch);

    //plugin
    // ui->listWidget->setLayoutDirection(Qt::RightToLeft);
    ui->listWidget->setItemDelegate(new MyItemDelegate(this));
    for (auto *p : DockItemManager::instance()->pluginList())
    {
        QListWidgetItem *item = new QListWidgetItem(QString("%1 (%2)").arg(p->pluginDisplayName().isEmpty() ? p->pluginName() : p->pluginDisplayName()).arg(p->pluginName()));
        Qt::ItemFlags flags = item->flags() | Qt::ItemIsUserCheckable;
        if(p->pluginIsAllowDisable())
            flags |= Qt::ItemIsEnabled;
        else
            flags &= ~Qt::ItemIsEnabled;

        item->setFlags(flags);

        item->setCheckState(p->pluginIsDisable() ? Qt::Unchecked : Qt::Checked);
        item->setData(Qt::UserRole, p->pluginName());

        ui->listWidget->addItem(item);
    }

    connect(ui->listWidget, &QListWidget::itemChanged, [this](QListWidgetItem *item) {
        if(item->flags().testFlag(Qt::ItemIsEnabled) == false)
            return;

        const QString &data = item->data(Qt::UserRole).toString();
        if (!data.isEmpty())
        {
            // item->setCheckState(item->checkState() == Qt::Unchecked ? Qt::Checked : Qt::Unchecked);
            for (auto *p : DockItemManager::instance()->pluginList()) {
                if (p->pluginName() == data) {
                    p->pluginStateSwitched();
                    emit pluginVisibleChanged(p->pluginDisplayName(), !p->pluginIsDisable());
                    break;
                }
            }
        }
     });

    // connect(ui->listWidget, &QListWidget::itemClicked, [](QListWidgetItem *item) {
    //     if(item->flags().testFlag(Qt::ItemIsEnabled) == false)
    //         return;

    //     const QString &data = item->data(Qt::UserRole).toString();
    //     if (!data.isEmpty())
    //     {
    //         item->setCheckState(item->checkState() == Qt::Unchecked ? Qt::Checked : Qt::Unchecked);
    //         for (auto *p : DockItemManager::instance()->pluginList()) {
    //             if (p->pluginName() == data)
    //                 return p->pluginStateSwitched();
    //         }
    //     }
    //  });

    // ui->listWidget->setSelectionMode(QAbstractItemView::NoSelection);
    // ui->listWidget->setDragDropMode(QAbstractItemView::NoDragDrop);
    // ui->listWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // ui->listWidget->setDragEnabled(false);
}

MainSettingWidget::~MainSettingWidget()
{
    delete ui;
}

void MainSettingWidget::showSetting() {
    ui->tabWidget->setCurrentIndex(0);
    show();
}

void MainSettingWidget::showAbout() {
    ui->tabWidget->setCurrentIndex(3);
    show();
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
