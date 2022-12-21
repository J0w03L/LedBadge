#ifndef SERIAL_H
#define SERIAL_H

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <string>
#include <vector>
#include <iostream>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <asm/termbits.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

struct SerialDevice
{
    uint        id;            // Identifier.
    bool        isUSB;         // Is this a ttyUSB device?
    bool        isNull;        // Is this a null device?
    char*       displayName;   // What name should this device have in the device list?
    char*       devName;       // What is the name of this device in /dev/?
};

int openSerialDevice(const char* deviceName);

#endif // SERIAL_H
