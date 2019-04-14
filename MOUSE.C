/***********************************************************************\
 * mouse.c
 * (C) 94   Ralf Seidel
 *          WÅlfrather Str. 45
 *          42105 Wuppertal
 *          email: seidel3@wrcs1.uni-wuppertal.de
 *
 * Set tabs to 3 to get a readable source.
\***********************************************************************/
#include <dos.h>
#include <i86.h>

#include "mouse.h"

#ifdef __386__
static void mintr( union REGS *pregs )
{
	int386( 0x33, pregs, pregs );
}
#else
static void mintr( union REGS *pregs )
{
	int86( 0x33, pregs, pregs );
}
#endif

int mouseAvail = 0;
int mouseButtons = 0;

void mouseReset()
{
	union REGS regs;

	regs.w.ax = 0;
	mintr( &regs );
	mouseAvail = (regs.w.ax == 0xFFFF);
	if ( mouseAvail ) {
		mouseButtons = regs.w.bx;
		mouseInstHandler( 0, 0 );
	} else {
		mouseButtons = 0;
	}
}


/***********************************************************************\
 * Mausfunktion 0x01:
 * Stelle den Mauscursor dar
\***********************************************************************/
void mouseShow()
{
	union REGS regs;

	if ( !mouseAvail ) return;
	regs.w.ax = 1;
	mintr( &regs );
}

/***********************************************************************\
 * Mausfunktion 0x01:
 * Loesche den Mauscursor
\***********************************************************************/
void mouseHide()
{
	union REGS regs;

	if ( !mouseAvail ) return;
	regs.w.ax = 2;
	mintr( &regs );
}

/***********************************************************************\
 * Mausfunktion 0x03:
 * Erfrage derzeite Mausposition und den Status der Maustasten
 * Input:
 * -
 * Output:
 * BX : Tastenstatus als Bitmaske fuer die Tasten
 *      ( Bit n==1 bedeutet Taste n ist gedrueckt )
 * CX : Horizontale Position
 * DX : Vertikale Position
\***********************************************************************/
void mouseState(
	short *posx,
	short *posy,
	unsigned short *btnstate )
{
	union REGS regs;

	if ( !mouseAvail ) {
		*posx = -1;
		*posy = -1;
		*btnstate = 0;
		return;
	}
	regs.w.ax = 3;
	mintr( &regs );
	*btnstate = regs.w.bx;
	*posx = regs.w.cx;
	*posy = regs.w.dx;
}

/***********************************************************************\
 * Mausfunktion 0x04:
 * Setzte Mause auf neue Position
 * Input:
 * CX = x-Position
 * DX = x-Position
 * Output:
 * -
\***********************************************************************/
void mouseSetPos( short posx, short posy )
{
	union REGS regs;

	if ( !mouseAvail ) return;
	regs.w.ax = 4;
	regs.w.cx = posx;
	regs.w.dx = posy;
	mintr( &regs );
}

/***********************************************************************\
 * Mausfunktion 0x07:
 * Festlegen des erlaubten horizontalen Bereiches fuer die Mausposition.
 * Input:
 * CX = minimale erlaubte x-Position
 * DX = maximale erlaubte x-Position
 * Wenn xmin > xmax vertauscht der Maustreiber automatisch deren Werte.
 * Output:
 * -
\***********************************************************************/
void mouseSetColRange( short xmin, short xmax )
{
	union REGS regs;

	if ( !mouseAvail ) return;
	regs.w.ax = 7;
	regs.w.cx = xmin;
	regs.w.dx = xmax;
	mintr( &regs );
}
	

/***********************************************************************\
 * Mausfunktion 0x08:
 * Festlegen des erlaubten vertikalen Bereiches fuer die Mausposition.
 * Input:
 * CX = minimale erlaubte y-Position
 * DX = maximale erlaubte y-Position
 * Wenn ymin > ymax vertauscht der Maustreiber automatisch deren Werte.
 * Output:
 * -
\***********************************************************************/
void mouseSetRowRange( short ymin, short ymax )
{
	union REGS regs;

	if ( !mouseAvail ) return;
	regs.w.ax = 8;
	regs.w.cx = ymin;
	regs.w.dx = ymax;
	mintr( &regs );
}	

/***********************************************************************\
 * Mausfunktion 0x09:
 * Festlegen des Grafik-Cursor-Blocks
 * Input:
 * BX = horizontaler hotspot
 * CX = vertikaler hotspot
 * ES:DS = Zeiger auf Cursor Bitmap
 * Fuer BX und CX sind Werte zwischen -16 und 16 erlaubt.
 * Output:
 * -
\***********************************************************************/


/***********************************************************************\
 * Mausfunktion 0x0A:
 * Festlegen des Text-Cursor-Blocks
 * Input:
 * BX = 0 / 1 : Software / Hardware Cursor
 * CX = Bildschirm Maske
 * DX = Cursor Maske
 * Output:
 * -
\***********************************************************************/
	
/***********************************************************************\
 * Mausfunktion 0x0C:
 * Installiere eine benutzerdefinierte Mausroutine. Es wird eine
 * Maske uebergeben, mit der Mausevents bestimmt werden koennen, die
 * die Prozedure aufrufen lassen.
 * Input:
 * CX = Eventmaske : Bit 0  - Maus wurde bewegt
 *                   Bit 1  - Linke Maustaste wurde gedrueckt
 *                   Bit 2  - Linke Maustaste wurde losgelassen
 *                   Bit 3  - Rechte Maustaste wurde gedrueckt
 *                   Bit 4  - Rechte Maustaste wurde losgelassen
 * Output:
 * -
 * Bemerkungen:
 * Auf einem 386er wird die Handlerfunktion gegen Auslagerungen
 * gesperrt. Das ist wichtig wenn Virtual Memory Managment aktiv ist.
 * Dazu muss dieser Funktion die Laenge des Handlers als dritter
 * Parameter uebergeben werden.
 * Rueckgabewerte der Funktion:
 * 0 : Fehler ist aufgetreten
 * 1 : Alles hat geklappt.
\***********************************************************************/
int mouseInstHandler(
	unsigned short mask,
	void __far *handler )
{
	union REGS regs;
	struct SREGS sregs;

	if ( !mouseAvail ) return 0;
	regs.w.ax = 0x0C;						// Aufruf von Mausfunktion 0x0C
	regs.w.cx = mask;
	sregs.es  = FP_SEG( handler );
#ifdef __386__
	regs.x.edx = FP_OFF( handler );
	int386x( 0x33, &regs, &regs, &sregs );
#else // For 286er Targets: use 16 Bit addresses
	regs.w.dx  = FP_OFF( handler );
	int86x( 0x33, &regs, &regs, &sregs );
#endif // __386__
	return 1;
}

