# LedBadge
An attempt at creating a cross-platform version of LedBadge using Qt6.

## Current Features
### Linux
 - [x] GUI
 - [x] Serial Communication
 - [ ] Commands:
    - [x] Ping
    - [x] GetVersion
    - [x] Swap
    - [x] PollInputs
    - [x] SetBrightness
    - [x] SetPixel
    - [x] GetPixel
    - [x] GetPixelRect
    - [x] SolidFill
    - [x] Fill
    - [x] Copy
    - [x] SetPowerOnImage
    - [ ] SetHoldTimings

### Windows
TODO.

## Drivers
### Linux
Most Linux distros since 2005 should already have drivers for badges using the PL2303TA installed. The Linux PL2303TA drivers can be found [here](https://github.com/torvalds/linux/blob/master/drivers/usb/serial/pl2303.c).

### Windows
I have uploaded the drivers (and original software) that came with my badge to the Internet Archive [here](https://archive.org/details/a16nf-drivers-6.40). They should work on any badge that uses the PL2303TA, though the software included in that archive may not necessarily be compatible with them.

**Important to note for users using Windows 11 or later:** the drivers that Windows will install automatically upon connecting a PL2303TA badge to your computer do NOT work! To get around this issue, install the Windows 8 version of drivers included in the above-mentioned archive. If you encounter issues on other versions of Windows, this is also worth giving a try as the newer versions of the PL2303TA driver may be at fault.

## Notes
Development starts from the v1.0 release of LedBadge, because it's the only version that works with the B1248 badge that I have.
I intend to at least try to bring some of the newer features over to this though!

