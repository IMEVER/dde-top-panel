#ifndef MAINSETTINGWIDGET_H
#define MAINSETTINGWIDGET_H

#include <QWidget>

namespace Ui {
class MainSettingWidget;
}

class MainSettingWidget : public QWidget
{
    Q_OBJECT

    enum FileType {
        closeButton
        , restoreButton
        , minimumButton
    };

public:
    explicit MainSettingWidget(QWidget *parent = nullptr);
    ~MainSettingWidget();

private:
    Ui::MainSettingWidget *ui;
    QMovie *movie;
protected:
    void closeEvent(QCloseEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:

    void panelColorButtonClicked();
    void fontColorButtonClicked();

    void closeButtonClicked();
    void closeResetButtonClicked();

    void unmaxButtonClicked();
    void unmaxResetButtonClicked();

    void minButtonClicked();
    void minResetButtonClicked();

    void copyIcon(const QString filePath, FileType type);
};

#endif // MAINSETTINGWIDGET_H
