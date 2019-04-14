/***********************************************************************\
 * events.c
 * (C) 94   Ralf Seidel
 *          WÅlfrather Str. 45
 *          42105 Wuppertal
 *          email: seidel3@wrcs1.uni-wuppertal.de
 *
 * Set tabs to 3 to get a readable source.
\***********************************************************************/
#include <stdlib.h>
#include <i86.h>
#include <signal.h>
#include <dos.h>

#include "mouse.h"
#include "events.h"

// Event handler routines defined in ehandler.c
// These functions are placed in a seperate file since they need some
// additional compiler switches.

void __far __loadds MouseEventHandler(
	unsigned short event,
	unsigned short btn_state,
	unsigned short xpos,
	unsigned short ypos );
void MEH_end(); // Dummy function to calculate the length of the event handler

void __interrupt __far KbdEventHandler();
void KEH_end(); // Dumy function...

// The following interrupt pointer isn't static because it is referenced
// by the keyboard event handler in ehandler.c.
unsigned evGetPos = 0;
unsigned evPutPos = 0;
TEvent events[EVENT_QUEUE_SIZE];
void (__interrupt __far *_OldKbdInt)();

#ifdef __386__
/***********************************************************************\
 * Interface fuer DPMI page locking services.
 * DPMI_lock ruft die DPMI interrupts auf um Speicher vor dem auslagern
 * (swapen) zu sichern.
 * Input:
 * address: Anfangsaddresse des zu schuetzenden Bereiches.
 * lenght : Laenge dieses Bereiches.
 * action : 1 = lock; 0 = unlock
 * Output:
 * Das Carry-Flag ist gesetzt falls ein Fehler auftrat. Die Funktion gibt
 * in diesem Fall 0 zurueck und 1 andernfalls.
\***********************************************************************/
static int DPMI_lock( void *address, unsigned long length, int action )
{
	static union REGS regs;
	unsigned long linadr = (unsigned long) address;

	regs.w.ax = action ? 0x600 : 0x0601;			// DPMI Lock/Unlock Linear Region
	regs.w.bx = (unsigned short)(linadr >> 16);	// Addresse in BX:CX
	regs.w.cx = (unsigned short)(linadr & 0xFFFF);
	regs.w.si = (unsigned short)(length >> 16);	// Laenge nach SI:DI
	regs.w.di = (unsigned short)(length & 0xFFFF);
	int386( 0x31, &regs, &regs );
	return (!regs.w.cflag);
}
#else
static int DPMI_lock( void *address, unsigned long length, int action )
{
	address = address; length=length; action=action; return 0;
}
#endif

static int lock_region( void *address, unsigned long length )
{
	return DPMI_lock( address, length, 1 );
}

static int unlock_region( void *address, unsigned long length )
{
	return DPMI_lock( address, length, 0 );
}

int evIsEmpty() { return (evPutPos == evGetPos); }
int evIsFull()  { return (((evPutPos + 1) % EVENT_QUEUE_SIZE ) == (evGetPos % EVENT_QUEUE_SIZE )); }

static const TEvent empty_event = { NO_EVENT };

/***********************************************************************\
 * Gibt das nÑchste Ereigniss aus der Event Queue zurÅck, lîscht es
 * aber dort NICHT.
 * Wenn kein Event vorliegt wird ein Ereigniss des Types NO_EVENT
 * zurÅckgegeben.
\***********************************************************************/
TEvent evPeek()
{
	if ( evIsEmpty() ) {
		return empty_event;
	} else {
		return events[evGetPos % EVENT_QUEUE_SIZE];
	}
}

/***********************************************************************\
 * Holt einen Event aus der Warteschlange und gibt ihn zurÅck.
 * Wenn kein Event vorliegt und wait = 0 ist wird eine Event des
 * Types NO_EVENT zurÅckgegeben.
\***********************************************************************/
TEvent evGet( int wait )
{
	if ( !wait && evIsEmpty() ) {
		return empty_event;
	} else {
		PEvent ev;
		while( evIsEmpty() ) ;
		_disable();
		ev = &events[evGetPos % EVENT_QUEUE_SIZE];
		evGetPos++;
		_enable();
		return *ev;
	}
}

static void sigHandler( int sigNumber )
{
	evDone();
	signal( sigNumber, SIG_DFL );
	raise( sigNumber );
}

/***********************************************************************\
 * Initialize the event handler.
 * Set handler routines and add evDone to the exit list (see atexit).
 * To be sure that at program abortion the event handler are deinstall
 * signal.
\***********************************************************************/
int evInit()
{
	unsigned long handler_length;

	// Bestimme die Addresse des Tastatur Interrupts und speichere sie
	// in _OldKbdInt
	_OldKbdInt = _dos_getvect( 0x09 );
	// Installiere einen handler fÅr den Fall, da· das Programm nicht
	// standardmÑ·ig beendet wird.
	signal( SIGABRT, sigHandler );
	signal( SIGSEGV, sigHandler );
	// Schuetze den keyboard handler gegen swap versuche.
	handler_length = (char near*)KEH_end - (char near*)KbdEventHandler;
	lock_region( (void *)KbdEventHandler, handler_length );
	_dos_setvect( 0x09, KbdEventHandler );

	// Ueberpruefe ob eine Maus installiert ist und binde sie ggf. in
	// die Event-Queue mit ein.
	mouseReset();
	if ( mouseAvail ) {
		void __far *pfar;

		pfar = MK_FP( FP_SEG(MouseEventHandler), FP_OFF(MouseEventHandler) );
		handler_length = (char near*)MEH_end - (char near*)MouseEventHandler;
		lock_region( (void *)MouseEventHandler, handler_length );
		mouseInstHandler( 0x00FF, pfar );
	} // end if mouseAvail
	// Benutzte atexit um sicher zu gehen, da· die Maus nach Programmende
	// wieder die default handler Routine aufruft.
	atexit( evDone );
	return 1;
}


/***********************************************************************\
 * Deinstall event handler.
\***********************************************************************/
void evDone()
{
	unsigned long handler_length;
	
	if ( _OldKbdInt ) {
		_dos_setvect( 0x09, _OldKbdInt ); // Install original interrupt handler.
		handler_length = (char near*)KEH_end - (char near*)KbdEventHandler;
		unlock_region( (void *)KbdEventHandler, handler_length );
		_OldKbdInt = NULL;
	}
	mouseReset();
	if ( mouseAvail ) {
		handler_length = (char near*)MEH_end - (char near*)MouseEventHandler;
		unlock_region( (void *)MouseEventHandler, handler_length );
	}
	return;
}


/***********************************************************************\
 * Testroutinen
\***********************************************************************/
#ifdef TESTEVENTS

#include <conio.h>
#include <stdio.h>
#include "events.h"

int main( )
{
	evInit();	
	if ( !mouseAvail ) {
		fprintf( stderr, "No mouse was found!\n" );
		return 1;
	}
	printf( "Press both mouse buttons or <Esc> to quit program.\n" );
	mouseShow();
	while ( 1 ) {
		TEvent ev;

		ev = evGet( 1 );
		if ( (ev.id >= MOUSE_MOVE) && (ev.id <= MOUSE_BTN3UP) ) {
			printf( "Event: %X Buttons: %X Position: (%d, %d)\n",
				ev.id, ev.mouse.btn_state, ev.mouse.xpos, ev.mouse.ypos );
			if ( (ev.mouse.btn_state & 3) == 3 ) break;
		} else if ( ev.id == KBD_INPUT ) {
			if (ev.kbd.keycode == kbEsc ) {
				break;
			} else {
				if ( ev.kbd.scancode != 0 ) {
					putch( ev.kbd.keycode );
				}
			}
		}
	} // while
	mouseHide();
	evDone();
	return 0;
}
#endif

