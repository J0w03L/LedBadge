#ifndef IMAGE_H
#define IMAGE_H

#include "log.h"
#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <cmath>

int readPNGFile(char* path);
int writePNGFile(char* path);
int pngMap(uint8_t* buf);
int memImageMap(uint8_t* dst, uint8_t* src, size_t width, size_t height);
int reducePNGColors();
void freeRowPtrs();

#endif // IMAGE_H
