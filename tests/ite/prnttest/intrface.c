/*---------------------------------------------------------------------------*\
| WINDOWS DIALOG INTERFACE MODULE                                             |
|   This module contains the routines necessary for handling the modeless     |
|   dialog box which acts as the interface for the application.  All routines |
|   which the interface (DialogBox) uses, are contained in this module.       |
|                                                                             |
| AUTHOR : Christopher Williams -ChrisWil-                                    |
| DATE   : July 01, 1989                                                      |
| SEGMENT: _TEXT                                                              |
|                                                                             |
| HISTORY: Jul 01, 1989 - created the interface module.                       |
|                                                                             |
\*---------------------------------------------------------------------------*/

#include <windows.h>
#include "PrntTest.h"

/*---------------------------------------------------------------------------*\
| INTERFACE DIALOG PROCEDURE                                                  |
|   This is the main interface module for the application.  It provides the   |
|   control for the Modeless DialogBox created and displayed throughout the   |
|   existence of the application.  The interface consists of the following:   |
|                                                                             |
|      2 ListBoxes   - one contains a list of the printer device lines which  |
|                      the user can select to test.  The other contains a list|
|                      of printer device lines which are to be tested.        |
|      4 PushButtons - These boxes allow the user to ADD, REMOVE, MODIFY and  |
|                      setup the printer device lines.                        |
|      4 EditBoxes   - These allow the user to view and change the text       |
|                      content of the printer device lines.                   |
|      1 Static Text - This box displays description text depending upon the  |
|                      currently selected box or button in the dialog box.    |
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
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   TRUE if successful.                                                       |
\*---------------------------------------------------------------------------*/
BOOL FAR PASCAL PrntTestDlg(hDlg, iMessage, wParam, lParam)
     HWND hDlg;
     unsigned iMessage;
     WORD     wParam;
     LONG     lParam;
{
     extern HANDLE hInst;

     char szBuffer[80];

     switch(iMessage)
     {
          case WM_INITDIALOG:
               InitializeInterface(hDlg);
               break;

          case WM_COMMAND:
               switch(wParam)
               {
                    case IDD_INTRFACE_ADD:
                         if(!AddProfiles(hDlg))
                              MessageBox(hDlg,"Adding profiles (intrface.c)","Assertion",MB_OK);
                         break;

                    case IDD_INTRFACE_REM:
                         if(!RemoveProfiles(hDlg))
                              MessageBox(hDlg,"Removing profiles (intrface.c)","Assertion",MB_OK);
                         break;

                    case IDD_INTRFACE_SET:
                         if(!SetupPrinter(hDlg))
                              MessageBox(hDlg,"Setting profiles (intrface.c)","Assertion",MB_OK);
                         break;

                    case IDD_INTRFACE_MOD:
                         if(!ModifyProfiles(hDlg))
                              MessageBox(hDlg,"Modifying profiles (intrface.c)","Assertion",MB_OK);
                         break;

                    /*--------------------------*\
                    | Never let the test listbox |
                    | show a selection.          |
                    \*--------------------------*/
                    case IDD_INTRFACE_TEST:
                         if(HIWORD(lParam) != LBN_SELCHANGE)
                              return(TRUE);
                         SendDlgItemMessage(hDlg,IDD_INTRFACE_TEST,LB_SETCURSEL,
                              -1,0l);
                         SetFocus(GetDlgItem(hDlg,IDD_INTRFACE_LIST));
                         break;

                    /*--------------------------*\
                    | If the listbox selection   |
                    | changes, then the edit ctrl|
                    | boxes should be updated.   |
                    \*--------------------------*/
                    case IDD_INTRFACE_LIST:
                         if(HIWORD(lParam) != LBN_SELCHANGE)
                              return(TRUE);
                         UpdateSelectionChange(hDlg);
                         break;

                    /*--------------------------*\
                    | If the edit boxes get the  |
                    | input focus, then output   |
                    | the text description to    |
                    | the status box.            |
                    \*--------------------------*/
                    case IDD_INTRFACE_PROF:
                    case IDD_INTRFACE_NAME:
                    case IDD_INTRFACE_DRIV:
                    case IDD_INTRFACE_PORT:
                         switch(HIWORD(lParam))
                         {
                              case EN_SETFOCUS:
                                   LoadString(hInst,IDS_INTRFACE_PROF+(wParam-IDD_INTRFACE_PROF),
                                        szBuffer,sizeof(szBuffer));
                                   SetDlgItemText(hDlg,IDD_INTRFACE_TXT,(LPSTR)szBuffer);
                                   break;
                              case EN_KILLFOCUS:
                                   SetDlgItemText(hDlg,IDD_INTRFACE_TXT,(LPSTR)"\0");
                                   break;
                         }
                         break;

                    Default:
                         return(FALSE);
               }
               break;

          default:
               return(FALSE);
     }

     return(TRUE);
}


/*---------------------------------------------------------------------------*\
| INITIALIZE INTERFACE BOXES (variables)                                      |
|   This routine looks reads in the information contained in the PRNTTEST.INI |
|   file to update the LIST and TEST listboxes.                               |
|                                                                             |
| CALLED ROUTINES                                                             |
|   -none-                                                                    |
|                                                                             |
| PARAMETERS                                                                  |
|   HWND hDlg - The dialog window handle.                                     |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   TRUE if all went successful.                                              |
\*---------------------------------------------------------------------------*/
BOOL InitializeInterface(hDlg)
     HWND hDlg;
{
     HANDLE hBuffer,hProfiles,hString;
     LPSTR  lpBuffer,lpProfiles,lpString,lpProfile;
     int    nCount,idx;
     static struct
          {
               char  szListName[15];
               WORD  wListID;
          }
          sListBoxes[] = {"ProfilesList",IDD_INTRFACE_LIST,
                          "ProfilesTest",IDD_INTRFACE_TEST};


     /*-----------------------------------------*\
     | Allocate buffers needed for initializing  |
     | the dialog box.                           |
     \*-----------------------------------------*/
     if(!(hProfiles = LocalAlloc(LHND,(WORD)512)))
          return(FALSE);
     if(!(hBuffer = LocalAlloc(LHND,(WORD)128)))
     {
          LocalFree(hProfiles);
          return(FALSE);
     }
     if(!(hString = LocalAlloc(LHND,(WORD)128)))
     {
          LocalFree(hBuffer);
          LocalFree(hProfiles);
          return(FALSE);
     }

     /*-----------------------------------------*\
     | Lock down the buffers.                    |
     \*-----------------------------------------*/
     if(!(lpProfiles = (LPSTR)LocalLock(hProfiles)))
     {
          LocalFree(hProfiles);
          LocalFree(hBuffer);
          LocalFree(hString);
          return(FALSE);
     }
     if(!(lpBuffer = (LPSTR)LocalLock(hBuffer)))
     {
          LocalUnlock(hProfiles);
          LocalFree(hProfiles);
          LocalFree(hBuffer);
          LocalFree(hString);
          return(FALSE);
     }
     if(!(lpString = (LPSTR)LocalLock(hString)))
     {
          LocalUnlock(hProfiles);
          LocalFree(hProfiles);
          LocalUnlock(hBuffer);
          LocalFree(hBuffer);
          LocalFree(hString);
          return(FALSE);
     }

     /*-----------------------------------------*\
     | Get ALL strings from the file sections,   |
     | and add to appropriate listboxes.         |
     \*-----------------------------------------*/
     for(idx=0; idx < 2; idx++)
     {
          nCount = GetPrivateProfileString((LPSTR)sListBoxes[idx].szListName,
                        NULL,(LPSTR)"\0",lpProfiles,512,(LPSTR)"PrntTest.ini");
          if(nCount <= 0)
          {
               LocalUnlock(hProfiles);
               LocalFree(hProfiles);
               LocalUnlock(hBuffer);
               LocalFree(hBuffer);
               LocalUnlock(hString);
               LocalFree(hString);
               return(FALSE);
          }

          lpProfile = lpProfiles;
          while(lpProfile)
          {
               GetPrivateProfileString((LPSTR)sListBoxes[idx].szListName,
                    lpProfile,(LPSTR)"\0",lpString,128,(LPSTR)"prnttest.ini");

               lstrcpy(lpBuffer,lpProfile);
               lstrcat(lpBuffer,(LPSTR)":");
               lstrcat(lpBuffer,lpString);
               SendDlgItemMessage(hDlg,sListBoxes[idx].wListID,LB_ADDSTRING,NULL,
                    (LONG)lpBuffer);

               while(*lpProfile++);
               if((int)(lpProfile-lpProfiles) >= nCount)
                    lpProfile = NULL;
          }
     }

     /*-----------------------------------------*\
     | Free up buffers.                          |
     \*-----------------------------------------*/
     LocalUnlock(hProfiles);
     LocalFree(hProfiles);
     LocalUnlock(hBuffer);
     LocalFree(hBuffer);
     LocalUnlock(hString);
     LocalFree(hString);

     /*-----------------------------------------*\
     | Update editboxes to reflect current sel.  |
     \*-----------------------------------------*/
     SendDlgItemMessage(hDlg,IDD_INTRFACE_TEST,LB_SETCURSEL,-1,0l);
     SendDlgItemMessage(hDlg,IDD_INTRFACE_LIST,LB_SETCURSEL,0,0l);
     SendMessage(hDlg,WM_COMMAND,IDD_INTRFACE_LIST,
          MAKELONG(GetDlgItem(hDlg,IDD_INTRFACE_LIST),LBN_SELCHANGE));

     return(TRUE);
}


/*---------------------------------------------------------------------------*\
| MODIFY PROFILE SELECTION                                                    |
|   This routine performs the modification of the currently selected profile  |
|   in the LIST ListBox.  If the profile indicated in the Edit box already    |
|   exist in the LIST, then no modification is performed.  If the modification|
|   is allowed, then both the LIST and TEST Listboxes are updated.            |
|                                                                             |
|   ALGORITHM                                                                 |
|     1. Get string from Profile EditBox.                                     |
|     2. Search LIST ListBox for Match                                        |
|        if(Match)                                                            |
|             MessageBox - Can't perform update, since it already exist.      |
|        else                                                                 |
|             Update the LIST and TEST listboxes with changes.                |
|     3. Set Selection to new modification string.                            |
|                                                                             |
| CALLED ROUTINES                                                             |
|   -none-                                                                    |
|                                                                             |
| PARAMETERS                                                                  |
|   HWND hDlg - The dialog window handle.                                     |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   TRUE if all went successful.                                              |
\*---------------------------------------------------------------------------*/
BOOL ModifyProfiles(hDlg)
     HWND hDlg;
{
     HANDLE hCompare,hBuffer,hProfile,hName,hDriver,hPort,hOldProfile;
     LPSTR  lpCompare,lpBuffer,lpProfile,lpName,lpDriver,lpPort,lpOldProfile;
     LPSTR  lpTmp;
     int    nProfile,nName,nDriver,nPort,nCount,idx;

     /*-----------------------------------------*\
     | Retrieve the profile from the Profile     |
     | EditBox.  Use buffer in local heap.       |
     \*-----------------------------------------*/
     nProfile  = (int)SendDlgItemMessage(hDlg,IDD_INTRFACE_PROF,EM_LINELENGTH,0,0l);
     if(!(hProfile  = LocalAlloc(LHND,(WORD)nProfile+1)))
          return(FALSE);
     if(!(lpProfile = (LPSTR)LocalLock(hProfile)))
     {
          LocalFree(hProfile);
          return(FALSE);
     }
     GetDlgItemText(hDlg,IDD_INTRFACE_PROF,lpProfile,nProfile+1);

     /*-----------------------------------------*\
     | Look for profile in the TEST list, if it  |
     | exists, then prompt that it can't be      |
     | changed.  Do only for modified edit ctrl. |
     \*-----------------------------------------*/
     if(SendDlgItemMessage(hDlg,IDD_INTRFACE_PROF,EM_GETMODIFY,NULL,0l))
     {
          /*------------------------------------*\
          | Alloc buffer for comparison search.  |
          \*------------------------------------*/
          if(!(hCompare  = LocalAlloc(LHND,(WORD)256)))
          {
               LocalUnlock(hProfile);
               LocalFree(hProfile);
               return(FALSE);
          }
          if(!(lpCompare = (LPSTR)LocalLock(hCompare)))
          {
               LocalUnlock(hProfile);
               LocalFree(hProfile);
               LocalFree(hCompare);
               return(FALSE);
          }

          /*------------------------------------*\
          | Search for the profile in LIST.      |
          \*------------------------------------*/
          nCount = (int)SendDlgItemMessage(hDlg,IDD_INTRFACE_LIST,LB_GETCOUNT,NULL,0l);
          for(idx=0; idx < nCount; idx++)
          {
               SendDlgItemMessage(hDlg,IDD_INTRFACE_LIST,LB_GETTEXT,idx,(LONG)lpCompare);
               if(SearchProfileString(lpCompare,lpProfile))
               {
                    MessageBox(GetParent(hDlg),(LPSTR)"This profile already exists.  It cannot be changed",NULL,MB_OK);
                    LocalUnlock(hProfile);
                    LocalUnlock(hCompare);
                    LocalFree(hProfile);
                    LocalFree(hCompare);
                    return(TRUE);
               }
          }
          LocalUnlock(hCompare);
          LocalFree(hCompare);
     }

     /*-----------------------------------------*\
     | Get Name from Driver Name Edit Box.       |
     \*-----------------------------------------*/
     nName = (int)SendDlgItemMessage(hDlg,IDD_INTRFACE_NAME,EM_LINELENGTH,0,0l);
     if(!(hName = LocalAlloc(LHND,(WORD)nName+1)))
     {
          LocalUnlock(hProfile);
          LocalFree(hProfile);
          return(FALSE);
     }

     /*-----------------------------------------*\
     | Get Driver from Edit Box.                 |
     \*-----------------------------------------*/
     nDriver = (int)SendDlgItemMessage(hDlg,IDD_INTRFACE_DRIV,EM_LINELENGTH,0,0l);
     if(!(hDriver = LocalAlloc(LHND,(WORD)nDriver+1)))
     {
          LocalUnlock(hProfile);
          LocalFree(hProfile);
          LocalFree(hName);
          return(FALSE);
     }

     /*-----------------------------------------*\
     | Get Driver Port from edit box.            |
     \*-----------------------------------------*/
     nPort = (int)SendDlgItemMessage(hDlg,IDD_INTRFACE_PORT,EM_LINELENGTH,0,0l);
     if(!(hPort = LocalAlloc(LHND,(WORD)nPort+1)))
     {
          LocalUnlock(hProfile);
          LocalFree(hProfile);
          LocalFree(hName);
          LocalFree(hDriver);
          return(FALSE);
     }

     if(!(lpName = (LPSTR)LocalLock(hName)))
     {
          LocalUnlock(hProfile);
          LocalFree(hProfile);
          LocalFree(hName);
          LocalFree(hDriver);
          LocalFree(hPort);
          return(FALSE);
     }
     GetDlgItemText(hDlg,IDD_INTRFACE_NAME,lpName,nName+1);

     if(!(lpDriver = (LPSTR)LocalLock(hDriver)))
     {
          LocalUnlock(hProfile);
          LocalFree(hProfile);
          LocalUnlock(hName);
          LocalFree(hName);
          LocalFree(hDriver);
          LocalFree(hPort);
          return(FALSE);
     }
     GetDlgItemText(hDlg,IDD_INTRFACE_DRIV,lpDriver,nDriver+1);

     if(!(lpPort = (LPSTR)LocalLock(hPort)))
     {
          LocalUnlock(hProfile);
          LocalFree(hProfile);
          LocalUnlock(hName);
          LocalFree(hName);
          LocalUnlock(hDriver);
          LocalFree(hDriver);
          LocalFree(hPort);
          return(FALSE);
     }
     GetDlgItemText(hDlg,IDD_INTRFACE_PORT,lpPort,nPort+1);

     /*-----------------------------------------*\
     | Create new string with modified strings   |
     | retrieved from edit boxes.                |
     \*-----------------------------------------*/
     if(!(hBuffer = LocalAlloc(LHND,(WORD)256)))
     {
          LocalUnlock(hProfile);
          LocalFree(hProfile);
          LocalUnlock(hName);
          LocalFree(hName);
          LocalUnlock(hDriver);
          LocalFree(hDriver);
          LocalUnlock(hPort);
          LocalFree(hPort);
          return(FALSE);
     }
     if(!(lpBuffer = (LPSTR)LocalLock(hBuffer)))
     {
          LocalUnlock(hProfile);
          LocalFree(hProfile);
          LocalUnlock(hName);
          LocalFree(hName);
          LocalUnlock(hDriver);
          LocalFree(hDriver);
          LocalUnlock(hPort);
          LocalFree(hPort);
          LocalFree(hBuffer);
          return(FALSE);
     }
     lstrcpy(lpBuffer,(LPSTR)lpProfile);
     lstrcat(lpBuffer,(LPSTR)":");
     lstrcat(lpBuffer,(LPSTR)lpName);
     lstrcat(lpBuffer,(LPSTR)",");
     lpTmp = lpDriver;
     while((*lpTmp != '.') && *lpTmp)
          lpTmp++;
     *lpTmp = '\0';
     AnsiUpper(lpDriver);
     SetDlgItemText(hDlg,IDD_INTRFACE_DRIV,lpDriver);
     lstrcat(lpBuffer,(LPSTR)lpDriver);
     lstrcat(lpBuffer,(LPSTR)",");
     lstrcat(lpBuffer,(LPSTR)lpPort);

     /*-----------------------------------------*\
     | Free up the edit profile strings.         |
     \*-----------------------------------------*/
     LocalUnlock(hName);
     LocalFree(hName);
     LocalUnlock(hDriver);
     LocalFree(hDriver);
     LocalUnlock(hPort);
     LocalFree(hPort);

     /*-----------------------------------------*\
     | Alloc and lock buffer for Old profile str.|
     \*-----------------------------------------*/
     if(!(hOldProfile = LocalAlloc(LHND,(WORD)128)))
     {
          LocalUnlock(hBuffer);
          LocalFree(hBuffer);
          LocalUnlock(hProfile);
          LocalFree(hProfile);
          return(FALSE);
     }
     if(!(lpOldProfile = (LPSTR)LocalLock(hOldProfile)))
     {
          LocalUnlock(hBuffer);
          LocalFree(hBuffer);
          LocalUnlock(hProfile);
          LocalFree(hProfile);
          LocalFree(hOldProfile);
          return(FALSE);
     }

     /*-----------------------------------------*\
     | Get old profile string from LIST box.     |
     \*-----------------------------------------*/
     if(!(hCompare = LocalAlloc(LHND,(WORD)256)))
     {
          LocalUnlock(hBuffer);
          LocalFree(hBuffer);
          LocalUnlock(hProfile);
          LocalFree(hProfile);
          LocalUnlock(hOldProfile);
          LocalFree(hOldProfile);
          return(FALSE);
     }
     if(!(lpCompare = (LPSTR)LocalLock(hCompare)))
     {
          LocalUnlock(hBuffer);
          LocalFree(hBuffer);
          LocalUnlock(hProfile);
          LocalFree(hProfile);
          LocalUnlock(hOldProfile);
          LocalFree(hOldProfile);
          LocalFree(hCompare);
          return(FALSE);
     }
     SendDlgItemMessage(hDlg,IDD_INTRFACE_LIST,LB_GETTEXT,
          (WORD)SendDlgItemMessage(hDlg,IDD_INTRFACE_LIST,LB_GETCURSEL,
                             NULL,0l),(LONG)lpCompare);
     ParseDeviceString(lpCompare,lpOldProfile,NULL,NULL,NULL);
     LocalUnlock(hCompare);
     LocalFree(hCompare);

     /*-----------------------------------------*\
     | Update the LIST box with new string.      |
     \*-----------------------------------------*/
     SendDlgItemMessage(hDlg,IDD_INTRFACE_LIST,LB_DELETESTRING,
          (WORD)SendDlgItemMessage(hDlg,IDD_INTRFACE_LIST,LB_GETCURSEL,
                             NULL,0l),0l);
     SendDlgItemMessage(hDlg,IDD_INTRFACE_LIST,LB_SETCURSEL,
          (WORD)SendDlgItemMessage(hDlg,IDD_INTRFACE_LIST,LB_ADDSTRING,NULL,
          (LONG)lpBuffer),0l);
     SetFocus(GetDlgItem(hDlg,IDD_INTRFACE_LIST));

     WritePrivateProfileString((LPSTR)"ProfilesList",lpOldProfile,
          (LPSTR)NULL,(LPSTR)"PrntTest.ini");
     WritePrivateProfileString((LPSTR)"ProfilesList",lpProfile,
          lpBuffer+lstrlen(lpProfile)+1,(LPSTR)"PrntTest.ini");

     /*-----------------------------------------*\
     | Look for string in TEST Box to update.    |
     \*-----------------------------------------*/
     if(!(hCompare = LocalAlloc(LHND,(WORD)256)))
     {
          LocalUnlock(hBuffer);
          LocalFree(hBuffer);
          LocalUnlock(hProfile);
          LocalFree(hProfile);
          LocalUnlock(hOldProfile);
          LocalFree(hOldProfile);
          return(FALSE);
     }
     if(!(lpCompare = (LPSTR)LocalLock(hCompare)))
     {
          LocalUnlock(hBuffer);
          LocalFree(hBuffer);
          LocalUnlock(hProfile);
          LocalFree(hProfile);
          LocalUnlock(hOldProfile);
          LocalFree(hOldProfile);
          LocalFree(hCompare);
          return(FALSE);
     }
     nCount = (int)SendDlgItemMessage(hDlg,IDD_INTRFACE_TEST,LB_GETCOUNT,NULL,0l);
     for(idx=0; idx < nCount; idx++)
     {
          SendDlgItemMessage(hDlg,IDD_INTRFACE_TEST,LB_GETTEXT,idx,(LONG)lpCompare);
          if(SearchProfileString(lpCompare,lpOldProfile))
          {
               SendDlgItemMessage(hDlg,IDD_INTRFACE_TEST,LB_DELETESTRING,idx,0l);
               SendDlgItemMessage(hDlg,IDD_INTRFACE_TEST,LB_ADDSTRING,NULL,(LONG)lpBuffer);

               WritePrivateProfileString((LPSTR)"ProfilesTest",lpOldProfile,
                    (LPSTR)NULL,(LPSTR)"PrntTest.ini");
               WritePrivateProfileString((LPSTR)"ProfilesTest",lpProfile,
                    lpBuffer+lstrlen(lpProfile)+1,(LPSTR)"PrntTest.ini");

               LocalUnlock(hBuffer);
               LocalFree(hBuffer);
               LocalUnlock(hProfile);
               LocalFree(hProfile);
               LocalUnlock(hOldProfile);
               LocalFree(hOldProfile);
               LocalUnlock(hCompare);
               LocalFree(hCompare);
               return(TRUE);
          }
     }

     LocalUnlock(hBuffer);
     LocalFree(hBuffer);
     LocalUnlock(hProfile);
     LocalFree(hProfile);
     LocalUnlock(hOldProfile);
     LocalFree(hOldProfile);
     LocalUnlock(hCompare);
     LocalFree(hCompare);

     return(TRUE);
}


/*---------------------------------------------------------------------------*\
| REMOVE PROFILE FROM LISTBOXES.                                              |
|   This routine removes the Selected profile from the test or list depending |
|   on whether the profile is included in the appropriate listbox.            |
|                                                                             |
| ALGORITHM                                                                   |
|   Get Profile to remove.                                                    |
|   Look in TEST Box to remove.                                               |
|     Exist?                                                                  |
|       Remove from TEST Box and profile.                                     |
|     Not Exist?                                                              |
|       Look in LIST Box to remove.                                           |
|         Exist?                                                              |
|           Remove from LIST Box and profile.                                 |
|         Not Exist?                                                          |
|           Prompt the profile doesn't exist.                                 |
|                                                                             |
| CALLED ROUTINES                                                             |
|   -none-                                                                    |
|                                                                             |
| PARAMETERS                                                                  |
|   HWND hDlg - The dialog window handle.                                     |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   TRUE if all went successful.                                              |
\*---------------------------------------------------------------------------*/
BOOL RemoveProfiles(hDlg)
     HWND hDlg;
{
     HANDLE   hProfile,hBuffer;
     LPSTR    lpProfile,lpBuffer;
     int      nCount,idx,nProfile;
     WORD     wRet;
     OFSTRUCT of;

     /*-----------------------------------------*\
     | Allocate buffers for use.                 |
     \*-----------------------------------------*/
     nProfile = (int)SendDlgItemMessage(hDlg,IDD_INTRFACE_PROF,EM_LINELENGTH,0,0l);
     if(!(hProfile = LocalAlloc(LHND,(WORD)nProfile+1)))
          return(FALSE);
     if(!(hBuffer = LocalAlloc(LHND,(WORD)128)))
     {
          LocalFree(hProfile);
          return(FALSE);
     }

     /*-----------------------------------------*\
     | Lock buffers for use.                     |
     \*-----------------------------------------*/
     if(!(lpProfile = (LPSTR)LocalLock(hProfile)))
     {
          LocalFree(hProfile);
          LocalFree(hBuffer);
          return(FALSE);
     }
     if(!(lpBuffer = (LPSTR)LocalLock(hBuffer)))
     {
          LocalUnlock(hProfile);
          LocalFree(hProfile);
          LocalFree(hBuffer);
          return(FALSE);
     }

     /*-----------------------------------------*\
     | Get the profile to remove.  Then look for |
     | it in the TEST Listbox.                   |
     \*-----------------------------------------*/
     GetDlgItemText(hDlg,IDD_INTRFACE_PROF,lpProfile,nProfile+1);
     nCount = (int)SendDlgItemMessage(hDlg,IDD_INTRFACE_TEST,LB_GETCOUNT,NULL,0l);
     for(idx=0; idx < nCount; idx++)
     {
          SendDlgItemMessage(hDlg,IDD_INTRFACE_TEST,LB_GETTEXT,idx,(LONG)lpBuffer);
          if(SearchProfileString(lpBuffer,lpProfile))
          {
               SendDlgItemMessage(hDlg,IDD_INTRFACE_TEST,LB_DELETESTRING,idx,0l);
               WritePrivateProfileString((LPSTR)"ProfilesTest",lpProfile,
                    (LPSTR)NULL,(LPSTR)"PrntTest.ini");
               SendDlgItemMessage(hDlg,IDD_INTRFACE_TEST,LB_SETCURSEL,-1,0l);
               SetFocus(GetDlgItem(hDlg,IDD_INTRFACE_LIST));
               LocalUnlock(hProfile);
               LocalFree(hProfile);
               LocalUnlock(hBuffer);
               LocalFree(hBuffer);
               return(TRUE);
          }
     }

     /*-----------------------------------------*\
     | Look for profile in the LIST listbox.     |
     | This is done to delete it from the tests  |
     | altogether.                               |
     \*-----------------------------------------*/
     nCount = (int)SendDlgItemMessage(hDlg,IDD_INTRFACE_LIST,LB_GETCOUNT,NULL,0l);
     for(idx=0; idx < nCount; idx++)
     {
          SendDlgItemMessage(hDlg,IDD_INTRFACE_LIST,LB_GETTEXT,idx,(LONG)lpBuffer);
          if(SearchProfileString(lpBuffer,lpProfile))
          {
               wRet = MessageBox(GetParent(hDlg),(LPSTR)"Do you wish to remove from List of profiles",NULL,MB_ICONQUESTION | MB_YESNO);
               if(wRet == IDYES)
               {
                    SendDlgItemMessage(hDlg,IDD_INTRFACE_LIST,LB_DELETESTRING,
                         idx,(LONG)lpBuffer);
                    WritePrivateProfileString((LPSTR)"ProfilesList",lpProfile,
                         (LPSTR)NULL,(LPSTR)"PrntTest.ini");
                    SendDlgItemMessage(hDlg,IDD_INTRFACE_LIST,LB_SETCURSEL,0,0l);
                    SendMessage(hDlg,WM_COMMAND,IDD_INTRFACE_LIST,
                         MAKELONG(GetDlgItem(hDlg,IDD_INTRFACE_LIST),LBN_SELCHANGE));
                    OpenFile(lpProfile,&of,OF_DELETE);
               }
               SetFocus(GetDlgItem(hDlg,IDD_INTRFACE_LIST));
               LocalUnlock(hProfile);
               LocalFree(hProfile);
               LocalUnlock(hBuffer);
               LocalFree(hBuffer);
               return(TRUE);
          }
     }

     /*-----------------------------------------*\
     | If the previous two checks didn't find the|
     | the profile, then profile doesn't exist.  |
     \*-----------------------------------------*/
     MessageBox(GetParent(hDlg),(LPSTR)"Profile Does not exist",NULL,MB_OK);
     LocalUnlock(hProfile);
     LocalFree(hProfile);
     LocalUnlock(hBuffer);
     LocalFree(hBuffer);

     return(TRUE);
}


/*---------------------------------------------------------------------------*\
| ADD PROFILE TO LISTBOXES                                                    |
|   This routine adds the selected profile from the edit box to the list or   |
|   test boxes, depending on the existence of the profile.                    |
|                                                                             |
| ALGORITHM                                                                   |
|   Get the profile to add.                                                   |
|     Search for profile in the TEST listbox.                                 |
|       Found?                                                                |
|         - prompt messagebox indicating it's been added.                     |
|       Not Found?                                                            |
|         Search for profile in the LIST listbox.                             |
|           Found?                                                            |
|             - Add it to TEST listbox.                                       |
|           Not Found?                                                        |
|             - Add it to LIST listbox.                                       |
|                                                                             |
| CALLED ROUTINES                                                             |
|   -none-                                                                    |
|                                                                             |
| PARAMETERS                                                                  |
|   HWND hDlg - The dialog window handle.                                     |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   TRUE if all went successful.                                              |
\*---------------------------------------------------------------------------*/
BOOL AddProfiles(hDlg)
     HWND hDlg;
{
     HANDLE hBuffer,hProfile,hName,hDriver,hPort;
     LPSTR  lpBuffer,lpProfile,lpName,lpDriver,lpPort,lpTmp;
     int    nProfile,nName,nDriver,nPort;
     int    nCount,idx;

     /*-----------------------------------------*\
     | Allocate buffer for temporary string use. |
     | Use local heap for storage.               |
     \*-----------------------------------------*/
     if(!(hBuffer = LocalAlloc(LHND,(WORD)128)))
          return(FALSE);

     /*-----------------------------------------*\
     | Allocate buffer for profile string.  Use  |
     | the local heap for storage.               |
     \*-----------------------------------------*/
     nProfile = (int)SendDlgItemMessage(hDlg,IDD_INTRFACE_PROF,EM_LINELENGTH,0,0l);
     if(!(hProfile = LocalAlloc(LHND,(WORD)nProfile+1)))
     {
          LocalFree(hBuffer);
          return(FALSE);
     }

     /*-----------------------------------------*\
     | Lock buffers for use.                     |
     \*-----------------------------------------*/
     if(!(lpBuffer = (LPSTR)LocalLock(hBuffer)))
     {
          LocalFree(hBuffer);
          return(FALSE);
     }
     if(!(lpProfile = (LPSTR)LocalLock(hProfile)))
     {
          LocalUnlock(hBuffer);
          LocalFree(hBuffer);
          LocalFree(hProfile);
          return(FALSE);
     }

     /*-----------------------------------------*\
     | Look for profile in the TEST listbox. If  |
     | it exists, then prompt user with Message. |
     | If the profile is not found, then drop    |
     | down for next test.                       |
     \*-----------------------------------------*/
     GetDlgItemText(hDlg,IDD_INTRFACE_PROF,lpProfile,nProfile+1);
     nCount = (int)SendDlgItemMessage(hDlg,IDD_INTRFACE_TEST,LB_GETCOUNT,NULL,0l);
     for(idx=0; idx < nCount; idx++)
     {
          SendDlgItemMessage(hDlg,IDD_INTRFACE_TEST,LB_GETTEXT,idx,(LONG)lpBuffer);
          if(SearchProfileString(lpBuffer,lpProfile))
          {
               MessageBox(GetParent(hDlg),(LPSTR)"Test already added",NULL,MB_OK);
               SetFocus(GetDlgItem(hDlg,IDD_INTRFACE_LIST));
               LocalUnlock(hProfile);
               LocalFree(hProfile);
               LocalUnlock(hBuffer);
               LocalFree(hBuffer);
               return(TRUE);
          }
     }

     /*-----------------------------------------*\
     | Look for profile in the LIST listbox.  If |
     | it exists, then add the profile and other |
     | strings to the TEST listbox.  If it does  |
     | not exist, then drop down for next test.  |
     \*-----------------------------------------*/
     nCount = (int)SendDlgItemMessage(hDlg,IDD_INTRFACE_LIST,LB_GETCOUNT,NULL,0l);
     for(idx=0; idx < nCount; idx++)
     {
          SendDlgItemMessage(hDlg,IDD_INTRFACE_LIST,LB_GETTEXT,idx,(LONG)lpBuffer);
          if(SearchProfileString(lpBuffer,lpProfile))
          {
               SendDlgItemMessage(hDlg,IDD_INTRFACE_TEST,LB_ADDSTRING,NULL,(LONG)lpBuffer);
               WritePrivateProfileString((LPSTR)"ProfilesTest",lpProfile,
                    lpBuffer+lstrlen(lpProfile)+1,(LPSTR)"PrntTest.ini");
               SetFocus(GetDlgItem(hDlg,IDD_INTRFACE_LIST));
               LocalUnlock(hProfile);
               LocalFree(hProfile);
               LocalUnlock(hBuffer);
               LocalFree(hBuffer);
               return(TRUE);
          }
     }

     /*-----------------------------------------*\
     | Free up Buffer.  It is no longer needed.  |
     | Unlock Profile buffer for next test.      |
     \*-----------------------------------------*/
     LocalUnlock(hBuffer);
     LocalFree(hBuffer);
     LocalUnlock(hProfile);

     /*-----------------------------------------*\
     | If the previous searches failed, then the |
     | string has not been added to the tests.   |
     | First we will add the strings to the test.|
     | Then we will drop down for the existence  |
     | of the profile.  First allocate the       |
     | buffers for the Name, Driver and Port.    |
     \*-----------------------------------------*/
     nName = (int)SendDlgItemMessage(hDlg,IDD_INTRFACE_NAME,EM_LINELENGTH,0,0l);
     if(!(hName = LocalAlloc(LHND,(WORD)nName+1)))
     {
          LocalFree(hProfile);
          return(FALSE);
     }
     nDriver = (int)SendDlgItemMessage(hDlg,IDD_INTRFACE_DRIV,EM_LINELENGTH,0,0l);
     if(!(hDriver = LocalAlloc(LHND,(WORD)nDriver+1)))
     {
          LocalFree(hProfile);
          LocalFree(hName);
          return(FALSE);
     }
     nPort = (int)SendDlgItemMessage(hDlg,IDD_INTRFACE_PORT,EM_LINELENGTH,0,0l);
     if(!(hPort = LocalAlloc(LHND,(WORD)nPort+1)))
     {
          LocalFree(hProfile);
          LocalFree(hName);
          LocalFree(hDriver);
          return(FALSE);
     }

     /*-----------------------------------------*\
     | Lock down the buffers for use during the  |
     | next test.                                |
     \*-----------------------------------------*/
     if(!(lpProfile = (LPSTR)LocalLock(hProfile)))
     {
          LocalFree(hProfile);
          LocalFree(hName);
          LocalFree(hDriver);
          LocalFree(hPort);
          return(FALSE);
     }
     if(!(lpName = (LPSTR)LocalLock(hName)))
     {
          LocalUnlock(hProfile);
          LocalFree(hProfile);
          LocalFree(hName);
          LocalFree(hDriver);
          LocalFree(hPort);
          return(FALSE);
     }
     if(!(lpDriver = (LPSTR)LocalLock(hDriver)))
     {
          LocalUnlock(hProfile);
          LocalFree(hProfile);
          LocalUnlock(hName);
          LocalFree(hName);
          LocalFree(hDriver);
          LocalFree(hPort);
          return(FALSE);
     }
     if(!(lpPort = (LPSTR)LocalLock(hPort)))
     {
          LocalUnlock(hProfile);
          LocalFree(hProfile);
          LocalUnlock(hName);
          LocalFree(hName);
          LocalUnlock(hDriver);
          LocalFree(hDriver);
          LocalFree(hPort);
          return(FALSE);
     }

     /*-----------------------------------------*\
     | Retrieve the strings from the edit ctrl   |
     | boxes.  The profile was already retrieved.|
     \*-----------------------------------------*/
     GetDlgItemText(hDlg,IDD_INTRFACE_NAME,lpName,nName+1);
     GetDlgItemText(hDlg,IDD_INTRFACE_DRIV,lpDriver,nDriver+1);
     GetDlgItemText(hDlg,IDD_INTRFACE_PORT,lpPort,nPort+1);

     /*-----------------------------------------*\
     | Build the device string from the strings  |
     | retrieved from the edit ctrl boxes.       |
     \*-----------------------------------------*/
     if(!(hBuffer = LocalAlloc(LHND,(WORD)128)))
     {
          LocalUnlock(hProfile);
          LocalFree(hProfile);
          LocalUnlock(hName);
          LocalFree(hName);
          LocalUnlock(hDriver);
          LocalFree(hDriver);
          LocalUnlock(hPort);
          LocalFree(hPort);
          return(FALSE);
     }
     if(!(lpBuffer = (LPSTR)LocalLock(hBuffer)))
     {
          LocalUnlock(hProfile);
          LocalFree(hProfile);
          LocalUnlock(hName);
          LocalFree(hName);
          LocalUnlock(hDriver);
          LocalFree(hDriver);
          LocalUnlock(hPort);
          LocalFree(hPort);
          LocalFree(hBuffer);
          return(FALSE);
     }
     lstrcpy(lpBuffer,lpProfile);
     lstrcat(lpBuffer,(LPSTR)":");
     lstrcat(lpBuffer,lpName);
     lstrcat(lpBuffer,(LPSTR)",");
     lpTmp = lpDriver;
     while((*lpTmp != '.') && *lpTmp)
          lpTmp++;
     *lpTmp = '\0';
     AnsiUpper(lpDriver);
     SetDlgItemText(hDlg,IDD_INTRFACE_DRIV,lpDriver);
     lstrcat(lpBuffer,lpDriver);
     lstrcat(lpBuffer,(LPSTR)",");
     lstrcat(lpBuffer,lpPort);

     /*-----------------------------------------*\
     | Add the string to the listbox, then add   |
     | to the prnttest.ini profile.              |
     \*-----------------------------------------*/
     SendDlgItemMessage(hDlg,IDD_INTRFACE_LIST,LB_ADDSTRING,NULL,(LONG)lpBuffer);
     WritePrivateProfileString((LPSTR)"ProfilesList",lpProfile,
          lpBuffer+lstrlen(lpProfile)+1,(LPSTR)"PrntTest.ini");
     WritePrivateProfileString((LPSTR)"Windows","Device",
          lpBuffer+lstrlen(lpProfile)+1,(LPSTR)lpProfile);

     /*-----------------------------------------*\
     | Free up the buffers used for building the |
     | device string.  THAT BE IT!!!             |
     \*-----------------------------------------*/
     LocalUnlock(hProfile);
     LocalFree(hProfile);
     LocalUnlock(hName);
     LocalFree(hName);
     LocalUnlock(hDriver);
     LocalFree(hDriver);
     LocalUnlock(hPort);
     LocalFree(hPort);
     LocalUnlock(hBuffer);
     LocalFree(hBuffer);

     return(TRUE);
}


/*---------------------------------------------------------------------------*\
| UPDATE SELECTION CHANGE                                                     |
|   This routine updates the edit control boxes to reflect the current string |
|   selection in the LIST ListBox.                                            |
|                                                                             |
| CALLED ROUTINES                                                             |
|   -none-                                                                    |
|                                                                             |
| PARAMETERS                                                                  |
|   HWND hDlg - The dialog window handle.                                     |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   TRUE if all went successful.                                              |
\*---------------------------------------------------------------------------*/
BOOL UpdateSelectionChange(hDlg)
     HWND hDlg;
{
     int    nSize;
     HANDLE hBuffer,hProfile,hName,hDriver,hPort;
     LPSTR  lpBuffer,lpProfile,lpName,lpDriver,lpPort;

     nSize = (int)SendDlgItemMessage(hDlg,IDD_INTRFACE_LIST,LB_GETTEXTLEN,
                       (WORD)SendDlgItemMessage(hDlg,IDD_INTRFACE_LIST,
                       LB_GETCURSEL,NULL,0l),0l);

     if(!(hBuffer = LocalAlloc(LHND,(WORD)nSize+1)))
          return(FALSE);
     if(!(hProfile = LocalAlloc(LHND,(WORD)128)))
     {
          LocalFree(hBuffer);
          return(FALSE);
     }
     if(!(hName = LocalAlloc(LHND,(WORD)128)))
     {
          LocalFree(hBuffer);
          LocalFree(hProfile);
          return(FALSE);
     }
     if(!(hDriver = LocalAlloc(LHND,(WORD)128)))
     {
          LocalFree(hBuffer);
          LocalFree(hProfile);
          LocalFree(hName);
          return(FALSE);
     }
     if(!(hPort = LocalAlloc(LHND,(WORD)128)))
     {
          LocalFree(hBuffer);
          LocalFree(hProfile);
          LocalFree(hName);
          LocalFree(hDriver);
          return(FALSE);
     }
     if(!(lpBuffer = (LPSTR)LocalLock(hBuffer)))
     {
          LocalFree(hBuffer);
          LocalFree(hProfile);
          LocalFree(hName);
          LocalFree(hDriver);
          LocalFree(hPort);
          return(FALSE);
     }
     if(!(lpProfile = (LPSTR)LocalLock(hProfile)))
     {
          LocalUnlock(hBuffer);
          LocalFree(hBuffer);
          LocalFree(hProfile);
          LocalFree(hName);
          LocalFree(hDriver);
          LocalFree(hPort);
          return(FALSE);
     }

     if(!(lpName = (LPSTR)LocalLock(hName)))
     {
          LocalUnlock(hBuffer);
          LocalFree(hBuffer);
          LocalUnlock(hProfile);
          LocalFree(hProfile);
          LocalFree(hName);
          LocalFree(hDriver);
          LocalFree(hPort);
          return(FALSE);
     }
     if(!(lpDriver = (LPSTR)LocalLock(hDriver)))
     {
          LocalUnlock(hBuffer);
          LocalFree(hBuffer);
          LocalUnlock(hProfile);
          LocalFree(hProfile);
          LocalUnlock(hName);
          LocalFree(hName);
          LocalFree(hDriver);
          LocalFree(hPort);
          return(FALSE);
     }
     if(!(lpPort = (LPSTR)LocalLock(hPort)))
     {
          LocalUnlock(hBuffer);
          LocalFree(hBuffer);
          LocalUnlock(hProfile);
          LocalFree(hProfile);
          LocalUnlock(hName);
          LocalFree(hName);
          LocalUnlock(hDriver);
          LocalFree(hDriver);
          LocalFree(hPort);
          return(FALSE);
     }

     SendDlgItemMessage(hDlg,IDD_INTRFACE_LIST,LB_GETTEXT,
          (WORD)SendDlgItemMessage(hDlg,IDD_INTRFACE_LIST,LB_GETCURSEL,NULL,0l),
          (LONG)lpBuffer);

     ParseDeviceString(lpBuffer,lpProfile,lpName,lpDriver,lpPort);

     SetDlgItemText(hDlg,IDD_INTRFACE_PROF,lpProfile);
     SetDlgItemText(hDlg,IDD_INTRFACE_NAME,lpName);
     SetDlgItemText(hDlg,IDD_INTRFACE_DRIV,lpDriver);
     SetDlgItemText(hDlg,IDD_INTRFACE_PORT,lpPort);
     SetFocus(GetDlgItem(hDlg,IDD_INTRFACE_LIST));
     SendDlgItemMessage(hDlg,IDD_INTRFACE_PROF,EM_SETMODIFY,FALSE,0l);

     LocalUnlock(hBuffer);
     LocalUnlock(hProfile);
     LocalUnlock(hName);
     LocalUnlock(hDriver);
     LocalUnlock(hPort);
     LocalFree(hBuffer);
     LocalFree(hProfile);
     LocalFree(hName);
     LocalFree(hDriver);
     LocalFree(hPort);

     return(TRUE);
}


/*---------------------------------------------------------------------------*\
| SEARCH PROFILE STRING                                                       |
|   This routine searches a buffer for the existence of the profile string.   |
|                                                                             |
| CALLED ROUTINES                                                             |
|   -none-                                                                    |
|                                                                             |
| PARAMETERS                                                                  |
|   LPSTR lpBuffer  - Buffer with text string.                                |
|   LPSRT lpProfile - Profile to find.                                        |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   TRUE if all went successful.                                              |
\*---------------------------------------------------------------------------*/
BOOL SearchProfileString(lpBuffer,lpProfile)
     LPSTR lpBuffer;
     LPSTR lpProfile;
{
     HANDLE hCompare;
     LPSTR  lpCompare;

     if(!(hCompare = LocalAlloc(LHND,(WORD)256)))
          return(FALSE);
     if(!(lpCompare = (LPSTR)LocalLock(hCompare)))
     {
          LocalFree(hCompare);
          return(FALSE);
     }

     /*-----------------------------------------*\
     | Get the string from the test List box,    |
     | and check it against the profile for cmp. |
     \*-----------------------------------------*/
     ParseDeviceString(lpBuffer,lpCompare,NULL,NULL,NULL);

     if(lstrcmp(lpProfile,lpCompare) == 0)
     {
          LocalUnlock(hCompare);
          LocalFree(hCompare);
          return(TRUE);
     }

     LocalUnlock(hCompare);
     LocalFree(hCompare);

     return(FALSE);
}


/*---------------------------------------------------------------------------*\
| PARSE DEVICE STRING                                                         |
|   This routine parses the device string into its Profile, Name, Driver and  |
|   Port strings.                                                             |
|                                                                             |
| CALLED ROUTINES                                                             |
|   -none-                                                                    |
|                                                                             |
| PARAMETERS                                                                  |
|   LPSTR lpString  - Buffer with text string.                                |
|   LPSRT lpProfile - Profile.                                                |
|   LPSRT lpName    - Driver text name.                                       |
|   LPSRT lpDriver  - Driver filename.                                        |
|   LPSRT lpPort    - Port.                                                   |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   TRUE if all went successful.                                              |
\*---------------------------------------------------------------------------*/
BOOL ParseDeviceString(szString,szProfile,szName,szDriver,szPort)
     LPSTR szString;
     LPSTR szProfile;
     LPSTR szName;
     LPSTR szDriver;
     LPSTR szPort;
{
     LPSTR lpSrc,lpDst;

     lpSrc = szString;

     /*-----------------------------------------*\
     | Copy the profile name.                    |
     \*-----------------------------------------*/
     lpDst = szProfile;
     if(lpDst)
     {
          while((*lpSrc != ':') && *lpSrc)
               *lpDst++ = *lpSrc++;
          *lpDst = '\0';
     }

     /*-----------------------------------------*\
     | Copy the driver name.                     |
     \*-----------------------------------------*/
     lpDst = szName;
     lpSrc++;
     if(lpDst)
     {
          while((*lpSrc != ',') && *lpSrc)
               *lpDst++ = *lpSrc++;
          *lpDst = '\0';
     }

     /*-----------------------------------------*\
     | Copy the driver module name.              |
     \*-----------------------------------------*/
     lpDst = szDriver;
     lpSrc++;
     if(lpDst)
     {
          while((*lpSrc != ',') && *lpSrc)
               *lpDst++ = *lpSrc++;
          *lpDst = '\0';
     }

     /*-----------------------------------------*\
     | Copy the port name.                       |
     \*-----------------------------------------*/
     lpDst = szPort;
     lpSrc++;
     if(lpDst)
     {
          while((*lpSrc != '\0') && *lpSrc)
               *lpDst++ = *lpSrc++;
          *lpDst = '\0';
     }

     return(TRUE);
}


/*---------------------------------------------------------------------------*\
| SETUP PRINTER                                                               |
|   This routine calls the printer drivers EXTDEVICEMODE routine to display   |
|   the Setup DialogBox and set the printer profiles devmode section.         |
|                                                                             |
| CALLED ROUTINES                                                             |
|   -none-                                                                    |
|                                                                             |
| PARAMETERS                                                                  |
|   HWND hDlg - Handle to the Modeless Dialogbox interface.                   |
|                                                                             |
| GLOBAL VARIABLES                                                            |
|   -none-                                                                    |
|                                                                             |
| RETURNS                                                                     |
|   TRUE if all went successful.                                              |
\*---------------------------------------------------------------------------*/
BOOL SetupPrinter(hDlg)
     HWND hDlg;
{
     FARPROC   lpProc;
     char      szModule[25],szName[25],szDriver[25],szPort[25],szProfile[25];
     HANDLE    hLibrary;
     LPDEVMODE lpDevMode;
     HANDLE    hDevMode;
     short     nSize;

     /*-----------------------------------------*\
     | Retrieve the text from the selected edit  |
     | boxes.                                    |
     \*-----------------------------------------*/
     GetDlgItemText(hDlg,IDD_INTRFACE_DRIV,(LPSTR)szDriver,sizeof(szDriver));
     GetDlgItemText(hDlg,IDD_INTRFACE_PORT,(LPSTR)szPort,sizeof(szPort));
     GetDlgItemText(hDlg,IDD_INTRFACE_NAME,(LPSTR)szName,sizeof(szName));
     GetDlgItemText(hDlg,IDD_INTRFACE_PROF,(LPSTR)szProfile,sizeof(szProfile));
     lstrcpy((LPSTR)szModule,szDriver);
     lstrcat((LPSTR)szModule,(LPSTR)".DRV");

     /*-----------------------------------------*\
     | Load the printer library, and retreive    |
     | the xxxDeviceMode from the library.       |
     \*-----------------------------------------*/
     if((hLibrary = LoadLibrary(szModule)) < 32)
          return(FALSE);
     if(!(lpProc = GetProcAddress(hLibrary,(LPSTR)"ExtDeviceMode")))
     {
          if(!(lpProc = GetProcAddress(hLibrary,(LPSTR)"DeviceMode")))
          {
               FreeLibrary(hLibrary);
               return(FALSE);
          }
          (*lpProc)((HWND)hDlg,(HANDLE)hLibrary,(LPSTR)szName,(LPSTR)szPort);
          FreeLibrary(hLibrary);
          return(TRUE);
     }

     /*-----------------------------------------*\
     | Call the ExtDeviceMode from the driver.   |
     \*-----------------------------------------*/
     nSize = (short)(*lpProc)(hDlg,hLibrary,(LPSTR)NULL,(LPSTR)szName,(LPSTR)szPort,
              (LPSTR)NULL,(LPSTR)szProfile,0);
     if(!nSize)
     {
          FreeLibrary(hLibrary);
          return(FALSE);
     }

     /*-----------------------------------------*\
     | Allocate the DevMode structure and call   |
     | the ExtDeviceMode for the settings.       |
     \*-----------------------------------------*/
     if(hDevMode = LocalAlloc(LHND,(WORD)nSize))
     {
          if(lpDevMode = (LPDEVMODE)LocalLock(hDevMode))
          {
               (*lpProc)((HWND)hDlg,(HANDLE)hLibrary,(LPSTR)lpDevMode,(LPSTR)szName,
                        (LPSTR)szPort,(LPSTR)lpDevMode,(LPSTR)szProfile,
                        DM_IN_PROMPT | DM_IN_BUFFER | DM_OUT_DEFAULT |
                        DM_OUT_BUFFER);
               LocalUnlock(hDevMode);
          }
          LocalFree(hDevMode);
          FreeLibrary(hLibrary);
          return(TRUE);
     }

     FreeLibrary(hLibrary);
     return(FALSE);
}
