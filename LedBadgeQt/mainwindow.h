#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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

    void on_queryGetImageButton_clicked();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
