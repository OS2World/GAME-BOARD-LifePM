/*************************************************************************\
 * threads.h
 * This file defines a structure for thread handling. For a description see
 * EDM/2 Nov. 1993 (Volume 1, Issue 6) : Threads in PM-Applications by
 * Larry Salomon, Jr.
 *
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

typedef struct _THREADINFO {
	/**********************************************************************\
	 * Initialized by the main thread
	\**********************************************************************/
	ULONG ulSize;		// This specifies the size of the structure.
	ULONG ulType;		// The thread type.
	HWND hwndOwner;	// This specifies the handle of the owning window.
	BOOL bKill;			// Set to TRUE when the thread is to be aborted.
	TID tid;				// Thread id (returned by _beginthread).
	/**********************************************************************\
	 * Initialized inside the thread
	\**********************************************************************/
	HAB hab;				// Anchor block of the thread
	HMQ hmq;				// Message queu of the thread
	BOOL bDead;			// This is set to TRUE by the thread when it is dead.
	ULONG ulResult;	// Result indicator of the completed task.
} THREADINFO, *PTHREADINFO;
 
VOID InitThreadInfo( PTHREADINFO pti, ULONG ulSize, ULONG ulType, HWND hwnd );

VOID InitThread( PTHREADINFO pti );

VOID DoneThread( PTHREADINFO pti, ULONG ulResult );

// Definition von BeginThread, um kompatibel mit Borland C++ zu sein.
#ifdef __BCC__
#define BeginThread( pfn, parms ) \
	_beginthread( (pfn), 0x10000, (PVOID)(parms) )
#else
#define BeginThread( pfn, parms ) \
	_beginthread( (pfn), NULL, 0x10000, (PVOID)(parms) )
#endif
	
#ifdef __cplusplus
}
#endif

