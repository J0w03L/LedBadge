#ifndef SERIAL_H
#define SERIAL_H

//#define NDEBUG // Comment to disable assertions.

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
#include "log.h"
#include "badge.h"

#define BIT_TEST(byte, bit) (byte & (1U << bit))

struct SerialDevice
{
    uint        id;                 // Identifier.
    bool        isUSB;              // Is this a ttyUSB device?
    bool        isNull;             // Is this a null device?
    char*       displayName;        // What name should this device have in the device list?
    char*       devName;            // What is the name of this device in /dev/?
};

enum SerialCommand : uint8_t
{
    Nop             = 0U  << 4,     // No Operation.
    Ping            = 1U  << 4,     // Ping the device.
    GetVersion      = 2U  << 4,     // Get the version of the device's firmware.
    Swap            = 3U  << 4,     // Swaps the front/back buffers.
    PollInputs      = 4U  << 4,     // Check the state of the device's buttons.
    SetBrightness   = 5U  << 4,     // Set the device's screen brightness.
    SetPixel        = 6U  << 4,     // Set pixel state.
    GetPixel        = 7U  << 4,     // Get pixel state.
    GetPixelRect    = 8U  << 4,     // Get state of pixels in rectangle.
    SolidFill       = 9U  << 4,     // Fill pixels solid.
    Fill            = 10U << 4,     // Fill pixels.
    Copy            = 11U << 4,     // Copy pixels in rectangle.
    SetPowerOnImage = 12U << 4,     // Set the image shown on the device's startup.
    SetHoldTimings  = 13U << 4      // Set the hold timings.
};

enum TargetBuffer : uint8_t
{
    Back    = 0U << 2,
    Front   = 1U << 2
};

int openSerialDevice(const char* deviceName);
bool isSerialConnected();
int closeSerialDevice();
int getVersion(uint8_t* buf);
int pingDevice(uint8_t* buf);
int pollInputs(uint8_t* buf);
int getPixelRect(uint8_t* buf, uint8_t x, uint8_t y, uint8_t width, uint8_t height, TargetBuffer target);
int setBrightness(int brightness);
int swapBuffers();
int getPixel(uint8_t x, uint8_t y, TargetBuffer target);
int setPixel(uint8_t x, uint8_t y, TargetBuffer target, uint8_t color);
int solidFillRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, TargetBuffer target, uint8_t color);
int fillRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, TargetBuffer target, uint8_t* buf, size_t len);
int copyRect(uint8_t srcX, uint8_t srcY, uint8_t width, uint8_t height, uint8_t dstX, uint8_t dstY, TargetBuffer src, TargetBuffer dst);
int setPowerOnImage();

#endif // SERIAL_H
