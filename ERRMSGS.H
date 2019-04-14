/*************************************************************************\
 * errmsgs.h
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

VOID _ErrMsg( const char *msg, const char *fname, const unsigned line );

VOID _ErrMsgPM( const HAB hab, const char *msg, const char *fname, const unsigned line );

VOID _ErrMsgKernel(
	const ULONG ulErrCode,
	const char *msg,
	const char *fname,
	const unsigned line );

VOID _ErrMsgRunLib(
	const char *msg,
	const char *fname,
	const unsigned line );

// Some dirty macros: (debugging aids)

#define ErrMsg( msg ) \
	_ErrMsg( (msg), __FILE__, __LINE__ )

#define ErrMsgPM( hab, msg ) \
	_ErrMsgPM( (hab), (msg), __FILE__, __LINE__ )

#define ErrMsgKernel( ulErrCode, msg ) \
	_ErrMsgKernel( (ulErrCode), (msg), __FILE__, __LINE__ )

#define ErrMsgRunLib( msg ) \
	_ErrMsgRunLib( (msg), __FILE__, __LINE__ )

#ifdef assert
#undef assert
#endif

#ifdef NDEBUG
#define assert( exp ) (void)0
#else
void _assert( const char *str, const char *fname, const unsigned line );
#define assert( exp ) ((exp) ? (void)0 : _assert( #exp, __FILE__, __LINE__ ))
#endif

#ifdef __cplusplus
}
#endif

