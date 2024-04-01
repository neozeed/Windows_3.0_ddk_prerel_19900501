// DESKJET.H
//
// Copyright (C) 1989-1990 Microsoft Corporation.
//  All rights reserved.

/* DeskJet Family defines and type definitions. */

typedef struct
	{
	BYTE left;
	BYTE width;
	BYTE right;
	WORD total;
	} WIDTH_TABLE;

#define CLASS_LASERJET        0
#define CLASS_DESKJET         1
#define CLASS_DESKJET_PLUS    2
#define LASERJET_FONT         0
#define DESKJET_FONT          5
#define DESKJET_PLUS_FONT     9
