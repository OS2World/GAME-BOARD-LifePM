/*************************************************************************\
 * profile.h
 * (C) 94   Ralf Seidel
 *          WÅlfrather Str. 45
 *          42105 Wuppertal
 *          email: seidel3@wrcs1.uni-wuppertal.de
 *
 * Set tabs to 3 to get a readable source code.
\*************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

BOOL StoreWindowPos(
	const HINI hini, const PCSZ pszApp,
	const PCSZ pszKey, const HWND hwnd );

BOOL RestoreWindowPos(
	const HINI hini, const PCSZ pszApp,
	const PCSZ pszKey, const HWND hwnd );

#ifdef __cplusplus
}
#endif

