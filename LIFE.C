/*************************************************************************\
 * life.c
 * (C) 94   Ralf Seidel
 *          WÅlfrather Str. 45
 *          42105 Wuppertal
 *          email: seidel3@wrcs1.uni-wuppertal.de
\*************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "life.h"

#ifndef __GNUC__
#define __inline__
#endif

PCell *planet;
static int rows = 0;
static int cols = 0;
static TCallbackFunc ScrUpdateFunc;
static PCell *nextplanet;


static void _DummyFunc( int r, int c, int s )
// Falls keine Updaten der Bildschirmanzeige erfolgen soll benutzte
// diese Funktion
{
	r = r; c = c; s = s; // Vermeide 'Unreferenced variables' Warnungen
}

int InitPlanet( const int r, const int c, const TCallbackFunc callback )
{
	int i;
	
   rows = r; cols = c;
   if ( callback )  ScrUpdateFunc = callback;
   else ScrUpdateFunc = _DummyFunc;
   // Belege Speicherplatz der Groesse (r+2) * (c+2)
   planet = (PCell*)malloc( (rows+2) * sizeof( PCell) );
   if ( !planet ) return 0;
   nextplanet = (PCell*)malloc( (rows+2) * sizeof( PCell) );
   if ( !nextplanet ) return 0;
   for ( i = 0; i < rows + 2; i++ ) {
      planet[i] = (PCell)calloc( (cols+2), sizeof( TCell ) );
      if ( !planet[i] ) return 0;
      nextplanet[i] = (PCell)malloc( (cols+2) * sizeof( TCell ) );
      if ( !nextplanet[i] ) return 0;
   }
   return 1;
}

/*************************************************************************\
 * FUNCTION ResizePlanet
 * Diese Funktion erlaubt es die Grî·e des Planeten nachtrÑglich zu
 * Ñndern. Sie gibt 1 zurÅck falls das gelungen ist und 0 sonst
 * (Speicherplatzmangel). Sollte der RÅckgabewert 0 sein, so mu· das
 * Programm sofort beendet werden, da die Zeiger planet und nextplanet
 * nicht mehr auf gÅltige Adressen verweisen.
 * Parameter:
 * newrows:
 *		Anzahl der neuen Zeilenanzahl
 * newcols:
 *		Anzahl der neuen Spaltenanzahl
 * cut_opt:
 *		Gibt an wo Zellen weggeschnitten bzw. hinzugefÅgt werden sollen.
 *		Dieser Parameter setzt sich zusammen (OR-VerknÅpfung) aus den
 *		folgenden Werten:
 *		CUT_TOP:
 *			Neue Zeilenanzahl kleiner als alte --> lîsche obere Zeilen
 *			Neue Zeilenanzahl grî·er als alte  --> fÅge oben Zeilen an
 *		CUT_BOTTOM:
 *			Neue Zeilenanzahl kleiner als alte --> lîsche untere Zeilen
 * 		Neue Zeilenanzahl grî·er als alte  --> fÅge unten Zeilen an
 *		CUT_TOP | CUT_BOTTOM:
 *			Neue Zeilenanzahl kleiner als alte --> lîsche oben und unten Zeilen
 * 		Neue Zeilenanzahl grî·er als alte  --> fÅge oben und unten Zeilen an
 *		CUT_LEFT, CUT_RIGHT, CUT_LEFT | CUT_RIGHT:
 *			entsprechend 
 * Der neue Planet hat nach dem Aufruf die Grî·e newrows x newcols
 * (Zeilen x Spalten).
 * Bemerkung: Es wird davon ausgegangen, da· die Zeile 1 die oberste Zeile
 * darstellt, d.h. in der OS/2 Applikation mÅssen die Werte CUT_TOP und
 * CUT_BOTTOM vertauscht werden.
\*************************************************************************/
int ResizePlanet( const int newrows, const int newcols, const int cut_opt )
{
	PCell *oldplanet;
	int oldrows, oldcols, rstart, rnum, cstart, cnum;
	int i;

	// Falls sich nicht geÑndert hat verlasse diese Funktion sofort.
	if ( (rows == newrows) && (cols == newcols) ) return 1;
	// Speichere den aktuellen Planeten
	oldplanet = planet;
	oldrows = rows;
	oldcols = cols;
	// Lîsche die Kopie des Planeten, die fÅr die Berechnung neuer
	// Generationen benutzt wird.
	for ( i = 0; i < rows + 2; i++ ) {
		free( nextplanet[i] );
	}
	free( nextplanet );
	planet = nextplanet = NULL;
	// Initialisiere einen neuen Planeten.
	if ( !InitPlanet( newrows, newcols, ScrUpdateFunc ) ) return 0;
	// Bestimme die Anzahl der zu kopierenden Zeilen und Spalten.
	// (Minimum der alten und neuen Zeilen/Spalten Anzahl)
	rnum = newrows <= oldrows ? newrows : oldrows;
	cnum = newcols <= oldcols ? newcols : oldcols;
	// Bestimme die erste zu kopierende Zeile.
	switch ( cut_opt & (CUT_TOP | CUT_BOTTOM) ) {
		case CUT_TOP: 			// Fange weiter unten an zu kopieren
			if ( oldrows < newrows )
				rstart = newrows - rnum;
			else
				rstart = oldrows - rnum;
			break;
		case CUT_BOTTOM: 		// Kopiere von oben nach unten
			rstart = 0;
			break;
		default:					// Kopiere sie aus dem Zentrum.
			rstart = abs(oldrows - newrows) / 2;
	}
	// Nun das gleiche fÅr die Spalten...
	// Komisch... hier wird jeweils eine 1 dazuaddiert.
	switch ( cut_opt & (CUT_LEFT | CUT_RIGHT) ) {
		case CUT_LEFT:
			if ( oldcols < newcols )
				cstart = newcols - cnum + 1;
			else
				cstart = oldcols - cnum + 1;
			break;
		case CUT_RIGHT:
			cstart = 1; break;
		default:
			cstart = abs(oldcols - newcols) / 2 + 1;
	}
	// Insgesamt gibt es beim Kopieren vier FÑlle zu unterscheiden.
	// 1. newrows < oldrows & newcols < oldcols
	// 2. newrows < oldrows & newcols Ú oldcols
	// 3. newrows Ú oldrows & newcols < oldcols
	// 3. newrows Ú oldrows & newcols Ú oldcols
	if ( newrows < oldrows ) {
		if ( newcols < oldcols ) {
			// 1. newrows < oldrows & newcols < oldcols
			for ( i = 1; i <= rnum; i++ ) 
				memcpy( &planet[i][1], &oldplanet[i + rstart][cstart], cnum * sizeof( TCell ) );
		} else {
			// 2. newrows < oldrows & newcols Ú oldcols
			for ( i = 1; i <= rnum; i++ ) 
				memcpy( &planet[i][cstart], &oldplanet[i + rstart][1], cnum * sizeof( TCell ) );
		}
	} else {
		if ( newcols < oldcols ) {
			// 3. newrows Ú oldrows & newcols < oldcols
			for ( i = 1; i <= rnum; i++ ) 
				memcpy( &planet[i + rstart][1], &oldplanet[i][cstart], cnum * sizeof( TCell ) );
		} else {
			// 4. newrows Ú oldrows & newcols Ú oldcols
			for ( i = 1; i <= rnum; i++ ) 
				memcpy( &planet[i + rstart][cstart], &oldplanet[i][1], cnum * sizeof( TCell ) );
		}
	}
	// Lîsche den alten Planeten
	for ( i = 0; i < oldrows + 2; i++ ) {
		free( oldplanet[i] );
	}
	return 1;
}	
	
int PlanetRows() { return rows; }

int PlanetCols() { return cols; }

static void _clear( PCell *acells )
{
	int i;
	
   for ( i = 0; i < rows + 2; i++ ) {
      memset( acells[i], 0, (cols+2) * sizeof( TCell ) );
   }
}
   
void ClearPlanet( void )
// Lîscht alle Felder des Planeten.
{
	_clear( planet );
}

void SetCell( const int row, const int col, const int state )
{
	planet[row][col] = state ? 1 : 0;
	ScrUpdateFunc( row, col, state );
}

static __inline__ int Neighbours( const int row, const int col )
// ZÑhle die Nachbarn der Zelle (Zeile = r , Spalte = c).
{
   int count;
   PCell pcell;

   // Links pruefen
   pcell = &planet[row][col-1];
   count = *pcell;
   // rechts pruefen
   pcell+= 2;
   count+= *pcell;
   // In der Zeile drueber zaehlen
   pcell = &planet[row-1][col-1];
   count+= *pcell++;
   count+= *pcell++;
   count+= *pcell;
   // Und noch die Zeile drunter checken
   pcell = &planet[row+1][col-1];
   count+= *pcell++;
   count+= *pcell++;
   count+= *pcell;
	// Falls wir den recursiven Algorithmus benutzten: Lîsche das
	// high nibble, in dem der Bearbeitungsstatus gespeichert wird.
   return count & 0x0f;
}

static void MakeTorus( void )
{
	PCell *pLine;

	for ( pLine = &planet[1]; pLine <= &planet[rows]; pLine++ ) {
		pLine[0][0] = pLine[0][cols];
		pLine[0][cols + 1] = pLine[0][1];
	}
	memcpy( planet[0], planet[rows], cols + 2 );
	memcpy( planet[rows + 1], planet[1], cols + 2 );
	return;
}

/*************************************************************************\
 * Klassischer sequentieller Algorithmus, um die nÑchste Generation zu
 * berechnen.
\*************************************************************************/
static void CalcGenClassic()
{
   PCell *help;
	int r, c;

   // Lîsche den neuen Planeten
   _clear( nextplanet );

   for( r = 1; r <= rows; r++ ) {
      // Schleife durch alle Zeilen der PlanetenoberflÑche.
      for( c = 1; c <= cols; c++ ) {
         // Schleife durch alle Zellen in einer Zeile.
         switch ( Neighbours( r, c ) ) {
            case 2:
            	// Nichts Ñndert sich
               nextplanet[r][c] = planet[r][c];
               break;
            case 3:
            	// Diese Zelle erwacht auf jeden Fall zum Leben.
               nextplanet[r][c] = 1;
            	// éndere ggf. die Bildschirmanzeige:
		         if ( !planet[r][c] ) ScrUpdateFunc( r, c, 1 );
               break;
            default:
	            // In allen anderen FÑllen wird die Zelle gelîscht.
	            // (Das ist durch die Initialisierung bereits geschehen.)
	            // Wenn die Zelle bislang am Leben war, so lîsche sie
	            // vom Bildschirm.
		         if ( planet[r][c] ) ScrUpdateFunc( r, c, 0 );
         }
      } // for c
   } // for r
   // Tausche die Bedeutung von planet und nextplanet
   help = planet; planet = nextplanet; nextplanet = help;
}

/*************************************************************************\
 * Recursiver Algorithmus zur Berechnung der nÑchsten Generation.
 * Der Algorithmus mu· zwischen einem Torus und einer endlichen FlÑche
 * unterscheiden. Es sind daher einige Funktionen mit minimalen Unter-
 * schieden doppelt.
\*************************************************************************/
static void ProcessCellF( const int r, const int c );
static void ProcessCellT( const int r, const int c );

static __inline__ void CheckCellF( const int r, const int c )
// Diese Funktion ÅberprÅft ob eine Zelle lebendig oder tod ist.
// Im letzteren Fall wird der neue Status berechnet. Ansonsten
// geht die Recursion weiter falls die Zelle noch nicht bearbeited
// wurde.
{
	PCell pcell = &planet[r][c];
	int state = *pcell;

   *pcell|= 0x10;               // Markiere Zelle als ÅberprÅft.
   if ( !state ) {
   	// Diese Zelle ist leer --> ZÑhle ihre Nachbarn
      if ( Neighbours( r, c ) == 3 ) {
      	// Sie hat die richtige Anzahl an Nachbarn um ins Leben gerufen
      	// zu werden.
       	nextplanet[r][c] = 1;
        	ScrUpdateFunc( r, c, 1 );			// Mache die énderung sichtbar.
      }
	} else if ( !--state ) {
		// state war = 1 --> Diese Zelle mu· noch bearbeitet werden.
      ProcessCellF( r, c );
   } // end if
}

static __inline__ void CheckCellT( const int r, const int c )
// Wie CheckCellF, ruft aber ProcessCellT anstatt ProcessCellF auf.
{
	PCell pcell = &planet[r][c];
	int state = *pcell;

   *pcell|= 0x10;               // Markiere Zelle als ÅberprÅft.
   if ( !state ) {
   	// Diese Zelle ist leer --> ZÑhle ihre Nachbarn
      if ( Neighbours( r, c ) == 3 ) {
      	// Sie hat die richtige Anzahl an Nachbarn um ins Leben gerufen
      	// zu werden.
       	nextplanet[r][c] = 1;
        	ScrUpdateFunc( r, c, 1 );			// Mache die énderung sichtbar.
      }
	} else if ( !--state ) {
		// state war = 1 --> Diese Zelle mu· noch bearbeitet werden.
      ProcessCellT( r, c );
   } // end if
}

static void ProcessCellF( const int r, const int c )
{
   int n;									// Anzahl der Nachbarn
	
   n = Neighbours( r, c );
   if ( (n == 2) || (n == 3) ) {
		// Zelle bleibt am Leben.
      nextplanet[r][c] = 1;
   } else {
   	// Die Zelle stirbt - lîsche sie vom Bildschirm
   	ScrUpdateFunc( r, c, 0 );
   }

  	// Die FlÑche soll begrenzt sein --> öberprÅfe nicht Åber den
  	// Rand hinaus.
   if ( r > 1 ) CheckCellF( r - 1, c );
   if ( r < rows ) CheckCellF( r + 1, c );
   if ( c > 1 ) {
      CheckCellF( r, c - 1);
      if ( r > 1 ) CheckCellF( r - 1, c - 1 );
      if ( r < rows ) CheckCellF( r + 1, c - 1 );
   }
   if ( c < cols ) {
      CheckCellF( r, r + 1);
      if ( r > 1 ) CheckCellF( r - 1, c + 1 );
      if ( r < rows ) CheckCellF( r + 1, c + 1 );
   }
   return;
}

static void ProcessCellT( const int r, const int c )
{
   int n;									// Anzahl der Nachbarn
	// Unser Planet ist ein Torus -->
	// Koordinaten der benachbarten Zellen:
   const int right = (c < cols) ? c + 1 : 1;
   const int left  = (c > 1)    ? c - 1 : cols;
   const int above = (r < rows) ? r + 1 : 1;
   const int below = (r > 1)    ? r - 1 : rows;

	
   n = Neighbours( r, c );
   if ( (n == 2) || (n == 3) ) {
		// Zelle bleibt am Leben.
      nextplanet[r][c] = 1;
   } else {
   	// Die Zelle stirbt - lîsche sie vom Bildschirm
   	ScrUpdateFunc( r, c, 0 );
   }

   CheckCellT( above, c );
   CheckCellT( below, c );
   CheckCellT( below, left );
   CheckCellT( r, left );
   CheckCellT( above, left );
   CheckCellT( above, right );
   CheckCellT( r, right );
   CheckCellT( below, right );
   return;
}

static void CalcGenRecursiv( TTopology topology )
{
	const cols1 = cols + 1;
   const PCell *p_last_line = &planet[rows];
   PCell *pline;
   PCell *help;

   _clear( nextplanet );

   for ( pline = &planet[1]; pline <= p_last_line; pline++ ) {
      PCell plast;
      PCell pcell;
      
		pcell = &pline[0][1];
		plast = &pline[0][cols1];
      pcell = (PCell)memchr( pcell, 1, plast - pcell );
      while ( pcell ) {
			const int r = (int)(pline - planet);
			const int c = (int)(pcell - pline[0]);

		   *pcell|= 0x10;               // Markiere Zelle als ÅberprÅft.
		   if ( topology == FINIT ) ProcessCellF( r, c );
	      else ProcessCellT( r, c );
         pcell = (PCell)memchr( pcell, 1, plast - pcell );
      } // while
   }
   help = planet; planet = nextplanet; nextplanet = help;
}

void CalcGen( TAlgorithm alg, TTopology topology )
// Berechne die nÑchste Generation mit dem Algorithmus `alg'
{
	// Falls die Welt einen Torus darstellen soll:
	// Initialisiere die Randfelder.
	if ( topology == TORUS ) MakeTorus();

	if ( alg == RECURSIV ) {
		CalcGenRecursiv( topology );
	} else {
		CalcGenClassic();
	}
}

