/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								       */
/* Create Date  : 2015/3/18 by hailong.dang								          */
/* Abstract     : None                                     			          */
/* Reference    : None														             */
/* Note         :                                                           */
/*		(ubuntu) apt-get install libX11-dev                                   */
/*		         apt-get install libjpeg-dev                                  */
/*		gcc -o ScreenshotHelper ScreenshotHelperL.c -lX11 -ljpeg              */
/****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <jpeglib.h>
#include "ScreenshotL.h"

/*
 * Global variables
 */
static int bytesPerLine;

static CARD8 bitsPerPixel;
static CARD16 redMax, greenMax, blueMax;
static CARD8  redShift, greenShift, blueShift;
static int byteOrder;
static char *jpegBeforeBuf = NULL;

/***************************************************************
Find least significant bit
****************************************************************/
int FindLSB(int word)
{
  int t = word;

  int m = 1;
  int i = 0;

  for (; i < sizeof(word) << 3; i++, m <<= 1)
  {
    if (t & m)
    {
      return i + 1;
    }
  }

  return 0;
}

/***************************************************************
Parse XImage data to a buffer
****************************************************************/
void PrepareRowForJpeg(CARD8 *dst, int y, int count)
{
  if (bitsPerPixel == 32)
  {
    if (redMax == 0xff &&
            greenMax == 0xff &&
                blueMax == 0xff)
    {
      PrepareRowForJpeg24(dst, y, count);
    }
    else
    {
      PrepareRowForJpeg32(dst, y, count);
    }
  }
  else if (bitsPerPixel == 24)
  {
    memcpy(dst, jpegBeforeBuf + y * bytesPerLine, count * 3);
  }
  else
  {
    /*
     * 16 bpp assumed.
     */
    PrepareRowForJpeg16(dst, y, count);
  }
}

void PrepareRowForJpeg24(CARD8 *dst, int y, int count)
{
  CARD8 *fbptr;
  CARD32 pix;

  fbptr = (CARD8 *) (jpegBeforeBuf + y * bytesPerLine);

  while (count--)
  {
    if (byteOrder == LSBFirst)
    {
      pix = (CARD32) *(fbptr + 2);
      pix = (pix << 8) | (CARD32) *(fbptr+1);
      pix = (pix << 8) | (CARD32) *fbptr;
    }
    else
    {
      pix = (CARD32) *(fbptr + 1);
      pix = (pix << 8) | (CARD32) *(fbptr + 2);
      pix = (pix << 8) | (CARD32) *(fbptr + 3);
    }

    *dst++ = (CARD8)(pix >> redShift);
    *dst++ = (CARD8)(pix >> greenShift);
    *dst++ = (CARD8)(pix >> blueShift);

    fbptr+=4;
  }
}

#define DEFINE_JPEG_GET_ROW_FUNCTION(bpp)                                   \
                                                                            \
void PrepareRowForJpeg##bpp(CARD8 *dst, int y, int count)                   \
{                                                                           \
  CARD8 *fbptr;                                                             \
  CARD##bpp pix;                                                            \
  int inRed, inGreen, inBlue;                                               \
  int i;                                                                    \
                                                                            \
  fbptr = (CARD8 *) (jpegBeforeBuf + y * bytesPerLine);                     \
                                                                            \
  while (count--)                                                           \
  {                                                                         \
    pix = 0;                                                                \
                                                                            \
    if (byteOrder == LSBFirst)                                              \
    {                                                                       \
      for (i = (bpp >> 3) - 1; i >= 0; i--)                                 \
      {                                                                     \
        pix = (pix << 8) | (CARD32) *(fbptr + i);                           \
      }                                                                     \
    }                                                                       \
    else                                                                    \
    {                                                                       \
      for (i = 0; i < (bpp >> 3); i++)                                      \
      {                                                                     \
        pix = (pix << 8) | (CARD32) *(fbptr + i);                           \
      }                                                                     \
    }                                                                       \
                                                                            \
    fbptr += bpp >> 3;                                                      \
                                                                            \
    inRed = (int)                                                           \
            (pix >> redShift   & redMax);                                   \
    inGreen = (int)                                                         \
            (pix >> greenShift & greenMax);                                 \
    inBlue  = (int)                                                         \
            (pix >> blueShift  & blueMax);                                  \
                                                                            \
    *dst++ = (CARD8)((inRed   * 255 + redMax / 2) /                         \
                         redMax);                                           \
    *dst++ = (CARD8)((inGreen * 255 + greenMax / 2) /                       \
                         greenMax);                                         \
    *dst++ = (CARD8)((inBlue  * 255 + blueMax / 2) /                        \
                         blueMax);                                          \
  }                                                                         \
}

DEFINE_JPEG_GET_ROW_FUNCTION(16)
DEFINE_JPEG_GET_ROW_FUNCTION(32)

/***************************************************************
Write XImage data to a JPEG file

Must include <jpeglib.h>
Return value:
0 - failed
1 - success
****************************************************************/
int JpegWriteFileFromImage(char *filename, XImage* img)
{
	FILE* fp;
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	fp = fopen(filename,"wb");
	if(fp==NULL)
	{
		return 0;
	}

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	jpeg_stdio_dest(&cinfo,fp);
	cinfo.image_width = img->width;
	cinfo.image_height = img->height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;

	jpeg_set_defaults(&cinfo);
	jpeg_start_compress(&cinfo,TRUE);

	JSAMPROW row_pointer[1];       /* pointer to scanline */
	unsigned char* pBuf = (unsigned char*)malloc(cinfo.image_width*cinfo.input_components);
	row_pointer[0] = pBuf;

	printf("XImage width=%d, height=%d\n", img->width, img->height);
	printf("byte_order=%d, depth=%d\n", img->byte_order, img->depth);
	printf("bytes_per_line=%d, bits_per_pixel=%d\n", img->bytes_per_line, img->bits_per_pixel);
	printf("red_mask=0x%x, green_mask=0x%x, blue_mask=0x%x\n", img->red_mask, img->green_mask, img->blue_mask);

	bitsPerPixel = img->bits_per_pixel;
	bytesPerLine = img->bytes_per_line;
	byteOrder    = img->byte_order;

	redShift   = FindLSB(img->red_mask)   - 1;
	greenShift = FindLSB(img->green_mask) - 1;
	blueShift  = FindLSB(img->blue_mask)  - 1;

	redMax   = img->red_mask   >> redShift;
	greenMax = img->green_mask >> greenShift;
	blueMax  = img->blue_mask  >> blueShift;

	jpegBeforeBuf = img->data;

	while(cinfo.next_scanline < cinfo.image_height)
	{
		PrepareRowForJpeg(pBuf, cinfo.next_scanline, img->width);
		jpeg_write_scanlines(&cinfo,row_pointer,1);
	}

	free(pBuf);
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	fclose(fp);
	return 1;
}

/*****************************************************************
Capture a local screenshot of the desktop,
saving to the file specified by filename.
If type = 0, then write a bitmap file, else
write a JPEG file.

Return Value:
0 - fail
1 - success
******************************************************************/
int CaptureDesktop(char* filename)
{
	Window desktop;
	Display* dsp;
	XImage* img;

	int screen_width;
	int screen_height;

	//dsp = XOpenDisplay(NULL);       /* Connect to a local display */
	dsp = XOpenDisplay(":0");
	if(NULL==dsp)
	{
		return 0;
	}
	desktop = RootWindow(dsp,0);/* Refer to the root window */
	if(0==desktop)
	{
		return 0;
	}

	/* Retrive the width and the height of the screen */
	screen_width = DisplayWidth(dsp,0);
	screen_height = DisplayHeight(dsp,0);

	/* Get the Image of the root window */
	img = XGetImage(dsp,desktop,0,0,screen_width,screen_height,~0,ZPixmap);

	JpegWriteFileFromImage(filename,img);

	XDestroyImage(img);
	XCloseDisplay(dsp);
	return 1;
}

/*****************************************************************
Screenshot full window

Return Value:
0 - success
other - fail
******************************************************************/
int ScreenshotFullWindow(char * fileName)
{
	int iRet = -1;
	if(fileName == NULL) return iRet;
	if(CaptureDesktop(fileName)) iRet = 0;
	return iRet;
}
