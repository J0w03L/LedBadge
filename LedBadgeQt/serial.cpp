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

/*
 * Currently this does not work correctly:
 *  - Most of the time too much data is read for buf1 and/or buf2.
 *  - Even when the correct amount of data is read for each buffer, the pixels are parsed incorrectly.
*/
int getPixelRect(uint8_t* buf1, uint8_t* buf2, uint8_t* buf3, uint8_t x, uint8_t y, uint8_t width, uint8_t height, TargetBuffer target)
{
    printf("getPixelRect()\n");
    if (!(x >= 0 && x < B_WIDTH && width > 0 && (x + width) <= B_WIDTH && y >= 0 && y < B_HEIGHT && height > 0 && (y + height) <= B_HEIGHT)) // I really need to make an assert function bruh
    {
        return -1;
    }
    uint8_t cmd1[1] = {SerialCommand::GetPixelRect | target};
    uint8_t cmd2[1] = {x};
    uint8_t cmd3[1] = {width};
    uint8_t cmd4[1] = {(y << 4U) | height };
    write(serialDevice, cmd1, sizeof(cmd1));
    write(serialDevice, cmd2, sizeof(cmd2));
    write(serialDevice, cmd3, sizeof(cmd3));
    write(serialDevice, cmd4, sizeof(cmd4));
    int buf1Read = read(serialDevice, buf1, sizeof(buf1));
    int buf2Read = read(serialDevice, buf2, sizeof(buf2));
    int buf3Read = 0;
    uint8_t tmp;
    for (int i = 0; i < (width * height) / 4; i++)
    {
        buf3Read += read(serialDevice, &tmp, sizeof(tmp));
        buf3[i] = tmp;
    }
    printf("buf1Read : %i\nbuf2Read : %i\n buf3Read : %i\n", buf1Read, buf2Read, buf3Read);
    return 0;
}
