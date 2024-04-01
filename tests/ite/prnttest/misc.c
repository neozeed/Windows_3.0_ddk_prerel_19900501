/*---------------------------------------------------------------------------*\
| MISC ROUTINES                                                               |
|   This module contains miscellaneous routines which can be used throughout  |
|   the entire application.  These functions are more than not, common to     |
|   more than one segment.  It is desirable to place these routines in a      |
|   segment which is always loaded.                                           |
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : June 22, 1989                                                      |
| SEGMENT: _TEXT                                                              |
|                                                                             |
| HISTORY: Jun 20, 1989 - moved segments around to maximize app performance.  |
|          Sep 03, 1989 - look into problems RandyG has with createDC.        |
|                                                                             |
\*---------------------------------------------------------------------------*/

#include <windows.h>
#include "PrntTest.h"

