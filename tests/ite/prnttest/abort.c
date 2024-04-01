/*---------------------------------------------------------------------------*\
| PRINTER ABORT                                                               |
|   This module contains the printer Abort procedure.  It is used so that     |
|   Windows may still process while the application is printing.              |
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : June 05, 1989                                                      |
| SEGMENT: _TEXT                                                              |
|                                                                             |
| HISTORY: Jun 20, 1989 - moved segments around to maximize app performance.  |
|          Oct 18, 1989 - added more comments.                                |
|                                                                             |
\*---------------------------------------------------------------------------*/

#include <windows.h>
#include "PrntTest.h"

/*---------------------------------------------------------------------------*\
| PRINT ABORT PROCEDURE                                                       |
|   This routine handles the message dispatching when the application is      |
|   printing.  It checks the message queue for any messages to process.       |
|   If the user responds to the ABORT dialog, then Printing is halted.        |
|                                                                             |
| CALLED ROUTINES                                                             |
|   -none-                                                                    |
|                                                                             |
| PARAMETERS                                                                  |
|   HDC   hDC   - Handle to the Printer Device Context.                       |
|   short nCode - Code passed to AbortProc indicating status info.            |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|  BOOL bAbort    - If set to TRUE, then begin abort-print process.           |
|  HWND hAbortDlg - Determines if Dlg windows exists.                         |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - TRUE indicates that the user hasn't aborted printing.  FALSE       |
|          indicates printing should continue.                                |
\*---------------------------------------------------------------------------*/
BOOL FAR PASCAL PrintAbortProc(hDC, nCode)
     HDC   hDC;
     short nCode;
{
     extern BOOL bAbort;                         /* If TRUE, then abort      */
     extern HWND hAbortDlg;                      /* Handle to abort dialog   */

     MSG msg;                                    /* Message Structure        */

     /*-----------------------------------------*\
     | Retreive messages until none, or user     |
     | aborts print test.                        |
     \*-----------------------------------------*/
     while(!bAbort && PeekMessage(&msg,NULL,0,0,PM_REMOVE))
     {
          if(!hAbortDlg || !IsDialogMessage(hAbortDlg,&msg))
          {
               TranslateMessage(&msg);
               DispatchMessage(&msg);
          }
     }

     return(!bAbort);
}


/*---------------------------------------------------------------------------*\
| PRINT ABORT DIALOG PROCEDURE                                                |
|   This routine is handles the processing of the Abort Dialog Box.  It       |
|   It displays a message and waits for user to cancel job.                   |
|                                                                             |
| CALLED ROUTINES                                                             |
|   -none-                                                                    |
|                                                                             |
| PARAMETERS                                                                  |
|   HWND     hDlg     - The Window Handle.                                    |
|   unsigned iMessage - Message to be processed.                              |
|   WORD     wParam   - Information associated with message.                  |
|   LONG     lParam   - Information associated with message.                  |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|  BOOL bAbort    - Set to TRUE if user hits OK to cancel.                    |
|  HWND hAbortDlg - Set to NULL once the dialog window is destroyed.          |
|                                                                             |
| RETURNS                                                                     |
|   BOOL - TRUE indicates the message was processed, otherwise a FALSE        |
|          is returned.                                                       |
\*---------------------------------------------------------------------------*/
BOOL FAR PASCAL AbortDlg(hDlg, iMessage, wParam, lParam)
     HWND     hDlg;
     unsigned iMessage;
     WORD     wParam;
     LONG     lParam;
{
     extern BOOL bAbort;
     extern HWND hAbortDlg;

     switch(iMessage)
     {
          /*------------------------------------*\
          | Initialize Window upon entering      |
          | the dialog procedure.                |
          \*------------------------------------*/
          case WM_INITDIALOG:
               SetWindowText(hDlg,"Windows Printer Device Test");
               EnableMenuItem(GetSystemMenu(hDlg,FALSE),SC_CLOSE,MF_GRAYED);
               break;

          /*------------------------------------*\
          | If user hits any KEY-->Quit Dialog.  |
          | Set bAbort=TRUE, and hAbortDlg=NULL. |
          \*------------------------------------*/
          case WM_COMMAND:
               bAbort = TRUE;
               EnableWindow(GetParent(hDlg),TRUE);
               DestroyWindow(hDlg);
               hAbortDlg = NULL;
               break;

          /*------------------------------------*\
          | Message wasn't processed.            |
          \*------------------------------------*/
          default:
               return(FALSE);
     }

     return(TRUE);
}
