void	  PASCAL DownLineLoadFont(LPPDEVICE, LPFX);
void	  PASCAL SelectFont(LPPDEVICE, LPFX);

short FAR PASCAL PutFontInline(LPPDEVICE, LPEXTTEXTDATA, short);
DWORD FAR PASCAL ShowStr(LPPDEVICE,int,int,LPSTR,int,LPFX,LPDRAWMODE,LPFT,short FAR *);
int   FAR PASCAL TrackKern(LPPDEVICE, LPFX, int);
void  FAR PASCAL OpaqueBox(LPPDEVICE lpdv, int x, int y, int dx, int dy, RGB color);
