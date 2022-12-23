#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "serial.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_devRefreshButton_clicked();

    void on_devConnectButton_clicked();

    void on_logTestButton_clicked();

    void on_logClearButton_clicked();

    void on_queryPingButton_clicked();

    void on_queryVersionButton_clicked();

    void on_queryPollInputsButton_clicked();

    void on_horizontalSlider_valueChanged(int value);

    void on_bufferSwapButton_clicked();

    void on_bufferGetFrontButton_clicked();

    void on_bufferGetBackButton_clicked();

    void on_bufferGetPixelButton_clicked();

    void on_bufferSetPixelButton_clicked();

    void on_bufferClearFrontButton_clicked();

    void on_bufferClearBackButton_clicked();

    void on_bufferDrawTestButton_clicked();

    void on_settingsSetPowerOnImageButton_clicked();

private:
    Ui::MainWindow *ui;
    void getScreenBuffer(TargetBuffer target);
};
#endif // MAINWINDOW_H
