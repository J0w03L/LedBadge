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
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_devRefreshButton_clicked()
{
    logf(ui->logTextBrowser, "Refreshing devices...");
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
    logf(ui->logTextBrowser, "Refreshed devices!");
    return;
}

void MainWindow::on_devConnectButton_clicked()
{
    // TODO: Disconnect previous connection if there was one?
    QString currentDeviceNameQString = ui->devComboBox->currentText();
    QByteArray currentDeviceNameQByteArray = currentDeviceNameQString.toLatin1();
    const char *currentDeviceName = currentDeviceNameQByteArray.data();
    if (currentDeviceName[0] == '\0')
    {
        logf(ui->logTextBrowser, "Please select a device to connect to first!");
        return;
    }
    plogf(ui->logTextBrowser, "Connecting to serial device...");
    int e = openSerialDevice(currentDeviceName, ui->logTextBrowser);
    if (e != 0)
    {
        plogf(ui->logTextBrowser, "Could not connect to serial device! (openSerialDevice returned %i)", e);
        return;
    }
    plogf(ui->logTextBrowser, "Connected to serial device successfully!");
    return;
}


void MainWindow::on_logTestButton_clicked()
{
    printf("Printing test string to log.\n");
    char* currentDeviceName = ui->devComboBox->currentText().toLatin1().data();
    printf("currentDeviceName: '%s'\n", currentDeviceName);
    logf(ui->logTextBrowser, "<span>Current selected device: <b>%s</b></span>", (currentDeviceName[0] != '\0') ? currentDeviceName : "None");
}


void MainWindow::on_logClearButton_clicked()
{
    printf("Clearing log.\n");
    ui->logTextBrowser->clear();
}


void MainWindow::on_queryPingButton_clicked()
{
    plogf(ui->logTextBrowser, "Pinging device...");
    uint8_t buf;
    int i = pingDevice(&buf);
    if (i == 0)
    {
        plogf(ui->logTextBrowser, "Received no reply!");
        return;
    }
    plogf(ui->logTextBrowser, "Received reply: %i (0x%02x)", buf, buf);
    return;
}


void MainWindow::on_queryVersionButton_clicked()
{
    plogf(ui->logTextBrowser, "Querying firmware version...");
    uint8_t buf;
    int i = getVersion(&buf);
    if (i == 0)
    {
        plogf(ui->logTextBrowser, "Couldn't query firmware version; received no reply!");
        return;
    }
    plogf(ui->logTextBrowser, "Firmware Version: %i (0x%02x)", buf, buf);
    return;
}


void MainWindow::on_queryPollInputsButton_clicked()
{
    plogf(ui->logTextBrowser, "Querying input states...");
    uint8_t buf;
    int i = pollInputs(&buf);
    if (i == 0)
    {
        plogf(ui->logTextBrowser, "Couldn't query input states; received no reply!");
        return;
    }
    plogf(ui->logTextBrowser, "K1: %i | K2: %i", BIT_TEST(buf, 0) ? 1 : 0, BIT_TEST(buf, 1) ? 1 : 0);
    return;
}


void MainWindow::on_queryGetImageButton_clicked()
{
    plogf(ui->logTextBrowser, "NOTE: This function does not work correctly yet!");
    plogf(ui->logTextBrowser, "Getting screen image...");
    uint8_t buf1;
    uint8_t buf2;
    uint8_t buf3[(B_WIDTH * B_HEIGHT) / 4];
    if (int e = getPixelRect(&buf1, &buf2, buf3, 0, 0, B_WIDTH, B_HEIGHT, TargetBuffer::Front) != 0)
    {
        plogf(ui->logTextBrowser, "Couldn't get screen image! (returned %i)", e);
        return;
    }
    plogf(ui->logTextBrowser, "buf1 : 0x%02x | buf2: 0x%02x", buf1, buf2);
    for (int y = 0; y < 12; y++)
    {
        int x = 0;
        while (x < 12)
        {
            for (int p = 0; p < 4; p++)
            {
                printf("%s", BIT_TEST(buf3[(y * 12) + x], p) ? "\u2588" : " ");
            }
            x++;
        }
        printf("\n");
    }
    return;
}

