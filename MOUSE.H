/*************************************************************************\
 * Mouse.h
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

extern int mouseAvail;
extern int mouseButtons;

void mouseReset( void );
void mouseShow( void );
void mouseHide( void );
void mouseState( short *, short *, unsigned short * );
void mouseSetPos( short, short );
void mouseSetColRange( short, short );
void mouseSetRowRange( short, short );
int mouseInstHandler( unsigned short, void __far * );

#ifdef __cplusplus
}
#endif

