#include "serial.h"

int serialDevice = 0;

int openSerialDevice(const char* deviceName)
{
    char devicePath[256] = {'/', 'd', 'e', 'v', '/', '\0'};
    strncat(devicePath, deviceName, 256);
    serialDevice = open(devicePath, O_RDWR);
    if (serialDevice < 0)
    {
        printf("Error opening serial device at \"%s\": (%i) %s\n", devicePath, errno, strerror(errno));
        return -1;
    }
    struct termios2 tty;
    if (ioctl(serialDevice, TCGETS2, &tty) != 0)
    {
        printf("Error getting attributes: (%i) %s\n", errno, strerror(errno));
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
    tty.c_cc[VTIME] = 80;                                                           // Wait to receive data for up to 8 seconds.
    tty.c_cc[VMIN] = 0;                                                             // Return all bytes received.
    tty.c_cflag &= ~CBAUD;                                                          // Baud speed bit mask.
    tty.c_cflag |= CBAUDEX;                                                         // Extra baud bit speed mask.
    tty.c_ispeed = 128000;                                                          // Use baud speed of 128000 for input.
    tty.c_ospeed = 128000;                                                          // Use baud speed of 128000 for output.

    if (ioctl(serialDevice, TCSETS2, &tty) != 0)
    {
        printf("Error setting attributes: (%i) %s\n", errno, strerror(errno));
        return -3;
    }
    /*

    /*
     * Now we send the sync sequence
    */
    printf("Sending sync sequence\n");
    uint8_t syncSeq1[256] = {0};
    uint8_t syncSeq2[1] = {2U << 4}; // GetVersion command.
    write(serialDevice, syncSeq1, sizeof(syncSeq1));
    write(serialDevice, syncSeq2, sizeof(syncSeq2));
    uint8_t verBuf[1] = {255}; // 255 is a placeholder value.
    printf("Sending GetVersion\n");
    int verRead = read(serialDevice, &verBuf, sizeof(verBuf));
    printf("GetVersion result: %i (%02X)\n", verRead, verRead);
    return 0;
}
