#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "serial.h"
#include "log.h"
#include <algorithm>
#include <cstring>

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
                        strncat(img, bit1 ? "\u2592" : "\u2591", 4); // 01 = Medium Shade; 00 = Light Shade (to keep character spacing equal).
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

