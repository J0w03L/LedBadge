#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "serial.h"
#include "log.h"
#include "image.h"
#include "webcam.h"
#include "framerate.h"
#include <algorithm>
#include <cstring>
#include <QFileDialog>
#include <QImage>
#include <QPixmap>
#include <QImageReader>
#include <QStringList>
#include <QSysInfo>
#include <chrono>
#include <unistd.h>
#include <spawn.h>
#include <signal.h>

extern char** environ;
QStringList imagePaths;
QStringList imageTmpPaths;
Webcam webcam;
char audioPath[65535] = {0};
int imagePreviewCurrentFrame = 0;
int imagePreviewFrameCount = 0;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    registerLogTextBrowser(ui->logTextBrowser);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateImagePreview(int index)
{
    if (index > (imagePreviewFrameCount - 1) || index < 0)
    {
        printf("updateImagePreview with out-of-range index!\nimagePreviewFrameCount: %i\nimagePreviewCurrentFrame: %i\nindex: %i\n", imagePreviewFrameCount, imagePreviewCurrentFrame, index);
        return;
    }
    imagePreviewCurrentFrame = index;
    char frameCountText[50] = {0};
    sprintf(frameCountText, "%i / %i", imagePreviewCurrentFrame + 1, imagePreviewFrameCount);
    ui->imagePreviewFrameCountLabel->setText(frameCountText);
    readPNGFile(imagePaths[imagePreviewCurrentFrame].toLatin1().data());
    reducePNGColors();
    char tmpPath[65535] = {0};
    sprintf(tmpPath, "/tmp/.LedBadgeQt_imgprev_%i.png", imagePreviewCurrentFrame);
    writePNGFile(tmpPath);
    imageTmpPaths.append(tmpPath);
    QLabel* imageLabel = ui->imagePreviewFrameImageLabel;
    QPixmap pixmap;
    if (!pixmap.load(tmpPath))
    {
        plogf("Failed to load preview image!");
        return;
    }
    pixmap = pixmap.scaled(imageLabel->size(), Qt::KeepAspectRatio);
    imageLabel->setPixmap(pixmap);
    return;
}

void MainWindow::getScreenBuffer(TargetBuffer target)
{
    plogf("Getting screen image...");
    uint8_t buf1[((B_WIDTH * B_HEIGHT) / 4) + 2];
    if (int e = getPixelRect(buf1, 0, 0, B_WIDTH, B_HEIGHT, target) != 0)
    {
        plogf("Couldn't get screen image! (returned %i)", e);
        return;
    }
    uint8_t buf2 = buf1[0];
    uint8_t buf3 = buf1[1];
    printf("buf2 : %i (0x%02x) | buf3: %i (0x%02x)\n", buf2, buf2, buf3, buf3);
    char img[150] = "\0";
    char blank[150] = "\0";

    for (int y = 0; y < 12; y++)
    {
        int x = 0;
        while (x < 12)
        {
            for (int p = 0; p < 8; p += 2)
            {
                uint8_t val;
                uint8_t bit1 = BIT_TEST(buf1[2 + (y * 12) + x], p) ? 1 : 0;
                uint8_t bit2 = BIT_TEST(buf1[2 + (y * 12) + x], (p + 1)) ? 1 : 0;
                switch (bit2)
                {
                    case 1:
                        strncat(img, bit1 ? "\u2588" : "\u2593", 4); // 11 = Full Block; 10 = Dark Shade.
                        break;
                    case 0:
                        strncat(img, bit1 ? "\u2592" : "\u2591", 4); // 01 = Medium Shade; 00 = Light Shade (to keep character spacing (mostly) equal).
                        break;
                }
            }
            x++;
        }
        plogf(img);
        strncpy(img, blank, 150);
    }
    plogf("Received screen image successfully!");
    return;
}

/*
 * TODO: The entire SerialDevice struct isn't really necessary and this code didn't work as originally intended anyway. Remove that struct later.
*/
void MainWindow::on_devRefreshButton_clicked()
{
    closeSerialDevice();
    logf("Refreshing devices...");
    SerialDevice *currentDevList = new SerialDevice[256];
    uint dev_count = 0;
    uint i = 0;
    std::array<std::string, 256> currentDeviceNames;
    struct dirent* dev;
    DIR *dir = opendir("/sys/class/tty");
    if (dir == NULL) { delete[] currentDevList; return; }
    while ((dev = readdir(dir)) != NULL)
    {
        std::string devName = dev->d_name;
        if (devName == "." || devName == "..") continue;
        printf("%u : %s\n", i, devName.c_str());
        currentDevList[i] = SerialDevice{ i++, (devName.rfind("ttyUSB", 0) == 0 || devName.rfind("ttyACM", 0) == 0), false, dev->d_name, dev->d_name };
        currentDeviceNames.at(i) = devName;
        dev_count++;
    }
    while (i > 256)
    {
        currentDevList[i] = SerialDevice{ i++, false, true, (char*) '?', (char*) '?' };
    }
    closedir(dir);
    printf("\n\n");
    ui->devComboBox->clear();
    for (int x = 0; x < dev_count; x++)
    {
        printf("%i : %s\n", x, currentDeviceNames[x].c_str());
        ui->devComboBox->addItem(currentDeviceNames[x].c_str());
    }
    delete[] currentDevList;
    logf("Refreshed devices!");
    return;
}

void MainWindow::on_devConnectButton_clicked()
{
    closeSerialDevice();
    QString currentDeviceNameQString = ui->devComboBox->currentText();
    QByteArray currentDeviceNameQByteArray = currentDeviceNameQString.toLatin1();
    const char *currentDeviceName = currentDeviceNameQByteArray.data();
    if (currentDeviceName[0] == '\0')
    {
        logf("Please select a device to connect to first!");
        return;
    }
    plogf("Connecting to serial device...");
    int e = openSerialDevice(currentDeviceName);
    if (e != 0)
    {
        plogf("Could not connect to serial device! (openSerialDevice returned %i)", e);
        return;
    }
    plogf("Connected to serial device successfully!");
    return;
}


void MainWindow::on_logTestButton_clicked()
{
    printf("Printing test string to log.\n");
    char* currentDeviceName = ui->devComboBox->currentText().toLatin1().data();
    printf("currentDeviceName: '%s'\n", currentDeviceName);
    logf("<span>Current selected device: <b>%s</b></span>", (currentDeviceName[0] != '\0') ? currentDeviceName : "None");
}


void MainWindow::on_logClearButton_clicked()
{
    printf("Clearing log.\n");
    ui->logTextBrowser->clear();
}


void MainWindow::on_queryPingButton_clicked()
{
    plogf("Pinging device...");
    uint8_t buf;
    int i = pingDevice(&buf);
    if (i == 0)
    {
        plogf("Received no reply!");
        return;
    }
    plogf("Received reply: %i (0x%02x)", buf, buf);
    return;
}


void MainWindow::on_queryVersionButton_clicked()
{
    plogf("Querying firmware version...");
    uint8_t buf;
    int i = getVersion(&buf);
    if (i == 0)
    {
        plogf("Couldn't query firmware version; received no reply!");
        return;
    }
    plogf("Firmware Version: %i (0x%02x)", buf, buf);
    return;
}


void MainWindow::on_queryPollInputsButton_clicked()
{
    plogf("Querying input states...");
    uint8_t buf;
    int i = pollInputs(&buf);
    if (i == 0)
    {
        plogf("Couldn't query input states; received no reply!");
        return;
    }
    plogf("K1: %i | K2: %i", BIT_TEST(buf, 0) ? 1 : 0, BIT_TEST(buf, 1) ? 1 : 0);
    return;
}


void MainWindow::on_bufferGetFrontButton_clicked()
{
    getScreenBuffer(TargetBuffer::Front);
    return;
}

void MainWindow::on_bufferGetBackButton_clicked()
{
    getScreenBuffer(TargetBuffer::Back);
    return;
}

void MainWindow::on_horizontalSlider_valueChanged(int value)
{
    printf("Setting brightness to %i\n", value);
    setBrightness(value);
    return;
}


void MainWindow::on_bufferSwapButton_clicked()
{
    printf("Swapping front/back buffers\n");
    swapBuffers();
    return;
}


void MainWindow::on_bufferGetPixelButton_clicked()
{
    printf("Getting pixel...\n");
    uint8_t x = ui->bufferXSpinBox->value();
    uint8_t y = ui->bufferYSpinBox->value();
    TargetBuffer target = (strcmp(ui->bufferTargetComboBox->currentText().toLatin1().data(), "Front")) ? TargetBuffer::Back : TargetBuffer::Front;
    if (target == TargetBuffer::Front) swapBuffers();
    uint8_t pixel = getPixel(x, y, TargetBuffer::Back);
    if (target == TargetBuffer::Front) swapBuffers();
    plogf("Pixel at %i, %i in %s buffer is %i (0x%02x).", x, y, (target == TargetBuffer::Front) ? "Front" : "Back", pixel, pixel);
    return;
}


void MainWindow::on_bufferSetPixelButton_clicked()
{
    printf("Setting pixel...\n");
    uint8_t x = ui->bufferXSpinBox->value();
    uint8_t y = ui->bufferYSpinBox->value();
    uint8_t color = ui->bufferColorSpinBox->value();
    TargetBuffer target = (strcmp(ui->bufferTargetComboBox->currentText().toLatin1().data(), "Front")) ? TargetBuffer::Back : TargetBuffer::Front;
    if (target == TargetBuffer::Front) swapBuffers();
    setPixel(x, y, TargetBuffer::Back, color);
    if (target == TargetBuffer::Front) swapBuffers();
    plogf("Set pixel at %i, %i in %s buffer to %i.", x, y, (target == TargetBuffer::Front) ? "Front" : "Back", color);
    return;
}


void MainWindow::on_bufferClearFrontButton_clicked()
{
    plogf("Clearing front buffer...");
    swapBuffers(); // These commands only seem to work on the back buffer, so we move the front to the back.
    solidFillRect(0, 0, B_WIDTH, B_HEIGHT, TargetBuffer::Front, 0);
    swapBuffers(); // Now we move it back to the front.
    plogf("Cleared front buffer!");
    return;
}


void MainWindow::on_bufferClearBackButton_clicked()
{
    plogf("Clearing back buffer...");
    solidFillRect(0, 0, B_WIDTH, B_HEIGHT, TargetBuffer::Back, 0);
    plogf("Cleared back buffer!");
    return;
}


void MainWindow::on_bufferDrawTestButton_clicked()
{
    plogf("Drawing test pattern...");
    uint8_t buf[144] = {
        0b00011011, 0b10010001, 0b10111001, 0b00011011, 0b10010001, 0b10111001, 0b00011011, 0b10010001, 0b10111001, 0b00011011, 0b10010001, 0b10111011,
        0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000011,
        0b00000000, 0b00000000, 0b00000000, 0b00110000, 0b00111100, 0b00000000, 0b00000000, 0b00001111, 0b00001111, 0b11000011, 0b00000000, 0b00000011,
        0b01000000, 0b00110011, 0b00000000, 0b00000000, 0b11110011, 0b00000000, 0b00000000, 0b00111100, 0b11000000, 0b00110011, 0b00000000, 0b00000010,
        0b01000000, 0b00110011, 0b00000000, 0b00110000, 0b11110011, 0b00110000, 0b00000011, 0b00111100, 0b11000000, 0b00110011, 0b00000000, 0b00000010,
        0b01000000, 0b00000000, 0b00000000, 0b00110000, 0b11110011, 0b00110000, 0b00000011, 0b00111100, 0b11000000, 0b00110011, 0b00000000, 0b00000010,
        0b10000000, 0b11000000, 0b11000000, 0b00110000, 0b11010111, 0b00110000, 0b00000011, 0b00110101, 0b11000011, 0b11000011, 0b00000000, 0b00000001,
        0b10000000, 0b00111111, 0b00000000, 0b00110000, 0b11001111, 0b00110000, 0b11000011, 0b00110011, 0b11000000, 0b00110011, 0b00000000, 0b00000001,
        0b10000000, 0b00000000, 0b00000000, 0b00110000, 0b11001111, 0b00110000, 0b11000011, 0b00110011, 0b11000000, 0b00110011, 0b00000000, 0b00000001,
        0b11000000, 0b00000000, 0b00000000, 0b00110000, 0b11001111, 0b00001100, 0b11001100, 0b00110011, 0b11000000, 0b00110011, 0b00000000, 0b00000000,
        0b11000000, 0b00000000, 0b00000000, 0b11000000, 0b00111100, 0b00000011, 0b00110000, 0b00001111, 0b00001111, 0b11000000, 0b11000000, 0b00000000,
        0b11000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
    };
    fillRect(0, 0, B_WIDTH, B_HEIGHT, TargetBuffer::Back, buf, 144);
    swapBuffers();
    plogf("Finished drawing test pattern!");
    return;
}


void MainWindow::on_settingsSetPowerOnImageButton_clicked()
{
    plogf("Setting power-on image...");
    setPowerOnImage();
    plogf("Set power-on image!");
    return;
}


void MainWindow::on_bufferCopyFrontToBackButton_clicked()
{
    plogf("Copying front buffer to back...");
    copyRect(0, 0, B_WIDTH, B_HEIGHT, 0, 0, TargetBuffer::Front, TargetBuffer::Back);
    plogf("Copied front buffer to back!");
    return;
}


void MainWindow::on_bufferCopyBackToFrontButton_clicked()
{
    plogf("Copying back buffer to front...");
    swapBuffers();
    copyRect(0, 0, B_WIDTH, B_HEIGHT, 0, 0, TargetBuffer::Front, TargetBuffer::Back);
    swapBuffers();
    plogf("Copied back buffer to front!");
    return;
}

void MainWindow::on_imageLoadImagesButton_clicked()
{
    QString filter = tr("PNG (*.png)");
    QStringList openPaths = QFileDialog::getOpenFileNames(this, "Open Images", "", filter, &filter);
    if (openPaths.isEmpty())
    {
        printf("No image files were selected, not setting.\n");
        return;
    }
    int openPathsCount = openPaths.size();
    if (!imagePaths.isEmpty())
    {
        QStringList().swap(imagePaths); // Empty our list by swapping it with a new one.
    }
    printf("Copying paths to imagePaths...\n");
    for (int i = 0; i < openPathsCount; i++)
    {
        imagePaths.append(openPaths[i]);
    }
    printf("Copying finished (%i / %i)\n", imagePaths.size(), openPathsCount);
    imagePreviewFrameCount = openPathsCount;
    imagePreviewCurrentFrame = 0;
    updateImagePreview(imagePreviewCurrentFrame);
    return;
}


void MainWindow::on_imageDrawImagesButton_clicked()
{
    if (!isSerialConnected())
    {
        plogf("You need to connect to a device first.");
        return;
    }
    int openPathsCount = imagePaths.size();
    if (openPathsCount == 0)
    {
        plogf("No images have been loaded, not displaying.");
        return;
    }
    if (!imageTmpPaths.isEmpty())
    {
        QStringList().swap(imageTmpPaths); // Empty our list by swapping it with a new one.
    }
    const std::intmax_t targetFPS = 30;
    plogf("Drawing images...");
    frameRate<targetFPS> frameRater;
    for (int i = 0; i < openPathsCount; i++)
    {
        qApp->processEvents();
        updateImagePreview(i);
        readPNGFile(imagePaths[i].toLatin1().data());
        reducePNGColors();
        uint8_t buf[144] = {0};
        pngMap(buf);
        fillRect(0, 0, B_WIDTH, B_HEIGHT, TargetBuffer::Back, buf, 144);
        swapBuffers();
        freeRowPtrs();
        frameRater.sleep();
    }
    plogf("Finished drawing!");
    printf("Removing temp images...\n");
    int tmpImageCount = imageTmpPaths.size();
    for (int i = 0; i < tmpImageCount; i++)
    {
        std::remove(imageTmpPaths[i].toLatin1().data());
    }
    return;
    printf("Done!\n");
}


void MainWindow::on_imagePreviewFrameNextButton_clicked()
{
    updateImagePreview(imagePreviewCurrentFrame + 1);
}


void MainWindow::on_imagePreviewFrameLastButton_clicked()
{
    updateImagePreview(imagePreviewFrameCount - 1);
}


void MainWindow::on_imagePreviewFramePrevButton_clicked()
{
    updateImagePreview(imagePreviewCurrentFrame - 1);
}


void MainWindow::on_imagePreviewFrameFirstButton_clicked()
{
    updateImagePreview(0);
}


void MainWindow::on_imageLoadAudioButton_clicked()
{
   QString path = QFileDialog::getOpenFileName(this, "Open Audio");
   if (path.isEmpty())
   {
       printf("No audio file was selected, not setting.\n");
       return;
   }
   strncpy(audioPath, path.toLatin1().data(), 65535);
}


void MainWindow::on_imageDrawAndPlayButton_clicked()
{
    if (!isSerialConnected())
    {
        plogf("You need to connect to a device first.");
        return;
    }
    int openPathsCount = imagePaths.size();
    if (openPathsCount == 0)
    {
        plogf("No images have been loaded, not displaying.");
        return;
    }
    if (audioPath[0] == '\0')
    {
        plogf("You need to load an audio file first. Use \"Draw Images\" if you don't want audio.");
        return;
    }
    if (!imageTmpPaths.isEmpty())
    {
        QStringList().swap(imageTmpPaths); // Empty our list by swapping it with a new one.
    }
    pid_t pid;
    int status;
    const std::intmax_t targetFPS = 30;
    plogf("Drawing images...");
    frameRate<targetFPS> frameRater;
    char* argv[] = {"ffplay", audioPath, (char*) 0};
    status = posix_spawn(&pid, "/usr/bin/ffplay", NULL, NULL, argv, environ);
    if (status != 0)
    {
        plogf("Cannot launch ffplay! Make sure that ffplay is installed and is in your PATH!");
        return;
    }
    printf("Started ffplay\n");
    for (int i = 0; i < openPathsCount; i++)
    {
        qApp->processEvents();
        updateImagePreview(i);
        readPNGFile(imagePaths[i].toLatin1().data());
        reducePNGColors();
        uint8_t buf[144] = {0};
        pngMap(buf);
        fillRect(0, 0, B_WIDTH, B_HEIGHT, TargetBuffer::Back, buf, 144);
        swapBuffers();
        freeRowPtrs();
        frameRater.sleep();
    }
    if (pid)
    {
        kill(pid, SIGQUIT);
        printf("Stopped ffplay\n");
    }
    plogf("Finished drawing!");
    printf("Removing temp images...\n");
    int tmpImageCount = imageTmpPaths.size();
    for (int i = 0; i < tmpImageCount; i++)
    {
        std::remove(imageTmpPaths[i].toLatin1().data());
    }
    printf("Done!\n");
    return;
}


void MainWindow::on_actionDumpToLog_triggered()
{
    QSysInfo sysInfo;
    QString prettyProductName = sysInfo.prettyProductName();
    QString currentCpuArchitecture = sysInfo.currentCpuArchitecture();
    QString kernelType = sysInfo.kernelType();
    QString kernelVersion = sysInfo.kernelVersion();

    plogf("========== LedBadgeQt Debug ==========");
    plogf("[ System Information ]");
    plogf("OS: %s (%s)", detectedOS, prettyProductName.toLatin1().data());
    plogf("Kernel: %s %s", kernelType.toLatin1().data(), kernelVersion.toLatin1().data());
    plogf("CPU Architecture: %s", currentCpuArchitecture.toLatin1().data());

    QString buildAbi = sysInfo.buildAbi();

    plogf("\n[ Build Info ]");
    plogf("Build stamp: %s at %s", __DATE__, __TIME__);
    plogf("Built for: %s", buildAbi.toLatin1().data());
}


void MainWindow::on_webcamOpenButton_clicked()
{
    int devID = ui->webcamDeviceIDSpinBox->value();
    plogf("Opening webcam %i...", devID);

    webcam.open(devID);
    webcam.setScale(48, 12);

    QImage previewImage = webcam.getFrame();
    QPixmap previewPixmap = QPixmap::fromImage(previewImage);
    ui->WebcamPreviewFrameImageLabel->setPixmap(previewPixmap);
}


void MainWindow::on_webcamDrawButton_clicked()
{
    if (!isSerialConnected())
    {
        plogf("You need to connect to a device first.");
        return;
    }
    const std::intmax_t targetFPS = 30;
    plogf("Drawing images...");
    frameRate<targetFPS> frameRater;
    while (true)
    {
        qApp->processEvents();

        QImage previewImage = webcam.getFrame();
        QPixmap previewPixmap = QPixmap::fromImage(previewImage);
        ui->WebcamPreviewFrameImageLabel->setPixmap(previewPixmap);

        /*
        readPNGFile(imagePaths[i].toLatin1().data());
        reducePNGColors();
        */

        uint8_t buf[144] = {0};
        memImageMap(buf, previewImage.bits(), 48, 12);
        fillRect(0, 0, B_WIDTH, B_HEIGHT, TargetBuffer::Back, buf, 144);
        swapBuffers();
        //freeRowPtrs();

        frameRater.sleep();
    }
    plogf("Finished drawing!");
    printf("Removing temp images...\n");
    int tmpImageCount = imageTmpPaths.size();
    for (int i = 0; i < tmpImageCount; i++)
    {
        std::remove(imageTmpPaths[i].toLatin1().data());
    }
    return;
    printf("Done!\n");
}

