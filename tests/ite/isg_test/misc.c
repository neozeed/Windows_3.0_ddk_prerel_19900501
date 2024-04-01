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
| EXTENDED TEXTOUT RECT TEST                                                  |
|   This routine outputs text along a clipping/opaquing rectangle.            |
\*---------------------------------------------------------------------------*/
BOOL FAR PASCAL TstExtTextOutRect(hDC,nMode,lpRect,lpString,nStrPos,lpDX)
     HDC    hDC;
     short  nMode;
     LPRECT lpRect;
     LPSTR  lpString;
     short  nStrPos;
     LPINT  lpDX;
{
     short  nRectHeight,nRectWidth,nStrHeight,nStrWidth,nStrX,nStrY;
     HBRUSH hOldBrush;
     HPEN   hOldPen;

     nRectHeight = lpRect->bottom - lpRect->top;
     nRectWidth  = lpRect->right - lpRect->left;
     nStrHeight  = HIWORD(GetTextExtent(hDC,lpString,lstrlen(lpString)));
     nStrWidth   = LOWORD(GetTextExtent(hDC,lpString,lstrlen(lpString)));

     switch(nStrPos)
     {
          case 0:
               nStrX = lpRect->left - (nStrWidth >> 1);
               nStrY = lpRect->top - (nStrHeight >> 1);
               break;
          case 1:
               nStrX = lpRect->left;
               nStrY = lpRect->top - (nStrHeight >> 1);
               break;
          case 2:
               nStrX = lpRect->right - (nStrWidth >> 1);
               nStrY = lpRect->top - (nStrHeight >> 1);
               break;
          case 3:
               nStrX = lpRect->left - (nStrWidth >> 1);
               nStrY = lpRect->top;
               break;
          case 4:
               nStrX = lpRect->left;
               nStrY = lpRect->top;
               break;
          case 5:
               nStrX = lpRect->right - (nStrWidth >> 1);
               nStrY = lpRect->top;
               break;
          case 6:
               nStrX = lpRect->left - (nStrWidth >> 1);
               nStrY = lpRect->top + (nRectHeight >> 1);
               break;
          case 7:
               nStrX = lpRect->left;
               nStrY = lpRect->top + (nRectHeight >> 1);
               break;
          case 8:
               nStrX = lpRect->right - (nStrWidth >> 1);
               nStrY = lpRect->top + (nRectHeight >> 1);
               break;
          default:
               return(FALSE);
     }

     ExtTextOut(hDC,nStrX,nStrY,nMode,lpRect,lpString,lstrlen(lpString),lpDX);

     hOldBrush = SelectObject(hDC,GetStockObject(NULL_BRUSH));
     hOldPen = SelectObject(hDC,GetStockObject(BLACK_PEN));
     Rectangle(hDC,lpRect->left,lpRect->top,lpRect->right,lpRect->bottom);
     SelectObject(hDC,hOldBrush);
     SelectObject(hDC,hOldPen);

     return(TRUE);
}


/*---------------------------------------------------------------------------*\
| OUTPUT DRAW OBJECT TEST                                                     |
|   This routine outputs text along a clipping/opaquing rectangle.            |
\*---------------------------------------------------------------------------*/
BOOL FAR PASCAL TstDrawObject(hDC,x,y,dx,dy,wObject)
     HDC   hDC;
     short x;
     short y;
     short dx;
     short dy;
     WORD  wObject;
{
     HPEN       hOldPen;
     HBRUSH     hOldBrush;
     POINT      pStar[5];
     int        nOldPolyFill,nOldBack,nOldRopCode;

     /*-----------------------------------------*\
     | Set the modes for the gray rectangle and  |
     | save the selected stuff for the object.   |
     \*-----------------------------------------*/
     nOldBack    = SetBkMode(hDC,OPAQUE);
     nOldRopCode = SetROP2(hDC,R2_COPYPEN);
     hOldBrush   = SelectObject(hDC,GetStockObject(GRAY_BRUSH));
     hOldPen     = SelectObject(hDC,GetStockObject(BLACK_PEN));

     /*-----------------------------------------*\
     | Output the gray rectangle to be 1/2 the   |
     | width of the object.                      |
     \*-----------------------------------------*/
     Rectangle(hDC,x+(dx >> 1),y,x+dx,y+dy);

     /*-----------------------------------------*\
     | Re-select the brush, pen and moded for    |
     | the object.                               |
     \*-----------------------------------------*/
     SelectObject(hDC,hOldBrush);
     SelectObject(hDC,hOldPen);
     SetROP2(hDC,nOldRopCode);
     SetBkMode(hDC,nOldBack);

     /*-----------------------------------------*\
     | Draw the object the user chose with the   |
     | call.                                     |
     \*-----------------------------------------*/
     switch(wObject)
     {
          case OBJ_ARC:
               Arc(hDC,x,y,x+dx,y+dy,x,y,x+dx,y);
               break;

          case OBJ_CHORD:
               Chord(hDC,x,y,x+dx,y+dy,x,y,x+dx,y);
               break;

          case OBJ_ELLIPSE:
               Ellipse(hDC,x,y,x+dx,y+dy);
               break;

          case OBJ_LINE:
               MoveTo(hDC,x,y);
               LineTo(hDC,x+dx,y+dy);
               break;

          case OBJ_PIE:
               Pie(hDC,x,y,x+dx,y+dy,x,y,x+dx,y);
               break;

          case OBJ_POLYGON:
               pStar[0].x = x+(dx >> 1);
               pStar[0].y = y;
               pStar[1].x = x+dx;
               pStar[1].y = y+dy;
               pStar[2].x = x;
               pStar[2].y = y+(dy/3);
               pStar[3].x = x+dx;
               pStar[3].y = y+(dy/3);
               pStar[4].x = x;
               pStar[4].y = y+dy;
               Polygon(hDC,pStar,5);
               break;

          case OBJ_POLYLINE:
               pStar[0].x = x+(dx >> 1);
               pStar[0].y = y;
               pStar[1].x = x+dx;
               pStar[1].y = y+dy;
               pStar[2].x = x;
               pStar[2].y = y+(dy/3);
               pStar[3].x = x+dx;
               pStar[3].y = y+(dy/3);
               pStar[4].x = x;
               pStar[4].y = y+dy;
               Polyline(hDC,pStar,5);
               break;

          case OBJ_RECTANGLE:
               Rectangle(hDC,x,y,x+dx,y+dy);
               break;

          case OBJ_ROUNDRECT:
               RoundRect(hDC,x,y,x+dx,y+dy,10,10);
               break;

          default:
               return(FALSE);
     }

     return(TRUE);
}

/*---------------------------------------------------------------------------*\
| OUTPUT GRAYSCALE TEST                                                       |
|   This routine outputs text along a clipping/opaquing rectangle.            |
\*---------------------------------------------------------------------------*/
BOOL FAR PASCAL TstGrayScale(hDC,x,y,dx,dy)
     HDC hDC;
     short x;
     short y;
     short dx;
     short dy;
{
     register int nIdx;
     HBRUSH       hBrush;
     RECT         rect;

     /*-----------------------------------------*\
     | Set the dimensions of the output to a     |
     | 255x100 Coordinate system.                |
     \*-----------------------------------------*/
     SaveDC(hDC);
     SetMapMode(hDC,MM_ANISOTROPIC);
     SetWindowExt(hDC,255,128);
     SetViewportExt(hDC,dx,-dy);
     SetViewportOrg(hDC,x,y+dy);

     /*-----------------------------------------*\
     | Output 256 rectangles, each with a gray-  |
     | scale brush.                              |
     \*-----------------------------------------*/
     for(nIdx=0; nIdx < 256; nIdx++)
     {
          SetRect(&rect,nIdx,0,nIdx+1,128);
          hBrush = CreateSolidBrush(RGB(nIdx,nIdx,nIdx));
          FillRect(hDC,&rect,hBrush);
          DeleteObject(hBrush);
          if(((nIdx+16) % 16) == 0)
          {
               MoveTo(hDC,nIdx,0);
               LineTo(hDC,nIdx,-2);
          }
     }
     MoveTo(hDC,255,0);
     LineTo(hDC,255,-2);

     hBrush = SelectObject(hDC,GetStockObject(NULL_BRUSH));
     Rectangle(hDC,0,0,255,128);
     SelectObject(hDC,hBrush);

     RestoreDC(hDC,-1);

     return(TRUE);
}

/*---------------------------------------------------------------------------*\
| OUTPUT BITBLT ROP TEST                                                      |
|   This routine outputs text along a clipping/opaquing rectangle.            |
\*---------------------------------------------------------------------------*/
BOOL FAR PASCAL TstBitBltRop(hDC,xStart,yStart,nHeight,nWidth,hSrc,hDst,hPat,dwROP,bVerify)
     HDC    hDC;
     short  xStart;
     short  yStart;
     short  nHeight;
     short  nWidth;
     HBRUSH hSrc;
     HBRUSH hDst;
     HBRUSH hPat;
     DWORD  dwROP;
     BOOL   bVerify;
{
     HBITMAP        hSrcBit;
     HBRUSH         hOldBrush;
     register short nTestWidth,nTestHeight;

     /*-----------------------------------------*\
     | Create the bitmaps for the test.          |
     \*-----------------------------------------*/
     nTestWidth  = nWidth / 3;
     nTestHeight = nHeight >> 1;

     /*-----------------------------------------*\
     | Output the sample bitmaps to device.      |
     \*-----------------------------------------*/
     hOldBrush = SelectObject(hDC,hSrc);
     PatBlt(hDC,xStart,yStart,nTestWidth,nTestHeight,PATCOPY);
     SelectObject(hDC,hDst);
     PatBlt(hDC,xStart+nTestWidth,yStart,nTestWidth,nTestHeight,PATCOPY);
     PatBlt(hDC,xStart,yStart+nTestHeight,(nTestWidth*3),nTestHeight,PATCOPY);
     SelectObject(hDC,hPat);
     PatBlt(hDC,xStart+(2*nTestWidth),yStart,nTestWidth,nTestHeight,PATCOPY);

     /*-----------------------------------------*\
     | Output the ROP test bitmaps.  Pat brush   |
     | is currently selected.                    |
     \*-----------------------------------------*/
     if(hSrcBit = CreateBrushBitmap(hDC,(nTestWidth*3),nTestHeight,hSrc))
     {
          OutputDDBToDevice(hDC,xStart,yStart+nTestHeight,hSrcBit,dwROP);
          SelectObject(hDC,hOldBrush);
          DeleteObject(hSrcBit);
          return(TRUE);
     }
     SelectObject(hDC,hOldBrush);

     return(FALSE);
}

BOOL FAR PASCAL TstColorMapping(hDC,dwColor)
     HDC      hDC;
     COLORREF dwColor;
{
     HBITMAP hbmBase,hbmMap;
     BOOL    bPass;

     bPass = FALSE;
     if(hbmBase = CreateRGBBitmap(hDC,1,1,dwColor))
     {
          if(hbmMap = CreateRGBBitmap(hDC,1,1,GetNearestColor(hDC,dwColor)))
          {
               bPass = CompareBitmaps(hbmBase,hbmMap);
               DeleteObject(hbmMap);
          }
          DeleteObject(hbmBase);
     }
     return(bPass);
}
