/***********************************************************************\
 * events.h
 * (C) 94   Ralf Seidel
 *          WÅlfrather Str. 45
 *          42105 Wuppertal
 *          email: seidel3@wrcs1.uni-wuppertal.de
 *
 * Set tabs to 3 to get a readable source.
\***********************************************************************/
#ifndef _EVENTS_H
#define _EVENTS_H
#include <string.h>
#include <i86.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EVENT_QUEUE_SIZE 128


enum _TEventId {
	NO_EVENT,
	KBD_INPUT,
	MOUSE_MOVE,
	MOUSE_BTN1DOWN,
	MOUSE_BTN1UP,
	MOUSE_BTN2DOWN,
	MOUSE_BTN2UP,
	MOUSE_BTN3DOWN,
	MOUSE_BTN3UP,
	__EVDUMMY = 0xFFFF
};

enum {
	kbEsc       = 0x011B,	kbAltSpace  = 0x0200,	kbCtrlIns   = 0x0400,
	kbShiftIns  = 0x0500,	kbCtrlDel   = 0x0600,	kbShiftDel  = 0x0700,
	kbBack      = 0x0E08,	kbCtrlBack  = 0x0E7F,	kbShiftTab  = 0x0F00,
	kbTab       = 0x0F09,	kbAltQ      = 0x1000,	kbAltW      = 0x1100,
	kbAltE      = 0x1200,	kbAltR      = 0x1300,	kbAltT      = 0x1400,
	kbAltY      = 0x1500,	kbAltU      = 0x1600,	kbAltI      = 0x1700,
	kbAltO      = 0x1800,	kbAltP      = 0x1900,	kbCtrlEnter = 0x1C0A,
	kbEnter     = 0x1C0D,	kbAltA      = 0x1E00,	kbAltS      = 0x1F00,
	kbAltD      = 0x2000,	kbAltF      = 0x2100,	kbAltG      = 0x2200,
	kbAltH      = 0x2300,	kbAltJ      = 0x2400,	kbAltK      = 0x2500,
	kbAltL      = 0x2600,	kbAltZ      = 0x2C00,	kbAltX      = 0x2D00,
	kbAltC      = 0x2E00,	kbAltV      = 0x2F00,	kbAltB      = 0x3000,
	kbAltN      = 0x3100,	kbAltM      = 0x3200,	kbF1        = 0x3B00,
	kbF2        = 0x3C00,	kbF3        = 0x3D00,	kbF4        = 0x3E00,
	kbF5        = 0x3F00,	kbF6        = 0x4000,	kbF7        = 0x4100,
	kbF8        = 0x4200,	kbF9        = 0x4300,	kbF10       = 0x4400,
	kbLeft      = 0x4B00,	kbRight     = 0x4D00,	kbDown      = 0x5000,
	kbUp        = 0x4800,	kbPgUp      = 0x4900,	kbPgDn      = 0x5100,
	kbEnd       = 0x4F00,	kbHome      = 0x4700,	
	kbIns       = 0x5200,	kbDel       = 0x5300,
	kbELeft     = 0x4BE0,	kbERight    = 0x4DE0,	kbEDown     = 0x50E0,
	kbEUp       = 0x48E0,	kbEPgUp     = 0x49E0,	kbEPgDn     = 0x51E0,
	kbEEnd      = 0x4FE0,	kbEHome     = 0x47E0,	
	kbEIns      = 0x52E0,	kbEDel      = 0x53E0,
	

	kbGrayMinus = 0x4A2D,	kbGrayPlus  = 0x4E2B,
	kbShiftF1   = 0x5400,	kbShiftF2   = 0x5500,	kbShiftF3   = 0x5600,
	kbShiftF4   = 0x5700,	kbShiftF5   = 0x5800,	kbShiftF6   = 0x5900,
	kbShiftF7   = 0x5A00,	kbShiftF8   = 0x5B00,	kbShiftF9   = 0x5C00,
	kbShiftF10  = 0x5D00,	kbCtrlF1    = 0x5E00,	kbCtrlF2    = 0x5F00,
	kbCtrlF3    = 0x6000,	kbCtrlF4    = 0x6100,	kbCtrlF5    = 0x6200,
	kbCtrlF6    = 0x6300,	kbCtrlF7    = 0x6400,	kbCtrlF8    = 0x6500,
	kbCtrlF9    = 0x6600,	kbCtrlF10   = 0x6700,	kbAltF1     = 0x6800,
	kbAltF2     = 0x6900,	kbAltF3     = 0x6A00,	kbAltF4     = 0x6B00,
	kbAltF5     = 0x6C00,	kbAltF6     = 0x6D00,	kbAltF7     = 0x6E00,
	kbAltF8     = 0x6F00,	kbAltF9     = 0x7000,	kbAltF10    = 0x7100,
	kbCtrlPrtSc = 0x7200,	kbCtrlLeft  = 0x7300,	kbCtrlRight = 0x7400,
	kbCtrlEnd   = 0x7500,	kbCtrlPgDn  = 0x7600,	kbCtrlHome  = 0x7700,
	kbAlt1      = 0x7800,	kbAlt2      = 0x7900,	kbAlt3      = 0x7A00,
	kbAlt4      = 0x7B00,	kbAlt5      = 0x7C00,	kbAlt6      = 0x7D00,
	kbAlt7      = 0x7E00,	kbAlt8      = 0x7F00,	kbAlt9      = 0x8000,
	kbAlt0      = 0x8100,	kbAltMinus  = 0x8200,	kbAltEqual  = 0x8300,
	kbCtrlPgUp  = 0x8400,	kbAltBack   = 0x0800,	kbNoKey     = 0x0000
};

typedef enum _TEventId TEventId;

struct _TEvent {
	TEventId id;
	union {
		struct {
			unsigned short btnstate;	// Tastenstatus der Maus
			short xpos;						// x-pos der Maus zur Zeit des Events
			short ypos;						// y-pos der Maus zur Zeit des Events
		} mouse;
		struct {
			unsigned short keycode;
			unsigned char charcode;
			unsigned char scancode;
		} kbd;
		struct {
			int d;
		} cmd;
		struct {
			long _1;
			long _2;
		} all;
	};
}; // end struct _TEvent

typedef struct _TEvent TEvent, *PEvent;

int evInit();
void evDone();
int evIsEmpty();
int evIsFull();
TEvent evPeek();
TEvent evGet( int wait );

int evPut( const PEvent pevent );


#ifdef __cplusplus
}
#endif
#endif // _EVENTS_H


