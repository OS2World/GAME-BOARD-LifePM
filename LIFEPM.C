/*************************************************************************\
 * lifepm.c
 * (C) 94   Ralf Seidel
 *          WÅlfrather Str. 45
 *          42105 Wuppertal
 *          email: seidel3@wrcs1.uni-wuppertal.de
 *
 * Set tabs to 3 to get a readable source.
\*************************************************************************/
// include files, macros and definitions
#define INCL_DOSPROCESS
#define INCL_WINMENUS
#define INCL_WINSYS
#define INCL_WINHELP
#define INCL_WININPUT
#define INCL_WINFRAMEMGR
#define INCL_WINWINDOWMGR
#define INCL_WINMESSAGEMGR
#define INCL_WINDIALOGS
#define INCL_WINSHELLDATA
#define INCL_WINSTDFILE
#define INCL_WINPOINTERS
#define INCL_WINTRACKRECT
#define INCL_WINBUTTONS
#define INCL_WINRECTANGLES
#define INCL_WINCLIPBOARD
#define INCL_GPIPRIMITIVES
#define INCL_GPIBITMAPS
// To get the full advantage of precompiled header files, most include
// directives are placed in one file.
#include "lifepm.inc"
// My header files.
#include "bitmaps.h"
#include "errmsgs.h"
#include "threads.h"
#include "profile.h"
#include "life.h"

// Some definitions for resources und events.
#include "resdefs.h"
#include "messages.h"

#define VERSION( compiler ) (PSZ)("1.0 " # compiler)
#if defined (__EMX__)
static const PSZ pszVersion = VERSION( emx );
#elif defined (__WATCOMC__)
static const PSZ pszVersion = VERSION( wcc );
#elif defined (__IBMC__)
static const PSZ pszVersion = VERSION( icc );
#elif defined (__BORLANDC__)
static const PSZ pszVersion = VERSION( bcc );
#elif defined (__GNUC__)
static const PSZ pszVersion = VERSION( gcc );
#endif

#define MIN_CELLS 			 10

#define MAX_RES_STRLENGTH	256	// Maximal length for resource strings
#define MAX_RES_MSGLENGTH	MAX_RES_STRLENGTH	


// Threadtyps
enum {
	TT_NONE,
	TT_STEP,
	TT_RUN
};

typedef VOID (* _PFNTHREAD)( PVOID );

// The data saved in following structure is initialized by main calling
// InitProgInfo. It must not be changed by any other routine i.e. it
// should be treated like read only data.

typedef struct {
	PSZ pszVersion;		// Version and compiler identifier
	PSZ pszProgPath;		// Pfad wo das Programm steht 
	PSZ pszProgName;		// Programname (*.exe)
	PSZ pszAppTitle;		// Name of this application
	PSZ pszHelpName;		// Name of the related help file
	PSZ pszIniName;		// Name of the ini file
	HAB hab;					// Anchor block of the main thread
	HWND hwndMain;			// window handle of the main window
	HWND hwndHelp;			// window handle of the help facility
} PROGINFO, *PPROGINFO;

// Data saved in this sturcture is stored in the program profile.

typedef struct {
	TTopology  topology;
	TAlgorithm algorithm;
	UINT Rows;					// Number of planet rows...
	UINT Cols;					// ... and columns
	UINT CellSize;				// Size of one cell
	BOOL bGrid;					// Specifies whether to draw a grid or not
} PROFILEDATA, *PPROFILEDATA;

/*************************************************************************\
 * There exists only one variable of type PROGINFO but two variables of
 * type PROFILEDATA. `ProfInfo' is initialize during startup and holds some
 * information about filenames and the main PM handles. The data stored in
 * `ProgOptions' can be changed by user interaction. The options valid at
 * program startup time are saved in `StartupOptions'. This is done to
 * avoid unnecessary ini file actions. We will only save program options if
 * `ProgOptions' is different to `StartupOptions'.
\*************************************************************************/

static PROGINFO ProgInfo = { 0 };

static PROFILEDATA ProgOptions, StartupOptions;

static const struct {
	PSZ Version;
	PSZ Settings;
	PSZ WndPos;
} PrfKeyNames = {
	"Version",
	"Settings",
	"MainWndPos"
};

// We have to subclass the main window frame procedure to take control over
// resizing. The original window procedure is saved in this variable.
static PFNWP DefMainFrameProc;

// Prototypen der Funktionen die in diesem Modul definert werden
MRESULT EXPENTRY MainClientProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );

MRESULT EXPENTRY MainFrameProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );

LONG LoadMessage( ULONG ulMsgId, PSZ pszMsg );

static void Unselect( HWND hwnd );

#define Xmalloc( p, size, action ) \
	if ( !((p) = (PVOID)malloc( (size_t)(size) ) ) ) { \
		CHAR msg[MAX_RES_MSGLENGTH]; \
		LoadMessage( IDMSG_MALLOC, msg ); \
		ErrMsgRunLib( msg ); \
		WinSendMsg( ProgInfo.hwndMain, WM_QUIT, (MPARAM)0, (MPARAM)0 ); \
		action; \
	}

/*************************************************************************\
 * PROCEDURE EnableMenuItem
 * Enable/Disables menu items
\*************************************************************************/
VOID EnableMenuItem( const HWND hwndClient, const USHORT usId, const BOOL fEnable )
{
	const HWND hwndFrame = WinQueryWindow( hwndClient, QW_PARENT );
	const HWND hwndMenu = WinWindowFromID( hwndFrame, FID_MENU );
	(VOID)WinEnableMenuItem( hwndMenu, usId, fEnable );
}

/*************************************************************************\
 * PROCEDURE SetMenuItemCheck
 * Set or unsets a check mark in menu items.
\*************************************************************************/
VOID SetMenuItemCheck( const HWND hwndClient, const USHORT usId, const BOOL fSet )
{
	const HWND hwndFrame = WinQueryWindow( hwndClient, QW_PARENT );
	const HWND hwndMenu = WinWindowFromID( hwndFrame, FID_MENU );
	(VOID)WinCheckMenuItem( hwndMenu, usId, fSet );
}

/*************************************************************************\
 * PROCEDURE LoadMessage
 * Loads a message string from the resource.
\*************************************************************************/
LONG LoadMessage( ULONG ulMsgId, PSZ pszMsg )
{
	return WinLoadMessage( ProgInfo.hab, NULLHANDLE, ulMsgId, MAX_RES_MSGLENGTH, pszMsg );
}

/*************************************************************************\
 * PROCEDURE LoadString
 * Loads a string from the resource.
\*************************************************************************/
LONG LoadString( ULONG ulStrId, PSZ pszStr )
{
	return WinLoadString( ProgInfo.hab, NULLHANDLE, ulStrId, MAX_RES_STRLENGTH, pszStr );
}

/*************************************************************************\
 * PROCEDURE UpdateCell
 * See message processing of MYM_UPDATECELL. (This function is only a
 * abbrevation.)
\*************************************************************************/
void UpdateCell( const HWND hwnd, const row, const col )
{
	WinSendMsg( hwnd, MYM_UPDATECELL, (MPARAM)row, (MPARAM)col );
}

/*************************************************************************\
 * PROCEDURE DrawCell
 * Zeichnet eine Zelle an gegebener Koordinate in the presentation space hps.
 * Global variables used:
 * ProgOptions.bGrid:
 *		Boolean variable that specifies whether to draw a grid or not.
 * ProgOptions.CellSize:
 * 	Specifies the size of one cell to draw.
\*************************************************************************/
void DrawCell( const HPS hps, const row, const col, const LONG lColor )
{
	POINTL ptl;		// Point in window coordinates where to draw the cell.
	LONG lBoxSize;	// Size of the box.

	GpiSetColor( hps, lColor );
	if ( ProgOptions.CellSize == 1 ) {
		ptl.x = (col - 1);
		ptl.y = (row - 1);
		GpiSetPel( hps, &ptl );
	} else {
		ptl.x = (col - 1) * ProgOptions.CellSize;
		ptl.y = (row - 1) * ProgOptions.CellSize;
		GpiMove( hps, &ptl );
		lBoxSize = ProgOptions.CellSize - (ProgOptions.bGrid ? 2 : 1);
		ptl.x+= lBoxSize;
		ptl.y+= lBoxSize;
		GpiBox( hps, DRO_FILL, &ptl, 0L, 0L );
	}
	return;
}

/*************************************************************************\
 * PROCEDURE DrawPlanet
 * This function is called in response to a WM_PAINT message.
 * Parameters:
 * hwnd:
 *		Window handle of the window to be updated.
 * prectSelect:
 *		Pointer to a selected area. Has to be NULL if nothing is selected.
 * Global variables used:
 * ProgOptions.bGrid:
 *		Boolean variable that specifies whether to draw a grid or not.
 * ProgOptions.CellSize:
 * 	Specifies the size of one cell to draw.
\*************************************************************************/
void DrawPlanet( const HWND hwnd, const PRECTL prectSelect )
{
	RECTL rectUpdate;	// The rectangle to be updated.
	RECTL rectPlanet;	// The rectangle to be updated in planet coordinates.
	const HPS hps = WinBeginPaint( hwnd, 0, &rectUpdate );
	POINTL ptlStart;
	POINTL ptlEnd;
	int row, col;

	// Clear the background
	WinFillRect( hps, &rectUpdate, SYSCLR_WINDOW );
	// Set background of selected area.
	if ( prectSelect ) {
		RECTL rect;
		WinSetRect( ProgInfo.hab, &rect, 
			(prectSelect->xLeft - 1) * ProgOptions.CellSize,
			(prectSelect->yBottom - 1) * ProgOptions.CellSize,
			(prectSelect->xRight - 1) * ProgOptions.CellSize,
			(prectSelect->yTop - 1) * ProgOptions.CellSize );
		WinFillRect( hps, &rect, CLR_PALEGRAY );
	}
	
	if ( ProgOptions.bGrid && (ProgOptions.CellSize != 1 )) {
		// Draw the grid: the horizontal lines first.
		GpiSetColor( hps, CLR_BLACK );
		ptlStart.x = rectUpdate.xLeft;
		ptlEnd.x = rectUpdate.xRight;
		// This calculation of the first vertical position to begin updating
		// is a little bit difficult to explain. I would guess that using paper
		// and pencil is the best way to find out how it works.
		ptlStart.y = ptlEnd.y =
			rectUpdate.yBottom + ProgOptions.CellSize -
			rectUpdate.yBottom % ProgOptions.CellSize - 1;
		while ( ptlStart.y < rectUpdate.yTop ) {
			GpiMove( hps, &ptlStart );
			GpiLine( hps, &ptlEnd );
			ptlStart.y = ptlEnd.y+= ProgOptions.CellSize;
		}
		// And now draw the vertical lines
		ptlStart.x = ptlEnd.x =
			rectUpdate.xLeft + ProgOptions.CellSize -
			rectUpdate.xLeft % ProgOptions.CellSize - 1;
		ptlStart.y = rectUpdate.yBottom;
		ptlEnd.y = rectUpdate.yTop;
		while( ptlStart.x < rectUpdate.xRight ) {
			GpiMove( hps, &ptlStart );
			GpiLine( hps, &ptlEnd );
			ptlStart.x = ptlEnd.x+= ProgOptions.CellSize;
		}
	} // end if ( bGrid )
	
	// draw the cells: first find out which cells have to be updated
	rectPlanet.xLeft = rectUpdate.xLeft / ProgOptions.CellSize + 1;
	rectPlanet.xRight = (rectUpdate.xRight - 1) / ProgOptions.CellSize + 1;
	rectPlanet.yBottom = rectUpdate.yBottom / ProgOptions.CellSize + 1;
	rectPlanet.yTop = (rectUpdate.yTop - 1) / ProgOptions.CellSize + 1;
	if ( !prectSelect ) {
		// if no selection rectangle is specified get rid of some
		// if statements.
		for ( row = rectPlanet.yBottom; row <= rectPlanet.yTop; row++ ) {
			for ( col = rectPlanet.xLeft; col <= rectPlanet.xRight; col++ ) {
				if ( planet[row][col] ) DrawCell( hps, row, col, CLR_BLACK );
			} // end for col
		} // end for row
	} else {
		POINTL pt;
		for ( pt.y = rectPlanet.yBottom; pt.y <= rectPlanet.yTop; pt.y++ ) {
			for ( pt.x = rectPlanet.xLeft; pt.x <= rectPlanet.xRight; pt.x++ ) {
				if ( planet[pt.y][pt.x] ) {
					if ( WinPtInRect( ProgInfo.hab, prectSelect, &pt ) ) 
						DrawCell( hps, pt.y, pt.x, CLR_DARKBLUE );
					else
						DrawCell( hps, pt.y, pt.x, CLR_BLACK );
				}
			} // end for col
		} // end for row
	} // end if prectSelect
	WinReleasePS( hps );
}

/*************************************************************************\
 * PROCEDURE CenterWindow
 * Zentriert das Child Fenster innerhalb im Parent Fenster.
 * Gibt TRUE zurÅck falls alles erfolgreich ablief und FALSE sonst.
\*************************************************************************/
BOOL CenterWindow( const HWND hwndParent, const HWND hwndChild )
{
	RECTL rectParent;
	SIZEL sizeParent;
	RECTL rectChild;
	SIZEL sizeChild;

	if ( !WinQueryWindowRect( hwndParent, &rectParent ) )
		return FALSE;
	sizeParent.cx = rectParent.xRight - rectParent.xLeft;
	sizeParent.cy = rectParent.yTop - rectParent.yBottom;
	
	if ( !WinQueryWindowRect( hwndChild, &rectChild ) )
		return FALSE;
	sizeChild.cx = rectChild.xRight - rectChild.xLeft;
	sizeChild.cy = rectChild.yTop - rectChild.yBottom;

	WinSetWindowPos(
		hwndChild, NULLHANDLE,
		(sizeParent.cx - sizeChild.cx) / 2,
		(sizeParent.cy - sizeChild.cy) / 2,
		0, 0, SWP_MOVE	);
	return TRUE;
}

/*************************************************************************\
 * FUNCTION InitHelp
 * Initialize help facilities and return a handle to the help window.
\*************************************************************************/
HWND InitHelp(
	const HAB hab,
	const USHORT usHelpTable,
	const ULONG ulHelpTitleID,
	const PSZ pszHelpName )
{
	HWND hwndHelp; 							 // Return value of this function
	HELPINIT HelpInit;
	CHAR szHelpTitle[MAX_RES_STRLENGTH];

	if ( !LoadString( ulHelpTitleID, szHelpTitle ) ) {
		return NULLHANDLE;
	}
	HelpInit.cb = sizeof(HELPINIT);
	HelpInit.ulReturnCode = 0L;
	HelpInit.pszTutorialName = NULL;
	HelpInit.phtHelpTable = (PHELPTABLE)MAKELONG(usHelpTable, 0xFFFF);
	HelpInit.hmodHelpTableModule = NULLHANDLE;
	HelpInit.hmodAccelActionBarModule = NULLHANDLE;
	HelpInit.idAccelTable = 0;
	HelpInit.idActionBar = 0;
	HelpInit.pszHelpWindowTitle = szHelpTitle;
	HelpInit.fShowPanelId = CMIC_HIDE_PANEL_ID;
	HelpInit.pszHelpLibraryName = pszHelpName;

	hwndHelp = WinCreateHelpInstance( hab, &HelpInit );
	if ( !hwndHelp || HelpInit.ulReturnCode  ) {
		// We cant initialize the help system. This is no fatal error
		// but help will be disabled --> Tell the user about it.
		CHAR pszResMsg[MAX_RES_MSGLENGTH];
		PSZ pszMsg;

		LoadMessage( IDMSG_WINCREATEHELPINSTANCE, pszResMsg );
		pszMsg = (PSZ)alloca( strlen( pszResMsg ) + strlen( pszHelpName ) );

		sprintf( pszMsg, pszResMsg, pszHelpName );
		ErrMsgPM( hab, pszMsg );
	} else if ( !WinAssociateHelpInstance( hwndHelp, ProgInfo.hwndMain ) ) {
		CHAR pszMsg[MAX_RES_MSGLENGTH];
		LoadMessage( IDMSG_WINASSOCIATEHELPINSTANCE, pszMsg );
		ErrMsgPM( hab, pszMsg );
	}
	return hwndHelp;
}


/*************************************************************************\
 * PROCEDURE SetDefaultOptions
 * Initialize the variable fields of the ProgInfo structure if no valid
 * data is saved in the profile.
\*************************************************************************/
VOID SetDefaultOptions( VOID )
{
	ProgOptions.CellSize = 8;
	ProgOptions.Cols = 50;
	ProgOptions.Rows = 50;
	ProgOptions.topology = TORUS;
	ProgOptions.algorithm = RECURSIV;
	ProgOptions.bGrid = TRUE;
	return;
}
	
/*************************************************************************\
 * PROCEDURE StoreOptions
 * Saves current settings of ProgOptions.
 * 
\*************************************************************************/
VOID StoreOptions( VOID )
{
	HINI hini;
	
	hini = PrfOpenProfile( ProgInfo.hab, ProgInfo.pszIniName );
	if ( hini == NULLHANDLE ) return;
	PrfWriteProfileData(
		hini,	ProgInfo.pszAppTitle, PrfKeyNames.Settings,
		(PVOID)&ProgOptions, sizeof( PROFILEDATA ) );
	PrfCloseProfile( hini );
	return;
}

/*************************************************************************\
 * FUNCTION RestoreOptions
 * Initialize the variable fields of the ProgInfo structure.
 * Returns TRUE if profiledata was successfull retrieved and FALSE
 * an error occured.
 * Note: This function uses same ugly gotos (all referencing the same
 * label)!!!
\*************************************************************************/
BOOL RestoreOptions( VOID )
{
	HINI hini;

	hini = PrfOpenProfile( ProgInfo.hab, ProgInfo.pszIniName );
	if ( hini == NULLHANDLE ) {
		// The ini file doesn't exist and can't be created.
		// Further using of profile functions would result in useing OS2.INI
		// so we only set the default options and return without telling the
		// main program anything about this error.
		SetDefaultOptions();
		return TRUE;
	} else {
		PSZ pszBuffer;
		ULONG ulNumRead = sizeof( PROFILEDATA );
		BOOL bResult;
		unsigned n = strlen( pszVersion ) + 1;

		pszBuffer = (PSZ)alloca( n );
		bResult =
		PrfQueryProfileString( hini, ProgInfo.pszAppTitle, PrfKeyNames.Version,
			NULL, pszBuffer, n );
		if ( !bResult || strcmp( pszVersion, pszBuffer ) ) {
			// The version saved differs --> create a new ini file.
			goto mknewprofile;
		}
		bResult = 
		PrfQueryProfileData(	hini,
			ProgInfo.pszAppTitle, PrfKeyNames.Settings, 
			(PVOID)&ProgOptions, &ulNumRead );
		if ( !bResult || (ulNumRead != sizeof( PROFILEDATA )) )
			goto mknewprofile;
	}
	PrfCloseProfile( hini );
	// Save current options.
	memcpy( &StartupOptions, &ProgOptions, sizeof( PROFILEDATA ) );
	return TRUE;

mknewprofile:
	SetDefaultOptions();
	// Force settings to be saved when the program terminates.
	memset( &StartupOptions, 0, sizeof( PROFILEDATA ) );
	PrfWriteProfileString(
		hini,	ProgInfo.pszAppTitle, PrfKeyNames.Version, pszVersion );
	PrfCloseProfile( hini );
	return TRUE;
}

/*************************************************************************\
 * PROCEDURE InitProgInfo
 * Initialize the fields of the ProgInfo structure.
\*************************************************************************/
static BOOL InitProgInfo( INT argc, CHAR *argv[], PSZ pszBase )
{
	CHAR drive[_MAX_DRIVE];
	CHAR dir[_MAX_DIR];
#if defined (__WATCOMC__)
	CHAR Name[_MAX_NAME];
#else
	CHAR Name[_MAX_FNAME];
#endif
	CHAR ext[_MAX_EXT];
	INT iBaseLen1;

	iBaseLen1 = strlen( pszBase ) + 1;
	Xmalloc( ProgInfo.pszAppTitle, iBaseLen1, return TRUE );
	strcpy( ProgInfo.pszAppTitle, pszBase );
	
	_splitpath( argv[0], drive, dir, Name, ext );
	Xmalloc( ProgInfo.pszProgPath, strlen( drive ) + strlen( dir ) + 1, return TRUE );
	strcpy( ProgInfo.pszProgPath, drive );
	strcat( ProgInfo.pszProgPath, dir );
	Xmalloc( ProgInfo.pszProgName, strlen( Name ) + strlen( ext ) + 1, return TRUE );
	strcpy( ProgInfo.pszProgName, Name );
	strcat( ProgInfo.pszProgName, ext );
	strcpy( ext, ".hlp" );
	Xmalloc( ProgInfo.pszHelpName, iBaseLen1 + strlen( ext ) , return TRUE );
	strcpy( ProgInfo.pszHelpName, pszBase );
	strcat( ProgInfo.pszHelpName, ext );
	strcpy( ext, ".ini" );
	Xmalloc( ProgInfo.pszIniName, iBaseLen1 + strlen( ext ) , return TRUE );
	strcpy( ProgInfo.pszIniName, pszBase );
	strcat( ProgInfo.pszIniName, ext );

	if ( !RestoreOptions() ) return FALSE;

	// Command line parsing would normaly follow here...

	argc = argc;	// To avoid `not referenced' warnings

	return TRUE;
}


/*************************************************************************\
 * FUNCTION CalcClientWindowSize
 * Calculate the size of the client window from the number of rows
 * and columns of the planet.
\*************************************************************************/
static VOID CalcClientWindowSize( PLONG pcx, PLONG pcy )
{
	*pcx = ProgOptions.CellSize * PlanetCols() - 1;
	*pcy = ProgOptions.CellSize * PlanetRows() - 1;
}

/*************************************************************************\
 * PROCEDURE SetMainWindowSize
 * Calculate the size of the main window from the number of rows and
 * columns of the planet.
\*************************************************************************/
BOOL SetMainWindowSize( HWND hwndClient )
{
	RECTL rect;
	HWND hwndFrame;
	SWP swp;
	LONG ldy;

	hwndFrame = WinQueryWindow( hwndClient, QW_PARENT );
	if ( !hwndFrame ) return FALSE;

	rect.xLeft = rect.yBottom = 0;
	CalcClientWindowSize( &rect.xRight, &rect.yTop );
	// Calculate the new size of the frame window.
	WinCalcFrameRect( hwndFrame, &rect, FALSE );
	// Get the current position of the main window
	WinQueryWindowPos( hwndFrame, &swp );
	ldy = swp.cy - (rect.yTop - rect.yBottom);
	WinSetWindowPos( hwndFrame, NULLHANDLE, swp.x, swp.y + ldy,
		rect.xRight - rect.xLeft, rect.yTop - rect.yBottom,
		SWP_MOVE | SWP_SIZE );
	return TRUE;
}

/*************************************************************************\
 * PROCEDURE DrawCallbackFunc
 * Funktion die vom Life Modul aus aufgerufen wird, um die
 * énderungen im Leben des Planeten auf dem Bildschirm
 * darzustellen.
 * (Etwas unschîne Benutzung einer globalen Variablen.)
\*************************************************************************/
static HPS globalhps;
VOID DrawCallbackFunc( int row, int col, int newstate )
{
	DrawCell( globalhps, row, col, (BOOL)newstate ? CLR_BLACK : SYSCLR_WINDOW );
}

/*************************************************************************\
 * PROCEDURE CalcGenThread
 * Prozedur, die als Thread aufgerufen wird, um die nÑchste
 * Generation berechnen zu lassen. 
\*************************************************************************/
VOID CalcGenThread( PVOID pvparm )
{
	PTHREADINFO pti = (PTHREADINFO)pvparm;

	InitThread( pti );
	globalhps = WinGetPS( pti->hwndOwner );
	if ( pti->ulType == TT_STEP ) {
		CalcGen( ProgOptions.algorithm, ProgOptions.topology );
	} else {
		while( !pti->bKill ) CalcGen( ProgOptions.algorithm, ProgOptions.topology );
	}
	WinReleasePS( globalhps );
	globalhps = NULLHANDLE;
	DoneThread( pti, TRUE );
}

/*************************************************************************\
 * PROCEDURE RndSet
 * Places up to n filled cells on our Planet.
\*************************************************************************/
VOID SetRndCells( HWND hwnd, int n ) {
	HPS hps;
	int i;

	Unselect( hwnd );
	hps = WinGetPS( hwnd );
	for ( i = 0; i < n; i++ ) {
		const int r = rand() % ( PlanetRows() + 1) + 1;
		const int c = rand() % ( PlanetCols() + 1) + 1;
		if ( planet[r][c] == 0 ) DrawCell( hps, r, c, CLR_BLACK );
		planet[r][c] = 1;
	}
	WinReleasePS( hps );
}

/*************************************************************************\
 * FUNCTION Unselect
 * Removes the selection.
\************************************************************************/
static VOID Unselect( HWND hwnd )
{
	WinSendMsg( hwnd, MYM_UNSELECT, (MPARAM)0, (MPARAM)0 );
}

/*************************************************************************\
 * FUNCTION SuspendThread
 * Kills the running thread and return boolean value which indicated
 * whether the thread has to be resumed or not.
\************************************************************************/
static BOOL SuspendThread( HWND hwnd )
{
	return (BOOL)WinSendMsg( hwnd, MYM_SUSPEND, (MPARAM)0, (MPARAM)0 );
}

/*************************************************************************\
 * FUNCTION ResumeThread
 * Resumes the calculation of new generations if the parameter 'bRestart'
 * is TRUE. bRestart has to be obtained from a previous call of
 * SuspendThread().
\************************************************************************/
static VOID ResumeThread( HWND hwnd, BOOL bRestart )
{
	WinSendMsg( hwnd, MYM_RESUME, (MPARAM)bRestart, (MPARAM)0 );
}

/*************************************************************************\
 * FUNCTION SelectWindowArea
 * Selects an area in hwnd.
 * Parameters:
 * hwnd:
 * 	Window handle
 * mPosX, mPosY:
 * 	Current mouse position.
\************************************************************************/
static void SelectWindowArea( HWND hwnd, LONG mPosX, LONG mPosY )
{
	// Use the WinTrackRect API-function to select a
	// rectangular region.
	TRACKINFO track;

	track.cxBorder = 
	track.cyBorder = 3;
	track.cxGrid =
	track.cyGrid =
	track.cxKeyboard =
	track.cyKeyboard = ProgOptions.CellSize;
	track.rclTrack.xLeft = mPosX - mPosX % ProgOptions.CellSize - 1;
	track.rclTrack.xRight = track.rclTrack.xLeft + ProgOptions.CellSize + 1;
	track.rclTrack.yTop = mPosY - mPosY % ProgOptions.CellSize + ProgOptions.CellSize;
	track.rclTrack.yBottom = track.rclTrack.yTop - ProgOptions.CellSize - 1;

	WinQueryWindowRect( hwnd, &track.rclBoundary );		
	track.ptlMinTrackSize.x = 
	track.ptlMinTrackSize.y = 0; 
	track.ptlMaxTrackSize.x = track.rclBoundary.xRight + 2;
	track.ptlMaxTrackSize.y = track.rclBoundary.yTop + 2;
	track.fs = TF_RIGHT | TF_BOTTOM | TF_GRID;
	if ( WinTrackRect( hwnd, NULLHANDLE, &track ) ) {
		if ( !WinIsRectEmpty( ProgInfo.hab, &track.rclTrack ) ) {
			RECTL rect;

			Unselect( hwnd );
			WinSetRect( ProgInfo.hab, &rect, 
				(track.rclTrack.xLeft + 1) / ProgOptions.CellSize + 1,
				(track.rclTrack.yBottom + 1) / ProgOptions.CellSize + 1,
				track.rclTrack.xRight / ProgOptions.CellSize + 1,
				track.rclTrack.yTop / ProgOptions.CellSize + 1);
			WinSendMsg( hwnd, MYM_SETSELECT, (MPARAM)&rect, (MPARAM)0 );
			WinInvalidateRect( hwnd, &track.rclTrack, FALSE );
		} // end if !WinIsRectEmpty()
	} // end if WinTrackRect()
}

/*************************************************************************\
 * FUNCTION InsertBitmap
 * Inserts a monochrom bitmap to the planet.
 * Parameters:
 * hwnd:
 * 	Window handle
 * hbmp:
 *		Handle of bitmap to be inserted.
 * bOverwrite:
 *		boolean variable which says whether empty pixel in the bitmap
 *		shall overwrite existing cells or not.
 * Return values:
 * 	TRUE : Bitmap was inserted
 * 	FALSE: Bitmap was not inserted
\************************************************************************/
static BOOL InsertBitmap( const HWND hwnd, const HBITMAP hbmp, const BOOL bOverwrite )
{
	BITMAPINFOHEADER2 bmpih;
	HPS hpsMem;
	BOOL fSuccess;
	unsigned rstart, cstart;

	bmpih.cbFix = 16;
	fSuccess = 	
	GpiQueryBitmapInfoHeader( hbmp, &bmpih );
	if ( !fSuccess ) {
		CHAR pszMsg[MAX_RES_MSGLENGTH];
		
		LoadMessage( IDMSG_GPIQUERYBITMAPINFOHEADER, pszMsg );
		ErrMsgPM( ProgInfo.hab, pszMsg );
		return FALSE;
	}
	if ( (bmpih.cBitCount != 1) || (bmpih.cPlanes != 1) ) {
		CHAR pszMsg[MAX_RES_MSGLENGTH];
		LoadMessage( IDMSG_WRONGBITMAPFORMAT, pszMsg );
		ErrMsg( pszMsg );
		return FALSE;
	}
	// The bitmap has the right format. Now we have to differ between
	// three cases:
	// 1: The bitmap is larger than our window
	//    --> we have to reject it.
	// 2: The bitmap has the same size like the planet
	//    --> replace all cells by the bitmap bits.
	// 3: The bitmap is smaller
	//    --> let the user select the position to place it.

	// Find out if the bitmap fits inside our rectangle and return FALSE
	// if it is to large.
	if ( (bmpih.cx > PlanetCols()) || (bmpih.cy > PlanetRows()) ) {
		CHAR pszBuffer[MAX_RES_MSGLENGTH];
		PSZ pszMsg;
		
		LoadMessage( IDMSG_BITMAPTOLARGE, pszBuffer );
		pszMsg = (PSZ)alloca( strlen( (char*)pszBuffer ) + 4 * 10 );
		sprintf( pszMsg, pszBuffer, PlanetCols(), bmpih.cx, PlanetRows(), bmpih.cy );
		WinMessageBox( HWND_DESKTOP, hwnd,
			pszMsg, "Error", (USHORT)0,
			MB_OK | MB_INFORMATION | MB_APPLMODAL | MB_MOVEABLE );
		
		return FALSE;
	}

	// If it is smaller than the planet let the user decide where to put it.
	if ( (bmpih.cx < PlanetCols()) || (bmpih.cy < PlanetRows()) ) {
		// We use the WinTrackRect API-function to select the insertion position.
		// Retrieve information about the bitmap to be inserted.
		TRACKINFO track;
		ULONG ulSizeX, ulSizeY;
		POINTL pt;				// Mouse pointer position

		// Get mouse position in window coordinates.	
		WinQueryPointerPos( HWND_DESKTOP, &pt );
		WinMapWindowPoints( HWND_DESKTOP, hwnd, &pt, 1 );
	
		// Determine the size of the tracking rectangle
		ulSizeX = bmpih.cx * ProgOptions.CellSize - 1;
		ulSizeY = bmpih.cy * ProgOptions.CellSize - 1;
		track.rclTrack.xLeft = pt.x - ulSizeX / 2;
		track.rclTrack.yBottom = pt.y - ulSizeY / 2;
		// Align bottom left position to grid
		track.rclTrack.xLeft-= track.rclTrack.xLeft % ProgOptions.CellSize;
		track.rclTrack.yBottom-= track.rclTrack.yBottom % ProgOptions.CellSize;
		track.rclTrack.xRight = track.rclTrack.xLeft + ulSizeX;
		track.rclTrack.yTop = track.rclTrack.yBottom + ulSizeY;
		// Get the size of the window. Note that WinQueryWindowRect always
		// returns (0, 0) as the left bottom corner.
		WinQueryWindowRect( hwnd, &track.rclBoundary );
		track.cxBorder = 
		track.cyBorder = 3;
		track.cxGrid =
		track.cyGrid =
		track.cxKeyboard =
		track.cyKeyboard = ProgOptions.CellSize;
	
		track.ptlMinTrackSize.x = 
		track.ptlMinTrackSize.y = 0; 
		track.ptlMaxTrackSize.x = track.rclBoundary.xRight + 2;
		track.ptlMaxTrackSize.y = track.rclBoundary.yTop + 2;
	
		track.fs = TF_MOVE | TF_GRID | TF_SETPOINTERPOS | TF_ALLINBOUNDARY;
		if ( !WinTrackRect( hwnd, NULLHANDLE, &track ) ) {
			// WinTrackRect was probably aborted --> return now
			return FALSE;
		}
		rstart = track.rclTrack.yBottom / ProgOptions.CellSize + 1;
		cstart = track.rclTrack.xLeft / ProgOptions.CellSize + 1;
		WinInvalidateRect( hwnd, &track.rclTrack, FALSE );
	} else {
		// Bitmap has the same size as the planet.
		WinInvalidateRect( hwnd, NULL, FALSE );
		rstart = cstart = 1;
	}
	hpsMem = CreateMemPS( ProgInfo.hab );
	if ( !hpsMem ) {
		CHAR msg[MAX_RES_MSGLENGTH];
		LoadMessage( IDMSG_CREATEMEMPS, msg );
		ErrMsgPM( ProgInfo.hab, msg );
		return FALSE;
	} else {
		UINT row, col;
		POINTL pt;

		GpiSetBitmap( hpsMem, hbmp );
		pt.y = 0;
		if ( bOverwrite ) {
			for ( row = 0; row < bmpih.cy; row++, pt.y++ ) {
				pt.x = 0;
				for ( col = 0; col < bmpih.cx; pt.x++, col++ ) {
					planet[rstart + row][cstart + col] = (TCell)(GpiQueryPel( hpsMem, &pt ) ? 1 : 0);
				}
			} // end for row
		} else { // Only insert living cells.
			for ( row = 0; row < bmpih.cy; row++, pt.y++ ) {
				pt.x = 0;
				for ( col = 0; col < bmpih.cx; pt.x++, col++ ) {
					if ( GpiQueryPel( hpsMem, &pt ) ? 1 : 0 ) 
						planet[rstart + row][cstart + col] = 1;
				} // end for col.
			} // end for row.
		} // end if bOverwrite
		
		DestroyMemPS( hpsMem );
	}
	return TRUE;
}

/*************************************************************************\
 * FUNCTION MakeBitmap
 * This function creates a monochrom bitmap from a selected area.
 * Returns:
 * Bitmap handle if everthing worked correctly and	NULLHANDLE in
 * case of an error.
\*************************************************************************/
HBITMAP MakeBitmap( PRECTL prectSelect )
{
	BITMAPINFOHEADER2 bmpih;
	PBITMAPINFOHEADER2 pbmpihCopy;
	unsigned uByteCt;	// Number of bytes in one bitmap row
	unsigned uIntCt;	// Number of integers in one bitmap row
	ULONG ulLength;	// length of bitmap header inclusivly the palette info.
	PBYTE pBmpData;	// Buffer for bitmap data.
	PBYTE pbyte;
	PCell pCell;
	PRGB2 prgbPal; 
	HBITMAP hbmp;
	HPS hpsMem;
	unsigned row, i, j;

	// PM doesn't need the full bitmap information
	bmpih.cbFix = 16;
	bmpih.cx = prectSelect->xRight - prectSelect->xLeft;
	bmpih.cy = prectSelect->yTop - prectSelect->yBottom;
	bmpih.cPlanes = 1;
	bmpih.cBitCount = 1;
	
	ulLength = bmpih.cbFix + 2 * sizeof( RGB2 );
	pbmpihCopy = (PBITMAPINFOHEADER2)malloc( ulLength );
	if ( !pbmpihCopy ) return NULLHANDLE;
	memcpy( pbmpihCopy, &bmpih, bmpih.cbFix );
	// Set up palette information
	prgbPal = (PRGB2)( (PBYTE)pbmpihCopy + bmpih.cbFix );
	*(PULONG)prgbPal = RGB_WHITE;
	prgbPal++;
	*(PULONG)prgbPal = RGB_BLACK;
	// Calculate the size of the bitmap data (pixels) and allocate memory
	uIntCt = ((( bmpih.cBitCount * bmpih.cx ) + 31 ) / 32 );
	uByteCt = 4 * uIntCt;
	ulLength = uByteCt * bmpih.cy * bmpih.cPlanes;
	pBmpData = (PBYTE)malloc( ulLength );
	if ( !pBmpData ) return NULLHANDLE;
	// Delete buffer
	memset( pBmpData, 0, ulLength );
	// Set pixels

	pbyte = pBmpData;
	for( row = 0; row < bmpih.cy; row++ ) {
		pCell = &planet[prectSelect->yBottom + row][prectSelect->xLeft];
		pbyte = pBmpData + row * uByteCt;
		for ( i = bmpih.cx / 8; i; i-- ) {
			// Loop through all full bytes in one line
			j = 7;
			do {						// Loop through all bits of this byte.
				*pbyte|= (*pCell & 1) << j;
				pCell++;
			} while( j-- );
			pbyte++;
		} // end for i
		for( i = bmpih.cx % 8, j = 7 ; i; i--, j--, pCell++ ) {
			*pbyte|= (*pCell & 1) << j;
		}
	} // end for row

	// All pixels have been set up - now create the bitmap
	hpsMem = CreateMemPS( ProgInfo.hab );
   hbmp =
   GpiCreateBitmap(
      hpsMem,							// Presentation-space handle
      &bmpih,	             		// Address of structure for format data
      CBM_INIT,              		// Options: create bitmap bits now
      pBmpData,                	// Address of buffer of image data
      (PBITMAPINFO2)pbmpihCopy); // Address of structure for color and format
	DestroyMemPS( hpsMem );
	return hbmp;
}

/*************************************************************************\
 * FUNCTION FileDlg
 * This function opens a file dialog box for loading and saveing bitmaps.
 * Parameters:
 * 	hwnd:
 * 		Window handle
 * 	pszFileName:
 *			FileName that was selected
 * 	fAction:
 * 		0: Select filename for loading
 *			other: Select filename for saving
 * Returns:
 *		TRUE : Filename was sucessfully selected.
 *		FALSE: Selection was aborted or an error occured.
\*************************************************************************/
BOOL FileDlg( HWND hwnd, PSZ pszFileName, BOOL fAction )
{
	FILEDLG fdlg;
	// If this dialog is executed several times we want to bring it up
	// with the last selected path.
	static CHAR szFullFile[CCHMAXPATH] = "";
	PCHAR pch;
	CHAR szTitle[MAX_RES_STRLENGTH];
	
	HWND hwndDlg;

	fdlg.cbSize = sizeof( FILEDLG );
	if ( fAction ) {
		// We want to save an image
		LoadString( IDSTR_SAVEDLGTITLE, szTitle );
		fdlg.fl = FDS_SAVEAS_DIALOG | FDS_CENTER | FDS_HELPBUTTON | FDS_ENABLEFILELB;
	} else {
		LoadString( IDSTR_LOADDLGTITLE, szTitle );
		fdlg.fl = FDS_OPEN_DIALOG | FDS_CENTER | FDS_HELPBUTTON;
	}
	fdlg.pszTitle = szTitle;
	fdlg.ulUser = 0;
	fdlg.pszOKButton = NULL;
	fdlg.pfnDlgProc = NULL;
	fdlg.hMod = NULLHANDLE;
	fdlg.x = 0;
	fdlg.y = 0;
	fdlg.usDlgId = 0;
	// Initial selection:
	pch = (PCHAR)strrchr( szFullFile, '\\' );
	if ( pch ) *(pch + 1) = 0;

	strcpy( fdlg.szFullFile, szFullFile );
	strcat( fdlg.szFullFile, "*.bmp" );
	
	// EA type list & initial type
	fdlg.pszIType = NULL;
	fdlg.papszITypeList = NULL;
	// Drive list & initial drive
	fdlg.pszIDrive = NULL;
	fdlg.papszIDriveList = NULL;
	fdlg.sEAType = 0;
	fdlg.papszFQFilename = NULL;
	fdlg.ulFQFCount = 0L;

	hwndDlg =
	WinFileDlg( HWND_DESKTOP, hwnd, &fdlg );
	if ( !hwndDlg ) {
		CHAR pszMsg[MAX_RES_MSGLENGTH];

		LoadMessage( IDMSG_WINFILEDLG, pszMsg );
		ErrMsgPM( ProgInfo.hab, pszMsg );
		return FALSE;
	}
	if ( fdlg.lReturn == DID_OK ) {
		strcpy( pszFileName, fdlg.szFullFile );
		strcpy( szFullFile, fdlg.szFullFile );
	}
	return (fdlg.lReturn == DID_OK);
}

/*************************************************************************\
 * Client-window-procedure of the main window.
 * All the message handling takes place here.
\*************************************************************************/
MRESULT EXPENTRY MainClientProc(	HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
	// I use static variable which are initialize during the WM_CREATE
	// message. Note that it would be better to use window pointers instead
	// as they allow several instances of one class to be created. However
	// this program is designed to create only one window.
	static BOOL bInputAllowed; // Flag whether input is allowed or not.
										// (Input is not allowed if a new generation
										// is calculated.)
	static int iCutOpt;			// Where to cut the planet if it is resized.
										// This variable is set if the window recieves
										// a MYM_SETCUTOPT message. This message is
										// sent by the frame window if the frame
										// is resized by user interaction.
	static PTHREADINFO ptiGenThread;	// Pointer to a THREADINFO block of
										// of the generation calculation thread.
	static BOOL bSelected;			// Specifies if a selection is active.
	static RECTL rectSelect;	// If a area is selected its extend is saved
										// here. The selected area is save in planet
										// coordinates i.e. the bottom left corner is
										// (1,1). Note that the rectangle is
										// non-inclusive i.e. the top-right corner of
										// the selected area is (rectSelect.xRight - 1,
										// rect.yTop - 1).
	static RECTL StartupPos;	// Position of this window when it was created.
	static enum {
		MMI_IGNORE,
		MMI_SET,
		MMI_DELETE
	} iMouseMoveInput;
	// Note: All static variable are initialized during the WM_CREATE message.

	switch ( msg ) {
		case WM_BUTTON1DOWN: {
			// Mouse button 1 was pressed. If no thread is currently
			// running (bInputAllowed == TRUE) and the input mode isn't set
			// we check if the control key was pressed.
			// In this case the user can select a region for editing.
			// Otherwise we simply set the state of the cell at mouse
			// cursor position to one.
			const USHORT fsflags = SHORT2FROMMP(mp2);
			// Check if input is currently allowed and if no input mode is set
			// i.e. mouse button 2 was pressed before and is still hold down.
			if ( bInputAllowed && iMouseMoveInput == MMI_IGNORE ) {
				if ( fsflags & KC_CTRL ) {
					const LONG mPosX = (LONG)SHORT1FROMMP(mp1);
					const LONG mPosY = (LONG)SHORT2FROMMP(mp1);
					SelectWindowArea( hwnd, mPosX, mPosY );
				} else {	// Ctrl-key was not pressed
					// Make the cell at mouse cursor pos. alive.
					const int r = SHORT2FROMMP(mp1) / ProgOptions.CellSize + 1;
					const int c = SHORT1FROMMP(mp1) / ProgOptions.CellSize + 1;
					if ( !planet[r][c] ) {
						planet[r][c] = 1;
						UpdateCell( hwnd, r, c );
					}
					iMouseMoveInput = MMI_SET;
					WinSetCapture( HWND_DESKTOP, hwnd );
				}
			} // end if bInputAllowed
			break;
		}

		case WM_BUTTON1UP:
			if ( iMouseMoveInput == MMI_SET ) {
				iMouseMoveInput = MMI_IGNORE;
				WinSetCapture( HWND_DESKTOP, NULLHANDLE );
			}
			break;
		// end case WM_BUTTON1UP

		case WM_BUTTON2DOWN:
			if ( bInputAllowed && iMouseMoveInput == MMI_IGNORE ) {
				// Delete a cell only if the control key was not pressed.
				const int r = SHORT2FROMMP(mp1) / ProgOptions.CellSize + 1;
				const int c = SHORT1FROMMP(mp1) / ProgOptions.CellSize + 1;

				if ( planet[r][c] ) {
					planet[r][c] = 0;
					UpdateCell( hwnd, r, c );
				}
				iMouseMoveInput = MMI_DELETE;
				WinSetCapture( HWND_DESKTOP, hwnd );
			} 
			break;
		// end case WM_BUTTON2DOWN

		case WM_BUTTON2UP:
			if ( iMouseMoveInput == MMI_DELETE ) {
				iMouseMoveInput = MMI_IGNORE;
				WinSetCapture( HWND_DESKTOP, NULLHANDLE );
			}
			break;
		// end case WM_BUTTON2UP

		case WM_MOUSEMOVE: {
			if ( bInputAllowed && (iMouseMoveInput != MMI_IGNORE) ) {
				const int r = SHORT2FROMMP(mp1) / ProgOptions.CellSize + 1;
				const int c = SHORT1FROMMP(mp1) / ProgOptions.CellSize + 1;
				if ( r >= 1 && r <= PlanetRows() && c >= 1 && c <= PlanetCols() ) {
					int op = (iMouseMoveInput == MMI_SET) ? 1 : 0;
					
					if ( planet[r][c] != op ) {
						// Zeichne nur falls sich der Status der Zelle
						// verÑndert hat.
						planet[r][c] = op;
						UpdateCell( hwnd, r, c );
					}
				}
			}
			break;
		} // end case WM_MOUSEMOVE

		case WM_PAINT: {
			BOOL bRestart = SuspendThread( hwnd );

			// The windows has to be updated
			DrawPlanet( hwnd, (bSelected ? &rectSelect : NULL) );
			ResumeThread( hwnd, bRestart );
        	return (MRESULT)0;
		} // end case WM_PAINT
			
		case WM_WINDOWPOSCHANGED: {
			PSWP pswp = (PSWP)mp1;
			BOOL bRestart;

			// We only want to have an eye on resizing
			if ( !(pswp->fl & SWP_SIZE) ) break;
			// Show and hide is also ignored.
			if ( (pswp->fl & SWP_SHOW) ) break;
			if ( (pswp->fl & SWP_HIDE) ) break;

			// If the thread is running wait for it to finish.
			bRestart = SuspendThread( hwnd );
			Unselect( hwnd );
			ProgOptions.Cols = (pswp->cx + 1) / ProgOptions.CellSize;
			ProgOptions.Rows = (pswp->cy + 1) / ProgOptions.CellSize;
			ResizePlanet( ProgOptions.Rows, ProgOptions.Cols, iCutOpt );
			iCutOpt = 0;
			ResumeThread( hwnd, bRestart );
			break; // Continue with default processing of this message
		} // end case WM_WINDOWSPOSCHANGED

		case MYM_STARTTHREAD: {
			const INT iThreadType = (INT)mp1;

			switch( iThreadType ) {
				case TT_STEP: {
					Unselect( hwnd );
					InitThreadInfo( ptiGenThread, sizeof( THREADINFO ), TT_STEP, hwnd );
					EnableMenuItem( hwnd, IDM_RUN, FALSE );
					EnableMenuItem( hwnd, IDM_STEP, FALSE );
					EnableMenuItem( hwnd, IDM_PASTE, FALSE );
					EnableMenuItem( hwnd, IDM_SAVE, FALSE );
					EnableMenuItem( hwnd, IDM_SAVEALL, FALSE );
					EnableMenuItem( hwnd, IDM_LOAD, FALSE );
					bInputAllowed = FALSE;
					break;
				} // end case TT_STEP
				case TT_RUN: 
					Unselect( hwnd );
					InitThreadInfo( ptiGenThread, sizeof( THREADINFO ), TT_RUN, hwnd );
					bInputAllowed = FALSE;
					EnableMenuItem( hwnd, IDM_STOP, TRUE );
					EnableMenuItem( hwnd, IDM_STEP, FALSE );
					EnableMenuItem( hwnd, IDM_PASTE, FALSE );
					EnableMenuItem( hwnd, IDM_SAVE, FALSE );
					EnableMenuItem( hwnd, IDM_SAVEALL, FALSE );
					EnableMenuItem( hwnd, IDM_LOAD, FALSE );
					break;
				// end case TT_RUN
				default: {
					CHAR pszMsg[MAX_RES_MSGLENGTH];
					LoadMessage( IDMSG_UNKOWNTHREADSTART, pszMsg );
					ErrMsg( pszMsg );
					return (MRESULT)0;
				} // end default
			} // end switch iThreadType

			assert( ptiGenThread->bDead );
			if ( (ptiGenThread->tid = (TID)BeginThread( CalcGenThread, ptiGenThread )) == -1 ) {
				CHAR msg[MAX_RES_MSGLENGTH];
				LoadMessage( IDMSG_BEGINTHREAD, msg );
				ErrMsgRunLib( msg );
				ptiGenThread->ulResult = (ULONG)-1;
				ptiGenThread->bDead = TRUE;
			}
			return (MRESULT)0;
		} // end case MYM_STARTTHREAD
		
		case MYM_ENDTHREAD: {
			const ULONG iThreadType = LONGFROMMP( mp1 );
			const PTHREADINFO pti = (PTHREADINFO)PVOIDFROMMP(mp2);

			while( !pti->bDead ) DosSleep( 1 );

			switch( iThreadType ) {
				case TT_STEP: {
					bInputAllowed = TRUE;
					EnableMenuItem( hwnd, IDM_RUN, TRUE );
					EnableMenuItem( hwnd, IDM_STEP, TRUE );
					EnableMenuItem( hwnd, IDM_SAVEALL, TRUE );
					EnableMenuItem( hwnd, IDM_LOAD, TRUE );
					// Reset the state of the paste menu item.
					WinPostMsg( hwnd, WM_DRAWCLIPBOARD, (MPARAM)0, (MPARAM)0 );
					break;
				} // end case TT_STEP
				case TT_RUN: 
					bInputAllowed = TRUE;
					EnableMenuItem( hwnd, IDM_STEP, TRUE );
					EnableMenuItem( hwnd, IDM_STOP, FALSE );
					EnableMenuItem( hwnd, IDM_SAVEALL, TRUE );
					EnableMenuItem( hwnd, IDM_LOAD, TRUE );
					// Reset the state of the paste menu item.
					WinPostMsg( hwnd, WM_DRAWCLIPBOARD, (MPARAM)0, (MPARAM)0 );
					break;
				// end case TT_RUN
				default: {
					CHAR pszMsg[MAX_RES_MSGLENGTH];
					LoadMessage( IDMSG_UNKOWNTHREADEND, pszMsg );
					ErrMsg( pszMsg );
					return (MRESULT)0;
				}
			} // end switch iThreadType
			return (MRESULT)0;
		} // end case MYM_ENDTHREAD

		case MYM_UPDATECELL: {
			POINTL pt;
			LONG lColor;
			HPS hps;

			pt.y = (ULONG)mp1;
			pt.x = (ULONG)mp2;
			assert( pt.y && pt.x );
			assert( pt.y <= PlanetRows() && pt.x <= PlanetCols() );
			if ( bSelected && WinPtInRect( ProgInfo.hab, &rectSelect, &pt ) ) {
				lColor = planet[pt.y][pt.x] ? CLR_DARKBLUE : CLR_PALEGRAY;
			} else {
				lColor = planet[pt.y][pt.x] ? CLR_BLACK : SYSCLR_WINDOW;
			}
			hps = WinGetPS( hwnd );
			DrawCell( hps, pt.y, pt.x, lColor );
			WinReleasePS( hps );
			return (MRESULT)0;
		}

		case MYM_KILLTHREAD: {
			const PTHREADINFO pti = (PTHREADINFO)PVOIDFROMMP( mp1 );

			// If the thread is running wait for it to finish.
			pti->bKill = TRUE;
			while( !pti->bDead ) DosSleep( 1 );	// Give up time slice
			return (MRESULT)0;
		} // end case MYM_KILLTHREAD

		case MYM_SUSPEND: {
			BOOL bRestart;
			
			if ( ptiGenThread->bDead ) return (MRESULT)FALSE;
			// Check whether the thread has to be restarted.
			// The value is passed back to the caller and can be used
			// for the MYM_RESUME message.
			bRestart = ptiGenThread->ulType == TT_RUN;
			// Wait for the thread to finish.
			WinSendMsg( hwnd, MYM_KILLTHREAD, (MPARAM)ptiGenThread, (MPARAM)0 );
			return (MRESULT)bRestart;
		} // end case MYM_SUSPEND

		case MYM_RESUME: {
			BOOL bRestart = (BOOL)mp1;
			if (bRestart) WinPostMsg( hwnd, MYM_STARTTHREAD, (MPARAM)TT_RUN, (MPARAM)0 );
			return (MRESULT)0;
		} // end case MYM_RESUME
		
		case MYM_ISRUNNING:
			return (MRESULT)(!ptiGenThread->bDead);
		// end case MYM_ISRUNNING

		case MYM_SETCUTOPT:
			iCutOpt = (int)mp1;
			return (MRESULT)0;
		// end case MYM_SETCUTOPT
		
		case MYM_SETSELECT:
			bSelected = TRUE;
			rectSelect = *(PRECTL)mp1;
			EnableMenuItem( hwnd, IDM_CUT, TRUE );
			EnableMenuItem( hwnd, IDM_COPY, TRUE );
			EnableMenuItem( hwnd, IDM_SAVE, TRUE );
			return (MRESULT)0;
		// end case MYM_SETSELECT

		case MYM_UNSELECT: 
			if ( bSelected ) {
				RECTL rectInvalidate;
				
				bSelected = FALSE;
				EnableMenuItem( hwnd, IDM_CUT, FALSE );
				EnableMenuItem( hwnd, IDM_COPY, FALSE );
				EnableMenuItem( hwnd, IDM_SAVE, FALSE );
				rectInvalidate.xLeft = (rectSelect.xLeft - 1) * ProgOptions.CellSize;
				rectInvalidate.xRight = rectSelect.xRight * ProgOptions.CellSize;
				rectInvalidate.yBottom = (rectSelect.yBottom - 1) * ProgOptions.CellSize;
				rectInvalidate.yTop = rectSelect.yTop * ProgOptions.CellSize;
				WinInvalidateRect( hwnd, &rectInvalidate, FALSE );
			}
			return (MRESULT) 0;
		// end case MYM_UNSELECT

		/****************************************************************\
		 * Menu command are handled here:
		\****************************************************************/
		case WM_COMMAND: {
			const USHORT usCmdId = SHORT1FROMMP( mp1 );
			switch ( usCmdId ) {

				// *** File messages ***

				case IDM_LOAD: {
					CHAR szFileName[CCHMAXPATH];

					if ( FileDlg( hwnd, szFileName, 0 ) ) {
						HPS hpsMem;
						HBITMAP hbmp;

						WinUpdateWindow( hwnd );
						hpsMem = CreateMemPS( ProgInfo.hab );
						hbmp = LoadBitmapFile( hpsMem, szFileName );
						InsertBitmap( hwnd, hbmp, TRUE );
						GpiDeleteBitmap( hbmp );
						DestroyMemPS( hpsMem );
					}
					return (MRESULT)0;
				} // end case IDM_LOAD

				case IDM_SAVE: {
					CHAR szFileName[CCHMAXPATH];

					if ( FileDlg( hwnd, szFileName, 1 ) ) {
						HBITMAP hbmp;

						hbmp = MakeBitmap( &rectSelect );
						SaveBitmapFile( ProgInfo.hab, hbmp, szFileName );
						GpiDeleteBitmap( hbmp );
						return (MRESULT)0;
					}
					return (MRESULT)0;
				} // end case IDM_SAVE
				
				case IDM_SAVEALL: {
					CHAR szFileName[CCHMAXPATH];

					if ( FileDlg( hwnd, szFileName, 1 ) ) {
						HBITMAP hbmp;
						RECTL rect;
	
						rect.xLeft =
						rect.yBottom = 1;
						rect.xRight = PlanetCols() + 1;
						rect.yTop = PlanetRows() + 1;
						hbmp = MakeBitmap( &rect );
						SaveBitmapFile( ProgInfo.hab, hbmp, szFileName );
						GpiDeleteBitmap( hbmp );
						return (MRESULT)0;
					}
					return (MRESULT)0;
				} // end case IDM_SAVEALL
				
				// *** Life messages ***

				case IDM_STEP: {
					WinSendMsg( hwnd, MYM_STARTTHREAD, (MPARAM)TT_STEP, (MPARAM)0 );
					return (MRESULT)0;
				} // end case IDM_STEP 

				// The STOP menu item is only enabled if RUN was selected
				// previously. So we can handle the IDM_STOP and IDM_RUN
				// message in the same way.
				case IDM_STOP:
				case IDM_RUN: {
					// Toggle between running and `do nothing' state.
					if ( ptiGenThread->bDead  ) {
						WinPostMsg( hwnd, MYM_STARTTHREAD, (MPARAM)TT_RUN, (MPARAM)0 );
					} else {
						WinSendMsg( hwnd, MYM_KILLTHREAD, (MPARAM)ptiGenThread, (MPARAM)0 );
					}
					return (MRESULT)0;
				} // end case IDM_RUN

				case IDM_CLEAR: 
					// Wait for the thread to finish, but don't restart.
					WinSendMsg( hwnd, MYM_KILLTHREAD, (MPARAM)ptiGenThread, (MPARAM)0 );
					// Clear the world
					ClearPlanet();
					// Mark the total window for repainting
					WinInvalidateRect( hwnd, NULL, FALSE );
					return (MRESULT)0;
				// end case IDM_CLEAR

				case IDM_RANDOMIZE:
					// Wait for the thread to finish.
					WinSendMsg( hwnd, MYM_KILLTHREAD, (MPARAM)ptiGenThread, (MPARAM)0 );
					SetRndCells( hwnd, 50 );
					return (MRESULT)0;
				// end case IDM_RANDOMIZE

				// *** Option messages ***

				case IDM_OPTALGSEQ:
					ProgOptions.algorithm = SEQUENTIAL;
					SetMenuItemCheck( hwnd, IDM_OPTALGSEQ, TRUE );
					SetMenuItemCheck( hwnd, IDM_OPTALGREC, FALSE );
					return (MRESULT)0;

				case IDM_OPTALGREC:
					ProgOptions.algorithm = RECURSIV;
					SetMenuItemCheck( hwnd, IDM_OPTALGSEQ, FALSE );
					SetMenuItemCheck( hwnd, IDM_OPTALGREC, TRUE );
					return (MRESULT)0;

				case IDM_OPTTOPFINIT:
					ProgOptions.topology = FINIT;
					SetMenuItemCheck( hwnd, IDM_OPTTOPFINIT, TRUE );
					SetMenuItemCheck( hwnd, IDM_OPTTOPTORUS, FALSE );
					return (MRESULT)0;

				case IDM_OPTTOPTORUS:
					ProgOptions.topology = TORUS;
					SetMenuItemCheck( hwnd, IDM_OPTTOPFINIT, FALSE );
					SetMenuItemCheck( hwnd, IDM_OPTTOPTORUS, TRUE );
					return (MRESULT)0;

				case IDM_OPTSIZE1x1:
				case IDM_OPTSIZE2x2:
				case IDM_OPTSIZE3x3:
				case IDM_OPTSIZE4x4:
				case IDM_OPTSIZE5x5:
				case IDM_OPTSIZE6x6:
				case IDM_OPTSIZE7x7:
				case IDM_OPTSIZE8x8:
				case IDM_OPTSIZE9x9:
				case IDM_OPTSIZE10x10: {
					BOOL bRestart;
					UINT uiNewSize;

					uiNewSize = (UINT)(usCmdId - IDM_OPTSIZE);
					// If nothing has changed do nothing.
					if ( uiNewSize == ProgOptions.CellSize ) return (MRESULT)0;

					// If the thread is running wait for it to finish.
					bRestart = SuspendThread( hwnd );

					// First unmark the current size menu items
					SetMenuItemCheck( hwnd, IDM_OPTSIZE + ProgOptions.CellSize, FALSE );
					// Set the new size.
					ProgOptions.CellSize = uiNewSize;
					// Resize the window
					SetMainWindowSize( hwnd );
					// Mark the selected size menu item
					SetMenuItemCheck( hwnd, usCmdId, TRUE );
					ResumeThread( hwnd, bRestart );
					return (MRESULT)0;
				}

				case IDM_OPTGRID:
					ProgOptions.bGrid = !ProgOptions.bGrid;
					SetMenuItemCheck( hwnd, IDM_OPTGRID, ProgOptions.bGrid );
					if ( ProgOptions.CellSize != 1 ) {
						// Let the whole window be redrawn.
						// Third parameter = TRUE: redraw now
						WinInvalidateRect( hwnd, NULL, TRUE );
					}
					return (MRESULT)0;

				// *** Edit messages ***

				case IDM_COPY: {
					HBITMAP hbmp;
					BOOL fSuccess;

					assert( bSelected );
					hbmp = MakeBitmap( &rectSelect );
					if ( !hbmp ) {
						CHAR pszMsg[MAX_RES_MSGLENGTH];
						LoadMessage( IDMSG_MAKEBITMAP, pszMsg );
						ErrMsg( pszMsg );
						return (MRESULT)0;
					}
					fSuccess = WinOpenClipbrd( ProgInfo.hab );
					if ( !fSuccess ) {
						CHAR pszMsg[MAX_RES_MSGLENGTH];
						LoadMessage( IDMSG_WINOPENCLIPBRD, pszMsg );
						ErrMsgPM( ProgInfo.hab, pszMsg );
						return (MRESULT)0;
					}
					WinEmptyClipbrd( ProgInfo.hab );
					fSuccess = 
					WinSetClipbrdData( ProgInfo.hab, (ULONG)hbmp, CF_BITMAP, CFI_HANDLE );
					if ( !fSuccess ) {
						CHAR pszMsg[MAX_RES_MSGLENGTH];
						LoadMessage( IDMSG_WINSETCLIPBRDDATA, pszMsg );
						ErrMsgPM( ProgInfo.hab, pszMsg );
					}
					WinCloseClipbrd( ProgInfo.hab );
					Unselect( hwnd );
					return (MRESULT)0;
				} // end case IDM_COPY

				case IDM_CUT: {
					RECTL rectSave;
					int row;
					int cols;

					rectSave = rectSelect;
					WinSendMsg( hwnd, WM_COMMAND, (MPARAM)IDM_COPY, (MPARAM)0 );
					cols = rectSave.xRight - rectSave.xLeft;
					for ( row = rectSave.yBottom; row < rectSave.yTop; row++ ) {
						memset( &planet[row][rectSave.xLeft], 0, sizeof( TCell ) * cols );
					}
					return (MRESULT)0;
				} // end case IDM_CUT

				case IDM_PASTE: {
					HBITMAP hbmp;
					BOOL fSuccess;
					
					fSuccess = WinOpenClipbrd( ProgInfo.hab );
					if ( !fSuccess ) {
						CHAR pszMsg[MAX_RES_MSGLENGTH];
						LoadMessage( IDMSG_WINOPENCLIPBRD, pszMsg );
						ErrMsgPM( ProgInfo.hab, pszMsg );
					} else {
						hbmp = WinQueryClipbrdData( ProgInfo.hab, CF_BITMAP );
						InsertBitmap( hwnd, hbmp, TRUE );
						WinCloseClipbrd( ProgInfo.hab );
					}
					return (MRESULT)0;
				} // end case IDM_PASTE

				// *** Help messages ***

				case IDM_HELP4HELP:
					WinPostMsg(ProgInfo.hwndHelp, HM_DISPLAY_HELP, NULL, NULL);
					return (MRESULT)0;
					
				case IDM_ABOUT:
					WinDlgBox( HWND_DESKTOP, hwnd,
						WinDefDlgProc,
						NULLHANDLE, IDD_ABOUT, NULL );
					return (MRESULT)0;
				// end case IDM_ABOUT

					
			} // end switch (usMenuItemId)
			break;
		} // end case WM_COMMAND

		case WM_CREATE: {
			HWND hwndFrame;
			HINI hini;
			INT result;
			SWP swp;
			USHORT usMenuId;

			bInputAllowed = TRUE;
			bSelected = FALSE;
			iMouseMoveInput = MMI_IGNORE;
			iCutOpt = 0;

			Xmalloc( ptiGenThread, sizeof( THREADINFO ), return(MRESULT)TRUE );
			ptiGenThread->bDead = TRUE;
			ptiGenThread->tid = -1;
			
			result =
			InitPlanet(	ProgOptions.Rows,	ProgOptions.Cols,	DrawCallbackFunc );
			if ( !result ) return (MRESULT)TRUE;

			if ( !(hwndFrame = WinQueryWindow( hwnd, QW_PARENT )) )
				return (MRESULT)TRUE;
			hini = PrfOpenProfile( ProgInfo.hab, ProgInfo.pszIniName );
			if ( (hini == NULLHANDLE) || !RestoreWindowPos( hini, ProgInfo.pszAppTitle, PrfKeyNames.WndPos, hwndFrame ) ) {
				SetMainWindowSize( hwnd );
				CenterWindow( HWND_DESKTOP, hwndFrame );
			} else {
				SetMainWindowSize( hwnd );
			}
			if ( hini != NULLHANDLE ) PrfCloseProfile( hini );

			WinQueryWindowPos( hwnd, &swp );
			// Save the window startup position so that we will save it only
			// if it has changed.
			StartupPos.xLeft = swp.x;
			StartupPos.yBottom = swp.y;
			StartupPos.xRight = swp.x + swp.cx;
			StartupPos.yTop = swp.y + swp.cy;
			// Set up the menu item checks.
			SetMenuItemCheck( hwnd, IDM_OPTGRID, ProgOptions.bGrid );
			SetMenuItemCheck( hwnd, (ProgOptions.algorithm == RECURSIV) ? IDM_OPTALGREC : IDM_OPTALGSEQ, TRUE );
			SetMenuItemCheck( hwnd, (ProgOptions.topology == TORUS) ? IDM_OPTTOPTORUS : IDM_OPTTOPFINIT, TRUE );
			
			usMenuId = IDM_OPTSIZE + ProgOptions.CellSize;
			SetMenuItemCheck( hwnd, usMenuId, TRUE );
			// Now activate the window
			WinSetWindowPos( hwndFrame, NULLHANDLE, 0, 0, 0, 0, SWP_ACTIVATE );
			break; // Continue with default WM_CREATE processing
		} // end case WM_CREATE

		case WM_CLOSE: {
			HWND hwndFrame;
			HINI hini;
			SWP swp;

			// Note: I have often read that all resources are freed if a
			// program terminates, but I have made the experience that OS/2
			// behaves very unstable if you let any presentation space open.
			// This is why I wait here for an unfinished thread. However, I
			// don't want to lock the PM message queue to long.
			if ( !ptiGenThread->bDead ) {
				static int MaxWait = 100;
				ptiGenThread->bKill = TRUE;
				while( !ptiGenThread->bDead && MaxWait-- ) DosSleep( 1 );
			}
			// Store program settings if they have changed
			if ( memcmp( &ProgOptions, &StartupOptions, sizeof(PROFILEDATA) ) ) {
				StoreOptions();
			}
			// Store current window position
			hwndFrame = WinQueryWindow( hwnd, QW_PARENT );
			if ( !hwndFrame ) break;
			hini = PrfOpenProfile( ProgInfo.hab, ProgInfo.pszIniName );
			if ( !hini ) break;
			WinQueryWindowPos( hwnd, &swp );
			// Only store the current window position if it has changed since
			// the program has started.
			if ( ( StartupPos.xLeft != swp.x ) || ( StartupPos.xRight != swp.x + swp.cx )
				||( StartupPos.yBottom != swp.y ) || ( StartupPos.yTop != swp.y + swp.cy ) ) {
				StoreWindowPos( hini, ProgInfo.pszAppTitle, PrfKeyNames.WndPos, hwndFrame );
			}
			PrfCloseProfile( hini );
			break;
		} // end case WM_CLOSE

		case WM_ACTIVATE: {
			if ((BOOL)mp1 ) {
				// Window was activated:
				// Set this window to be the clipboard viewer. If it fails
				// disable the `PASTE' command.
				// If the WinSetClipbrdViewer() call was successfull PM
				// posts a WM_DRAWCLIPBOARD message, where we will examine
				// if the clipboard data is useable.
				if ( !WinSetClipbrdViewer( ProgInfo.hab, hwnd ) )
					EnableMenuItem( hwnd, IDM_PASTE, FALSE );
			} else {
				// Window was deactivated:
				// Disable the `PASTE' command.
				WinSetClipbrdViewer( ProgInfo.hab, NULLHANDLE );
			}
			break;		// continue with default processing
		} // end case WM_ACTIVATE

		case WM_DRAWCLIPBOARD: {
			// This message is send to us whenever the clipboarddata
			// has changed but only if it is activated. This is achieved by
			// setting the clibboard view during the processing of the
			// WM_ACTIVATE message (see above).
			// Check if the data saved in the clipboard is valid for our
			// purposes i.e. make sure that the bitmap is monochrom.
			BOOL fEnable = FALSE;
			BOOL fSuccess;
			
			fSuccess = WinOpenClipbrd( ProgInfo.hab );
			if ( !fSuccess ) {
				CHAR pszMsg[MAX_RES_MSGLENGTH];

				LoadMessage( IDMSG_WINOPENCLIPBRD, pszMsg );
				ErrMsgPM( ProgInfo.hab, pszMsg );
			} else {
				HBITMAP hbmp;

				hbmp = WinQueryClipbrdData( ProgInfo.hab, CF_BITMAP );
				if ( hbmp ) {
					BITMAPINFOHEADER2 bmpih;
	
					bmpih.cbFix = 16;
					fSuccess = GpiQueryBitmapInfoHeader( hbmp, &bmpih );
					if ( !fSuccess ) {
						CHAR pszMsg[MAX_RES_MSGLENGTH];
						LoadMessage( IDMSG_GPIQUERYBITMAPINFOHEADER, pszMsg );
						ErrMsgPM( ProgInfo.hab, pszMsg );
					} else {
						fEnable = (bmpih.cPlanes == 1) && (bmpih.cBitCount == 1);
					}
				}
				WinCloseClipbrd( ProgInfo.hab );
			}

			EnableMenuItem( hwnd, IDM_PASTE, fEnable );
			break;
		} // end case WM_DRAWCLIPBOARD
		
	} // end switch msg
	return WinDefWindowProc( hwnd, msg, mp1, mp2 );
}

/*************************************************************************\
 * Frame-Window-procedure of the main window
 * The frame window procedure of the main window is subclassed because
 * we want to control resizing.
\*************************************************************************/
MRESULT EXPENTRY MainFrameProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
	switch ( msg ) {
#if 0
// Doesn't work so far.
		case WM_ADJUSTWINDOWPOS: {
			PSWP pswp = (PSWP)mp1;
			RECTL rect;
			ULONG cx, cy;		// Wished size of the client window.

			
			if ( !(pswp->fl & SWP_SIZE) ) break;
			// Calculate the size of the client window with the current 
			// frame size.
			rect.xLeft = pswp->x;
			rect.xRight = pswp->x + pswp->cx;
			rect.yBottom = pswp->y;
			rect.yTop = pswp->y + pswp->cy;
			WinCalcFrameRect( hwnd, &rect, TRUE );

			ProgOptions.Cols = (rect.xRight - rect.xLeft + 1) / ProgOptions.CellSize;
			ProgOptions.Rows = (rect.yTop - rect.yBottom + 1) / ProgOptions.CellSize;
			ResizePlanet( ProgOptions.Rows, ProgOptions.Cols, 0 );
			CalcClientWindowSize( &cx, &cy );
			// cx, cy are how holding the size values, so that the cells are
			// aligned at the window boundaries
			pswp->cx-= rect.xRight - rect.xLeft - cx;
			pswp->y+= rect.yTop - rect.yBottom - cy;
			pswp->cy-= rect.yTop - rect.yBottom - cy;
			
			break;
		}
#endif			
		case WM_TRACKFRAME: {
			TRACKINFO track;
			RECTL rect;
			SWP swp;
			int iCutOpt;

			// We examine this message only if the window is to be resized.
			if ( !((ULONG)mp1 & TF_MOVE) || (((ULONG)mp1 & TF_MOVE) == TF_MOVE) )
				break;
			track.fs = (ULONG)mp1 | TF_GRID; /*| TF_ALLINBOUNDARY;*/
			WinQueryWindowPos( hwnd, &swp );
			track.cxBorder = WinQuerySysValue( HWND_DESKTOP, SV_CXSIZEBORDER );
			track.cyBorder = WinQuerySysValue( HWND_DESKTOP, SV_CYSIZEBORDER );
			track.cxGrid = track.cyGrid =
			track.cxKeyboard = track.cyKeyboard = ProgOptions.CellSize;
			track.rclTrack.xLeft = swp.x;
			track.rclTrack.yBottom = swp.y;
			track.rclTrack.xRight = swp.x + swp.cx;

			track.rclTrack.yTop = swp.y + swp.cy;
			rect.xLeft =
			rect.yBottom = 0;
			rect.xRight = 
			rect.yTop = ProgOptions.CellSize * MIN_CELLS;

			WinCalcFrameRect( hwnd, &rect, FALSE );
			track.ptlMinTrackSize.x = rect.xRight - rect.xLeft;
			track.ptlMinTrackSize.y = rect.yTop - rect.yBottom;
			track.ptlMaxTrackSize.x = WinQuerySysValue( HWND_DESKTOP, SV_CXSCREEN );
			track.ptlMaxTrackSize.y = WinQuerySysValue( HWND_DESKTOP, SV_CYSCREEN );
			memset( &track.rclBoundary, 0, sizeof( RECTL ) );

			track.rclBoundary.xLeft = 
			track.rclBoundary.yBottom = 0;
			track.rclBoundary.xRight = track.ptlMaxTrackSize.x;
			track.rclBoundary.yTop = track.ptlMaxTrackSize.y;
			switch (track.fs & (TF_LEFT | TF_RIGHT )) {
				case TF_LEFT : iCutOpt = CUT_LEFT; break;
				case TF_RIGHT: iCutOpt = CUT_RIGHT; break;
				default: iCutOpt = 0;
			}
			switch (track.fs & (TF_TOP | TF_BOTTOM )) {
				case TF_TOP   : iCutOpt|= CUT_BOTTOM; break;
				case TF_BOTTOM: iCutOpt|= CUT_TOP; break;
			}
			if ( WinTrackRect( HWND_DESKTOP, NULLHANDLE, &track ) ) {
				WinSendMsg( WinWindowFromID( hwnd, FID_CLIENT ),
					MYM_SETCUTOPT, (MPARAM)iCutOpt, (MPARAM)0 );
				WinSetWindowPos( hwnd, NULLHANDLE,
					track.rclTrack.xLeft, track.rclTrack.yBottom,
					track.rclTrack.xRight - track.rclTrack.xLeft,
					track.rclTrack.yTop - track.rclTrack.yBottom,
					SWP_MOVE | SWP_SIZE );
			}
			return (MRESULT)0;
		} // end case WM_TRACKFRAME
	} // end switch msg
	return DefMainFrameProc( hwnd, msg, mp1, mp2 );
}

/*************************************************************************\
 * FUNCTION main
 * FÅhre einige Initialisierung durch und gehe dann in die
 * Message Schleife.
\*************************************************************************/
int main( int argc, char *argv[] )
{
	CHAR pszClientClass[64] = "Life.Child";
	HMQ hmq = NULLHANDLE;		// message-queue of the program
	QMSG qmsg;
	ULONG fWndCtrlData;			// window style for the created win
	BOOL fResult; 					// return value of various functions

	if ( (ProgInfo.hab = WinInitialize(0)) == NULLHANDLE ) {
		CHAR pszMsg[MAX_RES_MSGLENGTH];
		LoadMessage( IDMSG_WININITIALIZE, pszMsg );
		fputs( pszMsg, stderr );
		DosBeep( 440, 100 );
		return 1;
	}
	if ( !(hmq = WinCreateMsgQueue( ProgInfo.hab, 0 )) ) {
		CHAR pszMsg[MAX_RES_MSGLENGTH];

		LoadMessage( IDMSG_WINCREATEMSGQUEUE, pszMsg );
		fputs( pszMsg, stderr );
		DosBeep( 440, 100 );
		WinTerminate( ProgInfo.hab );
		return 1;
	}

	// Set up PROGINFO structure and retrieve settings from life.ini.
	if ( !InitProgInfo( argc, argv, "LifePM" ) ) {
		WinDestroyMsgQueue( hmq );
		WinTerminate( ProgInfo.hab );
		return 1;
	}
	fResult = 
	WinRegisterClass(
		ProgInfo.hab,
		pszClientClass,
		(PFNWP)MainClientProc,
		CS_SIZEREDRAW,
		0 );

	if ( !fResult ) {
		CHAR pszMsg[MAX_RES_MSGLENGTH];
		LoadMessage( IDMSG_WINREGISTERCLASS, pszMsg );
		
		fputs( pszMsg, stderr );
		DosBeep( 440, 100 );
		WinDestroyMsgQueue( hmq );
		WinTerminate( ProgInfo.hab );
		return 1;
	}

	fWndCtrlData = FCF_MINBUTTON		| FCF_MAXBUTTON	|
						FCF_ACCELTABLE		| FCF_TASKLIST		|
						FCF_SYSMENU 		|
						FCF_TITLEBAR 		| FCF_SIZEBORDER	|
						FCF_MENU				| FCF_ICON			|
						FCF_AUTOICON;
	// Create the programs main window
	ProgInfo.hwndMain =
	WinCreateStdWindow(
		HWND_DESKTOP, 	 			// Parent-window
		WS_ANIMATE,					// Noch nicht sichtbar
		&fWndCtrlData,				// Window parameters, s.o.
		pszClientClass,			// Class of the client window
		NULL,							// Titlebar text
		0,								// Client-window style
		NULLHANDLE,	 				// Resourcen sind in der .exe Datei
		IDR_MAIN,					// Frame-window identifier
		NULL );						// Output: Client-window handle

	if ( !ProgInfo.hwndMain ) {
		CHAR pszMsg[MAX_RES_MSGLENGTH];

		LoadMessage( IDMSG_WINCREATESTDWINDOW, pszMsg );
		ErrMsgPM( ProgInfo.hab, pszMsg );
		WinDestroyMsgQueue( hmq );
		WinTerminate( ProgInfo.hab );
		return 1;
	}
	// We have to subclass the frame-window procedure to be
	// aware of resizing.
	DefMainFrameProc =
	WinSubclassWindow( ProgInfo.hwndMain, (PFNWP)MainFrameProc );

	// Initialize online help
	ProgInfo.hwndHelp =
	InitHelp(
		ProgInfo.hab,
		HELP_TABLE,
		IDSTR_HELPWINDOWTITLE,
		ProgInfo.pszHelpName );
	// Make window visible.
	WinShowWindow( ProgInfo.hwndMain, TRUE );
	
	while( WinGetMsg( ProgInfo.hab, (PQMSG)&qmsg, NULLHANDLE, 0L, 0L ))
		 WinDispatchMsg( ProgInfo.hab, (PQMSG)&qmsg );

	// clean up
	if ( ProgInfo.hwndMain ) WinDestroyWindow( ProgInfo.hwndMain );
	if ( hmq ) WinDestroyMsgQueue( hmq );
	if ( ProgInfo.hab ) WinTerminate( ProgInfo.hab );
	return 0;
} // end of main

