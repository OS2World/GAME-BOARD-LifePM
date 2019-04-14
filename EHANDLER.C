/***********************************************************************\
 * ehandler.c
 * (C) 94   Ralf Seidel
 *          Wlfrather Str. 45
 *          42105 Wuppertal
 *          email: seidel3@wrcs1.uni-wuppertal.de
 *
 * Eventhandler routines for mouse and keyboard inputs.
 * These handler are placed in a seperate file since it requieres some
 * additional compiler options to use them. Both functions call evPut
 * to place new events in the event queue, so this function also needs
 * the additional compiler switches and for this reason it is define
 * here.
 *
 * Set tabs to 3 to get a readable source.
\***********************************************************************/
#include "events.h"

#pragma off (check_stack)

extern volatile void (__interrupt __far *_OldKbdInt)();
extern volatile unsigned evGetPos;
extern volatile unsigned evPutPos;
extern volatile TEvent events[EVENT_QUEUE_SIZE];

static int Full() { return (((evPutPos + 1) % EVENT_QUEUE_SIZE ) == (evGetPos % EVENT_QUEUE_SIZE )); }

int evPut( const PEvent pevent )
/***********************************************************************\
 * Fgt einen neuen Event in die Event Queue ein.
 * Rckgabewerte:
 * 0 : Queue ist voll
 * 1 : Aufruf war erfolgreich
 * The function is defined here because it is called by the event
 * handler and therefore also needs some additional compiler switches.
\***********************************************************************/
{
	_disable();
	if ( Full() ) return 0;
	events[evPutPos % EVENT_QUEUE_SIZE] = *pevent;
	evPutPos++;
	_enable();
	return 1;
}

/***********************************************************************\
 * Keyboard event handler funktion.
\***********************************************************************/

// Use inline assembler code for calling bios interrupts
unsigned short _getkey();

char _kbhit();

#pragma aux _getkey = \
	"mov	ah,0x10" \
	"int	0x16" \
	value [ax];

#pragma aux _kbhit = \
	"mov	ah,0x11" \
	"int	0x16" \
	"mov	al,0" \
	"jz	nokey" \
	"inc	al" \
	"nokey:" \
	value [al];

void __interrupt __far KbdEventHandler( )
{
	static TEvent kev = { KBD_INPUT };

	_OldKbdInt();					// Call original interrupt handler.
	if ( _kbhit() ) {
		kev.kbd.keycode = *(unsigned short*)&kev.kbd.charcode = _getkey();
		// Etwas geschlampt... Ordentlicher waere es so:
		// unsigned short ax;
		// ax = _getkey();
		//	kev.kbd.keycode = ax;
		// kev.kbd.charcode = (unsigned char)(ax & 0x0F);
		// kev.kbd.scancode = (unsigned char)(ax >> 8);
		evPut( &kev );
	}
}

void KEH_end() {}

/***********************************************************************\
 * Mouse event handler.
 *
 * Diese Routine bekommt vom Maustreiber die Parameter in den Registern
 * AX, BX, CX und DX uebergeben.
\***********************************************************************/

void __far __loadds MouseEventHandler(
   unsigned short event,			// AX
	unsigned short btnstate,		// BX
	unsigned short xpos,				// CX
	unsigned short ypos )			// DX
{
// Teile dem Compiler mit, in welchen Registern er die Parameter zu
// erwarten hat.
#ifdef __386__
#pragma aux MouseEventHandler parm [EAX] [EBX] [ECX] [EDX]
#else
#pragma aux MouseEventHandler parm [AX] [BX] [CX] [DX]
#endif
	static TEvent mev;

	mev.mouse.btnstate = btnstate;
	mev.mouse.xpos = xpos;
	mev.mouse.ypos = ypos;
	switch ( event ) {
		case 0x01: mev.id = MOUSE_MOVE;     break;
		case 0x02: mev.id = MOUSE_BTN1DOWN; break;
		case 0x04: mev.id = MOUSE_BTN1UP;   break;
		case 0x08: mev.id = MOUSE_BTN2DOWN; break;
		case 0x10: mev.id = MOUSE_BTN2UP;   break;
		case 0x20: mev.id = MOUSE_BTN3DOWN; break;
		case 0x40: mev.id = MOUSE_BTN3UP;   break;
		default  : // Ignoriere unbekannte Mausevents.
			return; 
		} // switch ( event )
	evPut( &mev );
	return;
}

// Dummy Funktion, zur Berechung der MouseEventHandler L„nge.
void MEH_end() {}


