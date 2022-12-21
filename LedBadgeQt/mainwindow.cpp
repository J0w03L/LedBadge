#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "serial.h"
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
}

void MainWindow::on_devConnectButton_clicked()
{
    // TODO: Disconnect previous connection if there was one?
    QString currentDeviceNameQString = ui->devComboBox->currentText();
    QByteArray currentDeviceNameQByteArray = currentDeviceNameQString.toLatin1();
    const char *currentDeviceName = currentDeviceNameQByteArray.data();
    openSerialDevice(currentDeviceName);

}

