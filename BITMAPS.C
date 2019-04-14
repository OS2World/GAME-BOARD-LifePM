/*************************************************************************\
 * loadbmp.c
 * (C) 94   Ralf Seidel
 *          Wlfrather Str. 45
 *          42105 Wuppertal
 *          email: seidel3@wrcs1.uni-wuppertal.de
 *
 * Set tabs to 3 to get a readable source.
\*************************************************************************/
#define INCL_DEV
#define INCL_DOSERRORS
#define INCL_DOSFILEMGR
#define INCL_DOSMEMMGR
#define INCL_DOSMODULEMGR
#define INCL_DOSRESOURCES
#define INCL_GPIBITMAPS
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#ifdef DEBUG
#include <stdio.h>
#endif
#include "bitmaps.h"

/*************************************************************************\
 * FUNCTION CreateMemPS
 * Creates a presentation space associated with a memory device context.
 * Returns a presentation space handle in case of success and NULLHANDLE
 * otherwise.
\*************************************************************************/
HPS CreateMemPS( HAB hab )
{
	HDC hdcMem;
	HPS hpsMem;
	const ULONG flOptions = PU_PELS | GPIF_DEFAULT | GPIT_MICRO | GPIA_ASSOC;
	SIZEL sizeHps = { 0, 0 };
	
	hdcMem = DevOpenDC( hab, OD_MEMORY, "*", 0, NULL, NULLHANDLE );
	if ( !hdcMem ) return NULLHANDLE;

	hpsMem = GpiCreatePS( hab, hdcMem, &sizeHps, flOptions );
	if ( !hpsMem ) DevCloseDC(hdcMem);

	return hpsMem;
}

/*************************************************************************\
 * FUNCTION DestroyMemPS
 * Destorys a presentation space associated with a memory device context.
 * The device context is also destroyed.
 * This function checks if the paramters hpsMem is really associated with
 * a memory device.
 * Returns TRUE in case of success and FALSE otherwise.
\*************************************************************************/
BOOL DestroyMemPS( HPS hpsMem )
{
	HDC hdc;
	LONG lFamily;

	hdc = GpiQueryDevice( hpsMem );
	if ( !hdc ) return FALSE;
	// Check the device type
	if ( !DevQueryCaps( hdc, 0, 1, &lFamily ) || lFamily != OD_MEMORY )
		return FALSE;
	GpiDestroyPS( hpsMem );
	DevCloseDC( hdc );
	return TRUE;
}

/*************************************************************************\
 * This variable controls wheter bitmap data should be placed into cache
 * when LoadBitmapFile is called. In general this is no good idea since
 * bitmaps are large and therefore may remove more often used data.
\*************************************************************************/
BOOL bPutBitmapsInCache = FALSE;

BOOL _LoadBitmap(
	const PSZ pszFileName,
	const HMODULE hmod, const ULONG ulResId,
	PBITMAPINFOHEADER2 *ppBmpih2, PBYTE *ppBmpData )
/************************************************************************\
 * Load a Bitmap file into memory and return bitmap header and image
 * bytes.
 *
 * Parameters:
 * The first has to be an handle to a presentation space associateted
 * with a memory device, the second parameter is the name of the
 * bitmap file. If pszFileName is NULL or emtpy hmod and ulResId are
 * used to retrieve the bitmap information from a resource.
 * 
 * The last two parameters are used for output.
 * *ppBmpih2 points to the PBITMAPINFO2 structure of the file while
 * *ppBmpData points to to the bitmap data (pixels).
 *
 * Returns:
 * On success the function returns TRUE and FALSE otherwise.
 *
 * Notes:
 * Both return structures are allocated using DosAllocMem!!!
 * The function does not load icons or pointers. If there are several
 * device forms saved in the file only the first (device independet)
 * is taken.
 * Note that there are 2 formats for bitmap files, one of which is
 * archaic.
 * Both formats are supported here. All new bitmaps should follow the
 * format in BITMAPFILEHEADER2.
 *
 * File Layout - size and layout of following map are dependent on the
 * type of bitmap.  The cbFix field of the structures is used to determine
 * the size of the structures which in turn identifies them.
 *
 *
 *         SINGLE BITMAP FILE FORMAT
 *
 * ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿ offset 0
 * ³  Bitmap File Header             (bfh2)  ³ pbfh2->offBits: data Ä¿
 * ³                                         ³ offset from begin of  ³
 * ³                                         ³ file                  ³
 * ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´                       ³
 * ³  Bitmap Information Header      (bmp2)  ³                       ³
 * ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´                       ³
 * ³  Color Table of RGB Structures  (argb2) ³                       ³
 * ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´ <ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
 * ³  Bitmap Data (scan lines)               ³
 * ³             .                           ³
 * ³             .                           ³
 * ³             .                           ³
 * ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
 *
 *
 *       BITMAP-ARRAY FILE FORMAT
 *
 * ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿ offset 0
 * ³  Bitmap Array File Header       (baf2)  ³
 * ³  (only for bitmap arrays)               ³
 * ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
 * ³  Bitmap File Header             (bfh2)  ³ pbfh2->offBits contains data Ä¿
 * ³                                         ³ offset (from begin of file)   ³
 * ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´                               ³
 * ³  Bitmap Information Header      (bmp2)  ³                               ³
 * ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´                               ³
 * ³  Color Table of RGB Structures  (argb2) ³                               ³
 * ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´                               ³
 * ³     next Bitmap Array File Header       ³                               ³
 * ³             .                           ³                               ³
 * ³             .                           ³                               ³
 * ³             .                           ³                               ³
 * ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´ <ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
 * ³  1st Bitmap Data (scan lines)           ³
 * ³                                         ³
 * ³                                         ³
 * ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
 * ³     next Bitmap Data (scan lines)       ³
 * ³             .                           ³
 * ³             .                           ³
 * ³             .                           ³
 * ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
 *
\*************************************************************************/
{
   PBYTE pFileContents;              /* pointer to the loaded (binary)  */
                                     /* file and resource data          */
   PBITMAPFILEHEADER2 pFileHeader2;  /* header of file of resource data */
   PBITMAPINFOHEADER2 pbmpih2;       /* address info header in file     */
  	APIRET rc;                     	 /* return code of API-functions    */
   ULONG ulLength;
   ULONG ulDataLength;

   pFileContents = NULL;
	*ppBmpih2 = NULL;
	*ppBmpData = NULL;

#ifdef DEBUG
	fprintf( stderr, "_LoadBitmap( %s, %lX, %lX, %p, %p )\n",
		pszFileName ? pszFileName : (PSZ)"NULL",
		hmod, ulResId, ppBmpih2, ppBmpData );
	fflush( stderr );
#endif		

	if ( pszFileName && *pszFileName ) {
	   HFILE hfile = NULLHANDLE;      /* Handle to the for the bmpfile.  */
	   FILESTATUS fsts;               /* fileinformation                 */
	   ULONG ulBytesRead;             /* number of bytes read            */
	   ULONG ulActionTaken;           /* What did OS/2 with the file     */
   	                               /* when opening it                 */
	   ULONG ulOpenFlags;
	   
		/* pszFileName is not equal NULL and does not point to an empty string */
		if ( bPutBitmapsInCache ) {
			ulOpenFlags = OPEN_FLAGS_SEQUENTIAL	| OPEN_FLAGS_NOINHERIT 	|
		                 OPEN_SHARE_DENYWRITE 	| OPEN_ACCESS_READONLY;
		} else {
			ulOpenFlags = OPEN_FLAGS_SEQUENTIAL	| OPEN_FLAGS_NOINHERIT 	|
		                 OPEN_SHARE_DENYWRITE 	| OPEN_ACCESS_READONLY	|
							  OPEN_FLAGS_NO_CACHE;
		}
	   rc =
		DosOpen(
			pszFileName,
	      &hfile,
	      &ulActionTaken,
	      0,    					/* FileSize - We don't need this */
			FILE_NORMAL,
			OPEN_ACTION_FAIL_IF_NEW	| OPEN_ACTION_OPEN_IF_EXISTS,
			ulOpenFlags,
			0 );
	   if ( rc != NO_ERROR ) return FALSE;
	   /* For a better performace we load the total file at once. Of course
	    * this gains speed only if we don't handle a bitmap array file with
	    * large bitmaps. However this kind of file is rather rare. */
	   rc =
		DosQueryFileInfo( hfile, 1, &fsts, sizeof(fsts) );
	   if ( rc != NO_ERROR ) {
			DosClose( hfile );
			return FALSE;
		}
		rc =
		DosAllocMem( (PVOID*)&pFileContents, fsts.cbFile, PAG_COMMIT | PAG_READ | PAG_WRITE );
	   if ( rc != NO_ERROR ) {
			DosClose( hfile );
			return FALSE;
		}
	   rc =
		DosRead( hfile, (PVOID)pFileContents, fsts.cbFile, &ulBytesRead );
	   if ( (rc != NO_ERROR) || (ulBytesRead != fsts.cbFile) ) {
	   	DosFreeMem( pFileContents );
			DosClose( hfile );
			return FALSE;
		}
		DosClose( hfile );
		
	} else { /* use hmod and ulResId to retrieve bitmap from resource file */
		PVOID pvResData;
		rc =
		DosQueryResourceSize( hmod, RT_BITMAP, ulResId, &ulLength );
		if ( rc != NO_ERROR ) return FALSE;
		rc =
		DosGetResource( hmod, RT_BITMAP, ulResId, &pvResData );
		if ( rc != NO_ERROR ) return FALSE;
		rc =
		DosAllocMem( (PVOID*)&pFileContents, ulLength, PAG_COMMIT | PAG_READ | PAG_WRITE );
	   if ( rc != NO_ERROR ) {
	   	DosFreeResource( pvResData );
	   	return FALSE;
	   }
	   memcpy( pFileContents, pvResData, ulLength );
   	DosFreeResource( pvResData );
	}

   pFileHeader2 = (PBITMAPFILEHEADER2)pFileContents;
   pbmpih2 = NULL;                /* only set this when we validate type */

   switch ( pFileHeader2->usType )
   {
      case BFT_BITMAPARRAY: /* "BA" */
			/****************************************************************\
          * If this is a Bitmap-Array, adjust pointer to the normal
          * file header. We'll just use the first bitmap in the
          * array and ignore other device forms.
          * Note: This is untested because the lack of appropriate test
          * files.
			\****************************************************************/
         pFileHeader2 = &(((PBITMAPARRAYFILEHEADER2) pFileContents)->bfh2);
				/* pointer to bitmap file header (both versions) */
         pbmpih2 = &pFileHeader2->bmp2;	/* pointer to info header */
         break;
      case BFT_ICON:				/* == "IC" */
      case BFT_POINTER:			/* == "PT" */
      case BFT_BMAP:				/* == "BM" */
         pbmpih2 = &pFileHeader2->bmp2;    /* pointer to info header */
         break;
      /* these formats aren't supported yet; don't set any ptrs */
      case BFT_COLORICON:		/* == "CI" */
      case BFT_COLORPOINTER:	/* == "CP" */
      default:
         goto clean;
   } /* end switch (pFileHeader2->usType) */

	/**********************************************************************\
    * Check to see if BMP file has an old structure, a new structure
    * (incl. Windows structure). Capture the common data and treat all bitmaps
    * generically with pointer to new format.  API's will determine format
    * using cbFixed field.
    *
    * Windows bitmaps have the new format, but with less data fields
    * than PM.  The old strucuture has some different size fields,
    * though the field names are the same.
    *
    *
    * NOTE: bitmap data is located by offsetting the beginning of the file
    *       by the offset contained in pFileHeader2->offBits. This value is in
    *       the same relative location for different format bitmap files.
	\**********************************************************************/

   if ( pbmpih2->cbFix == sizeof( BITMAPINFOHEADER ) ) {           
      /* convert old format to the new one */
	   BITMAPINFOHEADER2 bmpih2;     /* use this variables to convert   */
		PBITMAPINFO2 pbmpi2;
	   PBITMAPINFO pbmpih;				/* the old format to the new       */
	   USHORT usBits;
		INT i;
		
      pbmpih = (PBITMAPINFO)&((PBITMAPFILEHEADER)pFileHeader2)->bmp;
#if 1
		/* PM doesn't need the full bitmap information */
      bmpih2.cbFix = 16;
      bmpih2.cx = (ULONG)pbmpih->cx;
      bmpih2.cy = (ULONG)pbmpih->cy;
      bmpih2.cPlanes = pbmpih->cPlanes; 
      bmpih2.cBitCount = pbmpih->cBitCount;
#else      
      bmpih2.cbFix = sizeof( BITMAPINFOHEADER2 );
      bmpih2.cx = (ULONG)pbmpih->cx;
      bmpih2.cy = (ULONG)pbmpih->cy;
      bmpih2.cPlanes = pbmpih->cPlanes; 
      bmpih2.cBitCount = pbmpih->cBitCount;
      bmpih2.ulCompression = BCA_UNCOMP;
      bmpih2.cbImage = 0;			 	/* Bitmap is uncompress -> this field
      										 * can be zero */
      bmpih2.cxResolution = 640;		/* set to standard VGA Resolution */
      bmpih2.cyResolution = 480;
      bmpih2.cclrUsed = 0;				/* Assume all colors are used */
      bmpih2.cclrImportant = 0;
      bmpih2.usUnits = BRU_METRIC;
      bmpih2.usReserved = 0;
      bmpih2.usRecording = BRA_BOTTOMUP;
      bmpih2.usRendering = BRH_NOTHALFTONED;
      bmpih2.cSize1 = 0;
      bmpih2.cSize2 = 0;
      bmpih2.ulColorEncoding = BCE_RGB;
      bmpih2.ulIdentifier = 0;
#endif      
		usBits = bmpih2.cBitCount * bmpih2.cPlanes;
		ulLength = bmpih2.cbFix;
		/* No palette information for bitmaps with 24 bits per pixel! */
		if ( usBits != 24 ) {
			ulLength+= ((1 << usBits )) * sizeof( RGB2 );
		}

		/* Allocate memory for the returned pointer */
		rc =
		DosAllocMem( (PVOID*)&pbmpi2, ulLength, PAG_READ | PAG_WRITE | PAG_COMMIT );
	   if ( rc != NO_ERROR ) goto clean;
		memcpy( pbmpi2, &bmpih2, bmpih2.cbFix );
		
		/* Copy palette information. */
		if ( usBits != 24 ) {
			RGB *pPalOld = pbmpih->argbColor;
			PRGB2 pPalNew = (PRGB2)( (PBYTE)pbmpi2 + pbmpi2->cbFix );
			for ( i = 0; i < 1 << usBits; i++ ) {
				pPalNew->bBlue = pPalOld->bBlue;
				pPalNew->bGreen = pPalOld->bGreen;
				pPalNew->bRed = pPalOld->bRed;
				pPalNew->fcOptions = 0;
				pPalNew++;
				pPalOld++;
			}
		}
		*ppBmpih2 = (PBITMAPINFOHEADER2)pbmpi2;
		/* Set this variable for copying the pixel data.
		 * If the bitmap is compressed the total size information is hold
		 * in bmpih2.cbImage. */
		if ( bmpih2.cbFix < 24 || bmpih2.ulCompression == BCA_UNCOMP ) {
			ulDataLength = ((( bmpih2.cBitCount * bmpih2.cx ) + 31 ) / 32 )
				* 4 * bmpih2.cy * bmpih2.cPlanes;
		} else {
			ulDataLength = bmpih2.cbImage;
		}
   } else {
		/*******************************************************************\
		 * At this point pbmpih2 points to a valid BITMAPINFOHEADER2 structure.
		 * The pixel data is saved at position
		 * pFileContents + pFileHeader2->offBits.
		\*******************************************************************/
		USHORT usBits = pbmpih2->cBitCount * pbmpih2->cPlanes;
		
		ulLength = pbmpih2->cbFix;
		/* No palette information for bitmaps with 24 bits per pixel! */
		if ( usBits != 24 ) {
			ulLength+= ((1 << usBits )) * sizeof( RGB2 );
		}
		
		rc =
		DosAllocMem( (PVOID*)ppBmpih2, ulLength, PAG_READ | PAG_WRITE | PAG_COMMIT );
	   if ( rc != NO_ERROR ) goto clean;

		memcpy( *ppBmpih2, pbmpih2, ulLength );

		ulDataLength = ((( pbmpih2->cBitCount * pbmpih2->cx ) + 31 ) / 32 )
			* 4 * pbmpih2->cy * pbmpih2->cPlanes;
	}
	rc =
	DosAllocMem( (PVOID*)ppBmpData, ulDataLength, PAG_READ | PAG_WRITE | PAG_COMMIT );
   if ( rc != NO_ERROR ) goto clean;
	memcpy( *ppBmpData, pFileContents + pFileHeader2->offBits, ulDataLength );

	DosFreeMem( pFileContents );
	/* This is the return point if everything was successfull */   
	return TRUE;

	/* Label for cleanup code if any error occured */
clean:
	if ( pFileContents != NULL ) DosFreeMem( pFileContents );
	if ( *ppBmpih2 ) DosFreeMem( *ppBmpih2 );
	if ( *ppBmpData ) DosFreeMem( *ppBmpData );
	return FALSE;
}


HBITMAP LoadBitmapFile( const HPS hpsMem, const PSZ pszFileName )
/************************************************************************\
 * Load a bitmap file into memory and return a handle to it.
 *
 * Parameters:
 * The first has to be an handle to a presentation space associateted
 * with a memory device while the second parameter is the name of the
 * bitmap file.
 *
 * Returns:
 * On success the function returns a handle to the bitmap, otherwise
 * NULLHANDLE is returned.
\*************************************************************************/
{
   PBITMAPINFOHEADER2 pbmpih2; 	/* Address any info headers        */
   PBYTE pBuffer;						/* Holds the bitmap data (pixels)  */
   HBITMAP hbmp;                 /* Handle to bitmap which will
                                  * be returned                     */
#ifdef DEBUG
	fprintf( stderr, "LoadBitmapFile( %lX, %s )\n",
		hpsMem, pszFileName ? pszFileName : (PSZ)"NULL" );
	fflush( stderr );
#endif		
	if ( !_LoadBitmap( pszFileName, NULLHANDLE, 0L, &pbmpih2, &pBuffer ) )
		return NULLHANDLE;
	
   hbmp =
   GpiCreateBitmap(
      hpsMem,							/* Presentation-space handle */
      pbmpih2,             		/* Address of structure for format data */
      CBM_INIT,              		/* Options: create bitmap bits now */
      pBuffer,                	/* Address of buffer of image data */
      (PBITMAPINFO2)pbmpih2);    /* Address of structure for color and format */

   /* release memory allocated by _LoadBitmap */
	DosFreeMem( pbmpih2 );
	DosFreeMem( pBuffer );

   return hbmp;  
}


BOOL SaveBitmapFile( const HAB hab, const HBITMAP hbmp, const PSZ pszFileName )
{
	PBITMAPFILEHEADER2 pbmpfh;
	BITMAPINFOHEADER2 bmpih;
	ULONG cbBufferSize;			// Size of storage to retrieve all bitmap bits.
	ULONG cbInfoSize;				// Size of the infoheader inclusiv palette info.
	ULONG cbColorTable;			// Length of the color table (palette info).
	HPS hpsMem;
	PBYTE pbBuffer;
	PBITMAPINFO2 pbmpInfo;
	CHAR szFileName[CCHMAXPATH];

   HFILE hfile;					// Handle to the for the file.
   ULONG ulBytesWritten;		// Number of bytes written
   ULONG ulActionTaken;			// What did OS/2 with the file when opening it
   ULONG ulOpenFlags;
   APIRET rc;

	// Get some information about the bitmap.
	bmpih.cbFix = 16;
	if ( !GpiQueryBitmapInfoHeader( hbmp, &bmpih ) ) return FALSE;

	// Calculate the memory size needed for all bitmap pixels
	cbBufferSize =
		((( bmpih.cBitCount * bmpih.cx ) + 31 ) / 32 ) * 4  // No. of bytes per scan line
		* bmpih.cPlanes * bmpih.cy;
	// Get the size of the bitmap info header.
	if ( bmpih.cBitCount == 24 ) { 	// No color table for true color bitmaps.
		cbColorTable = 0;
	} else {
		cbColorTable = (1 << (bmpih.cBitCount * bmpih.cPlanes)) * sizeof(RGB2);
	}
	cbInfoSize = sizeof( BITMAPINFOHEADER2 ) + cbColorTable;
	if ( !( pbBuffer = (PBYTE)malloc( cbBufferSize ) ) ) return FALSE;
	pbmpfh = (PBITMAPFILEHEADER2)malloc( sizeof( BITMAPFILEHEADER2 ) + cbColorTable );
	if ( !pbmpfh ) {
		free( pbBuffer );
		return FALSE;
	}

	hpsMem = CreateMemPS( hab );
	if ( !hpsMem ) {
		free( pbBuffer );	free( pbmpfh ); return FALSE;
	}
	if ( GpiSetBitmap( hpsMem, hbmp ) == HBM_ERROR ) {
		DestroyMemPS( hpsMem );
		free( pbBuffer );
		free( pbmpfh );
		return FALSE;
	}
	pbmpInfo = (PBITMAPINFO2)&(pbmpfh->bmp2);
	*(PBITMAPINFOHEADER2)pbmpInfo = bmpih;
	GpiQueryBitmapBits( hpsMem, 0, bmpih.cy, pbBuffer, pbmpInfo );
	DestroyMemPS( hpsMem );
	pbmpfh->usType = BFT_BMAP;	// == "BM"
	pbmpfh->cbSize = 
	pbmpfh->offBits = sizeof( BITMAPFILEHEADER2 ) + cbColorTable; 
	pbmpfh->xHotspot = 0;
	pbmpfh->yHotspot = 0;

	strcpy( szFileName, pszFileName );
	if ( !strchr( szFileName, '.' ) ) {
		strcat( szFileName, ".bmp" );
	}
	ulOpenFlags = OPEN_FLAGS_SEQUENTIAL    | OPEN_FLAGS_NOINHERIT 	|
                 OPEN_SHARE_DENYREADWRITE | OPEN_ACCESS_WRITEONLY;
	if ( !bPutBitmapsInCache ) ulOpenFlags|= OPEN_FLAGS_NO_CACHE;
   rc = DosOpen(
		szFileName,
      &hfile,
      &ulActionTaken,
      pbmpfh->offBits + cbBufferSize,				// FileSize
		FILE_NORMAL,
		OPEN_ACTION_REPLACE_IF_EXISTS | OPEN_ACTION_CREATE_IF_NEW,
		ulOpenFlags,
		0 );
	if ( rc != NO_ERROR ) {
		#ifdef DEBUG
		fprintf( stderr, "DosOpen failed with return code %d\n", rc );
		#endif
		free( pbBuffer ); free( pbmpInfo ); return FALSE;
	}
   rc = DosWrite( hfile, (PVOID)pbmpfh, pbmpfh->cbSize, &ulBytesWritten );
	if ( rc == NO_ERROR ) {
	   rc = DosWrite( hfile, (PVOID)pbBuffer, cbBufferSize, &ulBytesWritten );
	} 
	#ifdef DEBUG
	if ( rc != NO_ERROR ) {
		fprintf( stderr, "DosWrite failed with return code %d\n", rc );
	}
	#endif
	DosClose( hfile );
	free( pbBuffer );
	free( pbmpInfo );
	return (rc == NO_ERROR );
}


