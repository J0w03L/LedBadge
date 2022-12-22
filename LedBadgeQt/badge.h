#ifndef BADGE_H
#define BADGE_H

#define B_WIDTH             48U
#define B_HEIGHT            12U
#define B_BPPXL             2U
#define B_CVALS             4U
#define B_PXLPB             8U / B_BPPXL
#define B_FRMSTD            B_WIDTH * B_PXLPB
#define B_FRMSIZE           B_FRMSTD * B_HEIGHT
#define B_IFRMSTD           B_WIDTH
#define B_IFRMSIZE          B_IFRMSTD * B_HEIGHT

#endif // BADGE_H
