#include "serial.h"

int serialDevice = 0;

int openSerialDevice(const char* deviceName, QTextBrowser* logTextBrowser)
{
    char devicePath[262] = "/dev/";
    strncat(devicePath, deviceName, 256);
    serialDevice = open(devicePath, O_RDWR);
    if (serialDevice < 0)
    {
        plogf(logTextBrowser, "Error opening serial device at \"%s\": (%i) %s", devicePath, errno, strerror(errno));
        return -1;
    }
    struct termios2 tty;
    if (ioctl(serialDevice, TCGETS2, &tty) != 0)
    {
        plogf(logTextBrowser, "Error getting attributes: (%i) %s", errno, strerror(errno));
        return -2;
    }

    /*
     * Here we configure our serial port.
    */

    tty.c_cflag &= ~PARENB;                                                         // No parity bit.
    tty.c_cflag &= ~CSTOPB;                                                         // One stop bit.
    tty.c_cflag &= ~CSIZE;                                                          // Clear data size flags.
    tty.c_cflag &= ~CRTSCTS;                                                        // Disable RTS/CTS.
    tty.c_cflag |= CS8;                                                             // 8 data bits.
    tty.c_cflag |= CREAD;                                                           // Enable read.
    tty.c_cflag |= CLOCAL;                                                          // Ignore modem signals.
    tty.c_lflag &= ~ICANON;                                                         // Disable canonical mode.
    tty.c_lflag &= ~(ECHO | ECHONL | ECHOE);                                        // Disable echo.
    tty.c_lflag &= ~ISIG;                                                           // Don't interpret INTR/QUIT/SUSP.
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);                                         // Disable software flow control.
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);    // Disable handling so we get raw bytes.
    tty.c_oflag &= ~(OPOST | ONLCR);                                                // Disable handling so we send raw bytes.
    tty.c_cc[VTIME] = 50;                                                           // Wait to receive data for up to 5 seconds.
    tty.c_cc[VMIN] = 0;                                                             // Return all bytes received.
    tty.c_cflag &= ~CBAUD;                                                          // Baud speed bit mask.
    tty.c_cflag |= CBAUDEX;                                                         // Extra baud bit speed mask.
    tty.c_ispeed = 128000;                                                          // Use baud speed of 128000 for input.
    tty.c_ospeed = 128000;                                                          // Use baud speed of 128000 for output.

    if (ioctl(serialDevice, TCSETS2, &tty) != 0)
    {
        plogf(logTextBrowser, "Error setting attributes: (%i) %s", errno, strerror(errno));
        return -3;
    }

    /*
     * Now we send the sync sequence
    */
    plogf(logTextBrowser, "Sending sync sequence");
    SerialCommand syncSeq1[256] = {SerialCommand::Nop};
    write(serialDevice, syncSeq1, sizeof(syncSeq1));
    plogf(logTextBrowser, "Sending GetVersion");
    uint8_t verBuf;
    int verRead = getVersion(&verBuf);
    plogf(logTextBrowser, "GetVersion result: %i (0x%02x, received %i bytes)", verBuf, verBuf, verRead);
    if (verBuf == 255 || verBuf == 0) return -4;
    return 0;
}

int getVersion(uint8_t* buf)
{
    printf("getVersion()\n");
    SerialCommand cmd[1] = {SerialCommand::GetVersion};
    write(serialDevice, cmd, sizeof(cmd));
    return read(serialDevice, buf, sizeof(buf));
}

int pingDevice(uint8_t* buf)
{
    printf("pingDevice()\n");
    SerialCommand cmd[1] = {SerialCommand::Ping};
    write(serialDevice, cmd, sizeof(cmd));
    return read(serialDevice, buf, sizeof(buf));
}

int pollInputs(uint8_t* buf)
{
    printf("pollInputs()\n");
    SerialCommand cmd[1] = {SerialCommand::PollInputs};
    write(serialDevice, cmd, sizeof(cmd));
    return read(serialDevice, buf, sizeof(buf));
}

int getPixelRect(uint8_t* buf, uint8_t x, uint8_t y, uint8_t width, uint8_t height, TargetBuffer target)
{
    printf("getPixelRect()\n");
    if (!(x >= 0 && x < B_WIDTH && width > 0 && (x + width) <= B_WIDTH && y >= 0 && y < B_HEIGHT && height > 0 && (y + height) <= B_HEIGHT)) // I really need to make an assert function bruh
    {
        return -1;
    }
    uint8_t tmp;
    uint8_t bufRead = 0;
    uint8_t cmd[4] = {SerialCommand::GetPixelRect | target, x, width, (y << 4) | height};
    write(serialDevice, cmd, sizeof(cmd));
    for (int i = 0; i < ((width * height) / 4) + 2; i++)
    {
        bufRead += read(serialDevice, &tmp, sizeof(tmp));
        buf[i] = tmp;
    }
    printf("bufRead : %i\n", bufRead);
    return 0;
}

int setBrightness(int brightness)
{
    printf("setBrightness()\n");
    uint8_t cmd[2] = {SerialCommand::SetBrightness, static_cast<uint8_t>(brightness)};
    write(serialDevice, cmd, sizeof(cmd));
    return 0;
}

int swapBuffers()
{
    printf("swapBuffers()\n");
    uint8_t cmd[1] = {SerialCommand::Swap};
    write(serialDevice, cmd, sizeof(cmd));
    return 0;
}
