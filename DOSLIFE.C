/*************************************************************************\
 * doslife.c
 * (C) 94   Ralf Seidel
 *          WÅlfrather Str. 45
 *          42105 Wuppertal
 *          email: seidel3@wrcs1.uni-wuppertal.de
 *
 * This program is designed to be compiled with Watcom C only.
 *
 * Set tabs to 3 to get a readable source.
\*************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dos.h>
#include <graph.h>
#include <time.h>
#include <conio.h>
#include "events.h"
#include "life.h"

typedef unsigned short ushort;

static ushort scr_rows;
static ushort scr_columns;

static void _outchar( const int row, const int col, const char c )
{
#ifdef __386__
	*(char*)(0xB8000 + (row - 1) * 160 + (col - 1) * 2) = c;
#else
	*(char __far *)MK_FP( 0xb800, (row - 1) * 160 + (col - 1) * 2 ) = c;
#endif
}

static void CallbackFunc( const int row, const int col, const int newstate )
{
	_outchar( row, col, (newstate ? '*' : ' ') );
}
	
void Displayplanet(void)
{
	int row, col;

	for ( row = 1; row <= scr_rows; row++ ) {
		_settextposition( row, 1 );
		for ( col = 1; col <= scr_columns; col++ ) {
			char c = planet[row][col] ? '*' : ' ';
			_outmem( &c, 1 );
		}
	}
}

static void printmsg( char *msg )
{
	int n;
	char spc[81];

	n = strlen( msg );
	_settextposition( scr_rows, 1 );
	_outtext( msg );
	memset( spc, ' ', scr_columns - n - 1);
	spc[scr_columns - n - 1] = 0;
	_outtext( spc );
}

static void printtime( char alg, int gen, clock_t time )
{
	char buffer[80];
	sprintf( buffer, "%c - frames per sec: %lu",
		alg, (long)gen * CLOCKS_PER_SEC / time );
	printmsg( buffer );
}
	

int main()
{
	int quit = 0;
	int crow, ccol;
	long generation = 0;
	TAlgorithm alg = SEQUENTIAL;
	TTopology topology;
	time_t runtime = 0;
	time_t starttime;
	time_t tdif;

	scr_rows = _settextrows( _MAXTEXTROWS );
	crow = scr_rows / 2;
	scr_columns = 80;
	ccol = scr_columns / 2;
	_setbkcolor( _WHITE );
	_settextcolor( _BLACK );
	_clearscreen( _GCLEARSCREEN );

	if ( !InitPlanet( scr_rows - 1, scr_columns, CallbackFunc ) ) {
		fprintf( stderr, "Programmfehler:\n" );
		fprintf( stderr, "Kann den planeten nicht initialisieren!\n" );
		exit( 1 );
	}
	if ( !evInit() ) {
		fprintf( stderr, "Programmfehler:\n" );
		fprintf( stderr, "Kann den Eventhandler nicht initialisierten\n" );
	}

	SetCell( 1, 1, 1 );
	SetCell( 1, 3, 1 );
	SetCell( 2, 2, 1 );
	SetCell( 2, 3, 1 );
	SetCell( 3, 2, 1 );

	while( !quit ) {
		struct rccoord curpos;
		TEvent ev;

		// Only update cursor position if it has changed.
		curpos = _gettextposition();
		if ( (curpos.row != crow) || (curpos.col != ccol) ) {
			_settextposition( crow, ccol );
		}
		// Wait for the next event:
		ev = evGet( 1 );
		
		switch ( ev.id ) {
			case KBD_INPUT:
				switch ( ev.kbd.keycode ) {
					case kbUp: 		// Cursor Up
					case kbEUp:		// Cursor Up
						if (!--crow) crow = PlanetRows();
						break;
					case kbDown: 	// Cursor Down
					case kbEDown: 	// Cursor Down
						crow%= PlanetRows();
						crow++;
						break;
					case kbLeft:	// Cursor Left
					case kbELeft:	// Cursor Left
						if (!--ccol) ccol = PlanetCols();
						break;
					case kbRight: // Cursor Right
					case kbERight: // Cursor Right
						ccol%= PlanetCols();
						ccol++;
						break;
					case kbEsc:		// Esc 
						quit = 1;
						break;
					case kbTab:		// Tab - next generation
						_displaycursor( _GCURSOROFF );
						CalcGen( alg, topology );
						_displaycursor( _GCURSORON );
						break;
					case kbEnter:
						// Enter - LÑ·t den planeten leben und von
						// Zeit zu Zeit die Zeit pro Durchlauf
						// ausgegeben.
						_displaycursor( _GCURSOROFF );
						starttime = clock();
						while ( ( ev = evGet(0), ev.id ) != KBD_INPUT ) {
							CalcGen( alg, topology );
							generation++;
							tdif = clock() - starttime;
							if ( tdif > 50 ) {
								runtime+= clock() - starttime;
								printtime( alg, generation, runtime );
								starttime = clock();
							}
						}
						_displaycursor( _GCURSORON );
						break;
					default:
						if ( ev.kbd.scancode == 0 ) break;
						// Normal character input:
						switch ( ev.kbd.charcode ) {
							case ' ':	// Space - invert cell at cursorpositon
								SetCell( crow, ccol, planet[crow][ccol] ^ 1 );
								break;
							case 'c':	// 'c' for classical
							case 'C':
							case 's':	// or 's' for sequential
							case 'S':
								alg = SEQUENTIAL; runtime = 0; generation = 0;
								printmsg( "Sequentieller Algorithmus gewÑhlt" );
								break;
							case 'r':
							case 'R':
								alg = RECURSIV; runtime = 0; generation = 0;
								printmsg( "Rekursiver Algorithmus gewÑhlt" );
								break;
							case 't':
							case 'T':
								printmsg( "Stellen jetzt eine Torus dar" );
								topology = TORUS;
								break;
							case 'p':
							case 'P':
								printmsg( "Die Welt ist ein Scheibe!" );
								topology = FINIT;
								break;
						} // end switch ev.kbd.charcode
				} // end switch ev.kbd.keycode
				break; // end case ev.id == KBD_INPUT
			case MOUSE_MOVE:
 				crow = ev.mouse.ypos / 8 + 1;
				ccol = ev.mouse.xpos / 8 + 1;
				if ( (ev.mouse.btnstate & 3) == 3 ) break;
				if ( ev.mouse.btnstate & 1 ) SetCell( crow, ccol, 1 );
				if ( ev.mouse.btnstate & 2 ) SetCell( crow, ccol, 0 );
				break; // end case ev.id == MOUSE_MOVE
			case MOUSE_BTN1DOWN:
				SetCell( crow, ccol, 1 );
				break; // end case ev.id == MOUSE_BTN1DOWN
			case MOUSE_BTN2DOWN:
				SetCell( crow, ccol, 0 );
				break; // end case ev.id == MOUSE_BTN2DOWN
		}
	} // end while !quit
	_setvideomode( _DEFAULTMODE );
	return 0;
}

