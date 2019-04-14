/*************************************************************************\
 * profile.c
 * (C) 94   Ralf Seidel
 *          WÅlfrather Str. 45
 *          42105 Wuppertal
 *          email: seidel3@wrcs1.uni-wuppertal.de
 *
 * Set tabs to 3 to get a readable source code.
\*************************************************************************/
#define INCL_WINSHELLDATA
#define INCL_WINWINDOWMGR
#include <os2.h>
#include "profile.h"

typedef struct {
	LONG x;
	LONG y;
	ULONG cx;
	ULONG cy;
	ULONG flags;
	USHORT xRestore;
	USHORT yRestore;
	USHORT cxRestore;
	USHORT cyRestore;
	USHORT xMin;
	USHORT yMin;
} PRF_POSDATA, *PPRF_POSDATA;

/*************************************************************************\
 * FUNCTION StoreWindowPos
 * This function is meant as a replacement for the OS/2 API function
 * WinStoreWindowPos. It stores the current size and postion of the window
 * hwnd (window handle) to the profile hini (profile handle).
 * I use my own function because the original always stores it's
 * information in os2.ini which slows down system performace. 
 * Note: The function WinStoreWindowPos does a bit more. It also saves
 * the presentation parameters (i.e. color, fonts etc) of the window.
 * Because I don't need this it's not included here. (It would be a lot of
 * stupid work.)
 * Like the original this function returns TRUE if it was successful and
 * FALSE otherwise.
\*************************************************************************/
BOOL StoreWindowPos(
	const HINI hini, const PCSZ pszApp,
	const PCSZ pszKey, const HWND hwnd )
{
	SWP swp;				/* structure to save window information returned
							 * by WinQueryWindowPos 									*/
	PRF_POSDATA prfPosData;

	/* Get Current window position and size */
	if ( !WinQueryWindowPos( hwnd, &swp ) ) return FALSE;
	prfPosData.x = swp.x;
	prfPosData.y = swp.y;
	prfPosData.cx = swp.cx;
	prfPosData.cy = swp.cy;
	prfPosData.flags = swp.fl;
	/* Query the restore and minimize positions */
	prfPosData.xRestore = WinQueryWindowUShort( hwnd, QWS_XRESTORE );
	prfPosData.yRestore = WinQueryWindowUShort( hwnd, QWS_YRESTORE );
	prfPosData.cxRestore = WinQueryWindowUShort( hwnd, QWS_CXRESTORE );
	prfPosData.cyRestore = WinQueryWindowUShort( hwnd, QWS_CYRESTORE );
	prfPosData.xMin = WinQueryWindowUShort( hwnd, QWS_XMINIMIZE );
	prfPosData.yMin = WinQueryWindowUShort( hwnd, QWS_YMINIMIZE );

	/* write the values to the profile hini */
	if ( !PrfWriteProfileData( hini, pszApp, pszKey,
										&prfPosData, sizeof( PRF_POSDATA ) )
		) return FALSE;
	return TRUE;
}

/*************************************************************************\
 * FUNCTION RestoreWindowPos
 * Analogue to StoreWindowPos but the contratry action.
 * Note that in difference to the original WinRestoreWindowPos this
 * function neither makes the window visible nor activates it.
\*************************************************************************/
BOOL RestoreWindowPos(
	const HINI hini, const PCSZ pszApp,
	const PCSZ pszKey, const HWND hwnd )
{
	PRF_POSDATA prfPosData;
	ULONG ulNumRead = sizeof( PRF_POSDATA );
	ULONG flags = SWP_SIZE | SWP_MOVE;
	BOOL bResult;

	bResult =
	PrfQueryProfileData(
		hini, pszApp, pszKey, &prfPosData, &ulNumRead );
	if ( !bResult || (ulNumRead != sizeof( PRF_POSDATA )) ) {	
		return FALSE;
	}
	if ( prfPosData.flags & SWP_MAXIMIZE ) flags|= SWP_MAXIMIZE;
	if ( prfPosData.flags & SWP_MINIMIZE ) flags|= SWP_MINIMIZE;

	/* Set new position */
	bResult = 
	WinSetWindowPos( hwnd, NULLHANDLE,
		prfPosData.x, prfPosData.y,
		prfPosData.cy, prfPosData.cy, flags );
	if ( !bResult ) return FALSE;
	WinSetWindowUShort( hwnd, QWS_XRESTORE, prfPosData.xRestore );
	WinSetWindowUShort( hwnd, QWS_YRESTORE, prfPosData.yRestore );
	WinSetWindowUShort( hwnd, QWS_CXRESTORE, prfPosData.cxRestore );
	WinSetWindowUShort( hwnd, QWS_CYRESTORE, prfPosData.cyRestore );
	WinSetWindowUShort( hwnd, QWS_XMINIMIZE, prfPosData.xMin );
	WinSetWindowUShort( hwnd, QWS_YMINIMIZE, prfPosData.yMin );
	return TRUE;
}

