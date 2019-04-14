/*************************************************************************\
 * threads.c
 * (C) 94   Ralf Seidel
 *          WÅlfrather Str. 45
 *          42105 Wuppertal
 *          email: seidel3@wrcs1.uni-wuppertal.de
 *
 * Set tabs to 3 to get a readable source.
\*************************************************************************/
#define INCL_WINWINDOWMGR
#define INCL_WINMESSAGEMGR
#define INCL_DOSPROCESS
#include <os2.h>
#include "messages.h"
#include "threads.h"

/*************************************************************************\
 * PROCEDURE InitThreadInfo
 * Initialize the THREADINFO sturcture.
\*************************************************************************/
VOID InitThreadInfo( PTHREADINFO pti, ULONG ulSize, ULONG ulType, HWND hwnd )
{
	pti->ulSize = ulSize;
	pti->ulType = ulType;
	pti->hwndOwner = hwnd;
	pti->bKill = FALSE;
}

/*************************************************************************\
 * PROCEDURE InitThread
 * Initialize the THREADINFO sturcture inside the thread function.
\*************************************************************************/
VOID InitThread( PTHREADINFO pti )
{
	pti->bDead = FALSE;
	pti->hab = WinInitialize( 0 );
	pti->hmq = WinCreateMsgQueue( pti->hab, 0L );
	pti->ulResult = 0;
	/* Because the thread usually doesn't examine the message queue
	 * we want to prevent that OS/2 places a WM_QUIT message there. */
	WinCancelShutdown( pti->hmq, TRUE );
}

/*************************************************************************\
 * PROCEDURE DoneThread
 * Post a message that the thread has ended and do some cleanup.
\*************************************************************************/
VOID DoneThread( PTHREADINFO pti, ULONG ulResult )
{
	pti->ulResult = ulResult;
	WinPostMsg( pti->hwndOwner, MYM_ENDTHREAD,
		MPFROMLONG( pti->ulType ), MPFROMP( pti ) );
	WinDestroyMsgQueue( pti->hmq );
	WinTerminate( pti->hab );
	DosEnterCritSec();
	pti->bDead = TRUE;
}
