/*---------------------------------------------------------------------------*\
| MISC MODULE                                                                 |
|   This module contains miscellaneous routines which can be used for testing |
|   purposes.                                                                 |
|                                                                             |
| OBJECT                                                                      |
|   (----)                                                                    |
|                                                                             |
| METHODS                                                                     |
|   TstExtTextOutRect()                                                       |
|   TstDrawObject()                                                           |
|   TstGrayScale()                                                            |
|   TstBitBltRop()                                                            |
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : January 22, 1990                                                   |
| SEGMENT: _MISC                                                              |
\*---------------------------------------------------------------------------*/

#include <windows.h>
#include "isg_test.h"

/*---------------------------------------------------------------------------*\
| VERIFY PIXEL POINT.                                                         |
|   This routine outputs text along a clipping/opaquing rectangle.            |
\*---------------------------------------------------------------------------*/
BOOL FAR PASCAL VerifyPixelColor(hDC,x,y,rgbVerify)
     HDC      hDC;
     int      short x;
     int      short y;
     COLORREF rgbVerify;
{
     HBRUSH  hBrush;
     HBITMAP hVerify,hPoint;

     /*-----------------------------------------*\
     | Create a bitmap representation of the     |
     | rgbColor passed to this function.         |
     \*-----------------------------------------*/
     hBrush  = CreateSolidBrush(rgbVerify);
     hVerify = CreateBrushBitmap(hDC,1,1,hBrush);
     DeleteObject(hBrush);

     hPoint = CreatePixelBitmap(hDC,x,y);

     if(!CompareBitmaps(hVerify,hPoint))
     {
          DeleteObject(hVerify);
          DeleteObject(hPoint);
          return(FALSE);
     }

     DeleteObject(hVerify);
     DeleteObject(hPoint);

     return(TRUE);
}
