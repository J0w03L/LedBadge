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
int reducePNGColors();
void freeRowPtrs();

#endif // IMAGE_H
