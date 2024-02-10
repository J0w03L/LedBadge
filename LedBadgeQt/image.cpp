#include "image.h"

int width, height;
png_byte colorType;
png_byte bitDepth;
png_bytep *rowPtrs = NULL;

int readPNGFile(char *path)
{
    FILE *pngFile = fopen(path, "rb");
    //printf("file descriptor: %i\n", fileno(pngFile));
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) return -1;
    png_infop pngInfo = png_create_info_struct(png);
    if (!pngInfo) return -2;
    if (setjmp(png_jmpbuf(png))) return -3;

    // Read metadata.
    png_init_io(png, pngFile);
    png_read_info(png, pngInfo);
    width       = png_get_image_width(png, pngInfo);
    height      = png_get_image_height(png, pngInfo);
    colorType   = png_get_color_type(png, pngInfo);
    bitDepth    = png_get_bit_depth(png, pngInfo);

    if (bitDepth == 16)                                     png_set_strip_16(png);                  // Reduce 16-bit depth to 8-bit depth.
    if (colorType == PNG_COLOR_TYPE_PALETTE)                png_set_palette_to_rgb(png);            // Convert palleted images to RGB.
    if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8)   png_set_expand_gray_1_2_4_to_8(png);    // Convert 1/2/4-bit grayscale to 8-bit.
    if (colorType & PNG_COLOR_MASK_ALPHA)                   png_set_strip_alpha(png);               // Remove alpha channels.
    if (colorType & PNG_COLOR_MASK_COLOR)                   png_set_rgb_to_gray(png, 2, -1, -1);    // Reduce to grayscale.

    png_read_update_info(png, pngInfo);

    // Read PNG.
    if (rowPtrs) return -4;
    rowPtrs = (png_bytep*) malloc(sizeof(png_bytep) * height);
    for (int y = 0; y < height; y++)
    {
        rowPtrs[y] = (png_byte*) malloc(png_get_rowbytes(png, pngInfo));
    }
    png_read_image(png, rowPtrs);
    png_destroy_read_struct(&png, &pngInfo, NULL);
    fclose(pngFile);

    //int flushed = fflush(pngFile);
    //int closed = fclose(pngFile);
    //if (closed != 0) printf("closed: %i\n", closed);
    //printf("flushed: %i\nclosed: %i\n", flushed, closed);
    return 0;
}

int pngMap(uint8_t* buf)
{
    if (!rowPtrs) return -1;
    for (int y = 0; y < height; y++)
    {
        png_bytep row = rowPtrs[y];
        for (int x = 0; x < (width / 4); x++)
        {
            for (int i = 0; i < 4; i++)
            {
                png_bytep px = &(row[x * 4 + i]);
                if (px[0] < 85)
                {
                    if (px[0] >= 43)
                    {
                        buf[(y * 12) + x] |= 0b01000000 >> (i * 2);
                        continue;
                    }
                    buf[(y * 12) + x] |= 0b00000000 >> (i * 2);
                    continue;
                }
                if (px[0] < 170)
                {
                    if (px[0] >= 128)
                    {
                        buf[(y * 12) + x] |= 0b10000000 >> (i * 2);
                        continue;
                    }
                    buf[(y * 12) + x] |= 0b01000000 >> (i * 2);
                    continue;
                }
                if (px[0] < 255)
                {
                    if (px[0] >= 213)
                    {
                        buf[(y * 12) + x] |= 0b11000000 >> (i * 2);
                        continue;
                    }
                    buf[(y * 12) + x] |= 0b10000000 >> (i * 2);
                    continue;
                }
                buf[(y * 12) + x] |= 0b11000000 >> (i * 2);
                continue;
            }
        }
    }
    return 0;
}

int memImageMap(uint8_t* dst, uint8_t* src, size_t width, size_t height)
{
    for (int y = 0; y < height; y++)
    {
        uint8_t* row = (uint8_t*)calloc(sizeof(uint8_t), width);
        memcpy(row, &src[width * y], width);
        for (int x = 0; x < (width / 4); x++)
        {
            for (int i = 0; i < 4; i++)
            {
                uint8_t px = row[x * 4 + i];
                if (px < 85)
                {
                    if (px >= 43)
                    {
                        dst[(y * height) + x] |= 0b01000000 >> (i * 2);
                        continue;
                    }
                    dst[(y * height) + x] |= 0b00000000 >> (i * 2);
                    continue;
                }
                if (px < 170)
                {
                    if (px >= 128)
                    {
                        dst[(y * height) + x] |= 0b10000000 >> (i * 2);
                        continue;
                    }
                    dst[(y * height) + x] |= 0b01000000 >> (i * 2);
                    continue;
                }
                if (px < 255)
                {
                    if (px >= 213)
                    {
                        dst[(y * height) + x] |= 0b11000000 >> (i * 2);
                        continue;
                    }
                    dst[(y * height) + x] |= 0b10000000 >> (i * 2);
                    continue;
                }
                dst[(y * height) + x] |= 0b11000000 >> (i * 2);
                continue;
            }
        }
        free(row);
    }
    return 0;
}

int reducePNGColors()
{
    if (!rowPtrs) return -1;
    for (int y = 0; y < height; y++)
    {
        png_bytep row = rowPtrs[y];
        for (int x = 0; x < width; x++)
        {
            png_bytep px = &(row[x]);
            /*
             * This is a quick and dirty implementation; but it should work for our needs.
            */
            if (px[0] < 85)
            {
                if (px[0] >= 43)
                {
                    px[0] = 85;
                    continue;
                }
                px[0] = 0;
                continue;
            }
            if (px[0] < 170)
            {
                if (px[0] >= 128)
                {
                    px[0] = 170;
                    continue;
                }
                px[0] = 85;
                continue;
            }
            if (px[0] < 255)
            {
                if (px[0] >= 213)
                {
                    px[0] = 255;
                    continue;
                }
                px[0] = 170;
                continue;
            }
            px[0] = 255;
            continue;
        }
    }
    return 0;
}

int writePNGFile(char *path)
{
    FILE *pngFile = fopen(path, "wb");
    if (!pngFile) return -1;
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) return -2;
    png_infop pngInfo = png_create_info_struct(png);
    if (!pngInfo) return -3;
    if (setjmp(png_jmpbuf(png))) return -4;

    // Set metadata.
    png_init_io(png, pngFile);
    png_set_IHDR(png, pngInfo, width, height, 8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png, pngInfo);

    if (!rowPtrs) return -5;

    png_write_image(png, rowPtrs);
    //png_write_end(png, NULL);
    png_write_end(png, pngInfo);
    freeRowPtrs();
    png_destroy_write_struct(&png, &pngInfo);
    int closed = fclose(pngFile);
    printf("tmp closed: %i\n", closed);
    return 0;
}

void freeRowPtrs()
{
    for (int y = 0; y < height; y++)
    {
        free(rowPtrs[y]);
    }
    free(rowPtrs);
    rowPtrs = NULL;
    return;
}
