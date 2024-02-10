#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "serial.h"

#ifdef __unix__
    #ifdef __linux__
        #define detectedOS "Linux/Unix-based OS"
    #else
        #define detectedOS "Other Unix-based OS"
    #endif
#else
    #ifdef _WIN32
        #ifdef _WIN64
            #define detectedOS "Windows 64-bit OS"
        #else
            #define detectedOS "Windows 32-bit OS"
        #endif
    #else
        #ifdef __APPLE__
            #define detectedOS "Apple OS"
        #else
            #define detectedOS "Unknown OS"
        #endif
    #endif
#endif



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

    void on_bufferCopyFrontToBackButton_clicked();

    void on_bufferCopyBackToFrontButton_clicked();

    void on_imageLoadImagesButton_clicked();

    void on_imageDrawImagesButton_clicked();

    void on_imagePreviewFrameNextButton_clicked();

    void on_imagePreviewFrameLastButton_clicked();

    void on_imagePreviewFramePrevButton_clicked();

    void on_imagePreviewFrameFirstButton_clicked();

    void on_imageLoadAudioButton_clicked();

    void on_imageDrawAndPlayButton_clicked();

    void on_actionDumpToLog_triggered();

    void on_webcamOpenButton_clicked();

    void on_webcamDrawButton_clicked();

private:
    Ui::MainWindow *ui;
    void getScreenBuffer(TargetBuffer target);
    void updateImagePreview(int index);
};
#endif // MAINWINDOW_H
