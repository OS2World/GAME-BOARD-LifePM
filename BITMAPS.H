/*************************************************************************\
 * loadbmp.h
 * (C) 94   Ralf Seidel
 *          WÅlfrather Str. 45
 *          42105 Wuppertal
 *          email: seidel3@wrcs1.uni-wuppertal.de
 *
 * Set tabs to 3 to get a readable source.
\*************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

extern BOOL bPutBitmapsInCache;

HPS CreateMemPS( HAB hab );

BOOL DestroyMemPS( HPS hpsMem );

BOOL _LoadBitmap(
	const PSZ pszFileName, const HMODULE hmod, const ULONG ulResId,
	PBITMAPINFOHEADER2 *ppBmpih2, PBYTE *ppBmpData );

HBITMAP LoadBitmapFile( const HPS hpsMem, const PSZ pszFileName );

BOOL SaveBitmapFile( const HAB hab, const HBITMAP hbmp, const PSZ pszFileName );

#ifdef __cplusplus
}
#endif

