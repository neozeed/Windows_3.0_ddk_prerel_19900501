        parmD   lpDstDev
        parmW   DstxOrg
        parmW   DstyOrg
        parmD   lpClipRect
        parmD   lpString
        parmW   Count
        parmD   lpFont
        parmD   lpDrawMode
        parmD   lpTextForm
        parmD   lpCharWidths
        parmD   lpOpaqueRect
	parmW	Options
;       
	localD	lpPBrush
	localW	pwOldFontSize
	localW	pwFontInfoTable
	localW	pwOldFaceName
	localW	pwOldFontHeight
	localW	wFaceNameLength
        localW  FontHeight
        localW  PointSize
        localW  xExt
	localW	LocDstxOrg		;used in (P)STRBLT.ASM
	localW	CMSDstxOrg		;used in STRCOLOR.ASM
	localW	CMSDstyOrg		;used in STRCOLOR.ASM
	localW	wDestX			;used in (P)GETFONT.ASM
	localW	wDestY			;used in (P)GETFONT.ASM
	localW	wByteCount		;used in (P)GETFONT.ASM
        localW  PropSpaceFlag
        localW  BreakExtra
        localW  BreakErr
        localW  BreakRem
        localW  BreakCount
        localW  CharExtra
        localW  BoardDstDev
        localW  ClippedY
        localW  yCoordinate
        localW  FontStorageyExt
	localW	StrColouryCoordinate
	localW	SegmentIndex
	localW	WidthBytes
        localB  DstType
        localB  SystemFontFlag
        localB  FirstChar
        localB  BreakChar
	localB	LastChar
	localB	DefaultChar
	localB	bRemainderNeeded	;used in (P)GETFONT.ASM
        localB  BackgroundColour
        localB  ForegroundColour
	localB	OpaqueFlag
	localB	SystemFontPresent
	localB	WritePlane
	localB	bNewFont
        localV  MinimumClipRect,8
