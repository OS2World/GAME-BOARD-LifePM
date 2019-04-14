/*************************************************************************\
 * life.h
 * (C) 94   Ralf Seidel
 *          WÅlfrather Str. 45
 *          42105 Wuppertal
 *          email: seidel3@wrcs1.uni-wuppertal.de
\*************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

enum {
	CUT_LEFT 	= 1,
	CUT_RIGHT	= 2,
	CUT_TOP		= 4,
	CUT_BOTTOM	= 8
};

typedef unsigned char TCell;

typedef TCell *PCell;

typedef enum { FINIT, TORUS } TTopology;

typedef enum {
	RECURSIV  = 'r',
	SEQUENTIAL = 's'
} TAlgorithm;

typedef void (*TCallbackFunc)( int, int, int );

extern PCell *planet;

int InitPlanet( const int r, const int c, const TCallbackFunc );
int ResizePlanet( const int r, const int c, const int cut_opt );
int PlanetRows();
int PlanetCols();
void ClearPlanet( void );
void SetCell( const int r, const int c, const int state );
void CalcGen( TAlgorithm alg, TTopology togology );
#ifdef __cplusplus
}
#endif

