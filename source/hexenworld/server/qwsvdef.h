// quakedef.h -- primary header for server

#include "../../server/server.h"
#include "../../server/quake_hexen/local.h"
#include "../../server/hexen2/local.h"
#include "../../server/progsvm/progsvm.h"

//define	PARANOID			// speed sapping error checking

#include <setjmp.h>

#include "bothdefs.h"

#include "common.h"
#include "sys.h"
#include "zone.h"

#include "net.h"
#include "protocol.h"
#include "cmd.h"

#include "server.h"

//=============================================================================

// the host system specifies the base of the directory tree, the
// command line parms passed to the program, and the amount of memory
// available for the program to use

typedef struct
{
	const char* basedir;
	int argc;
	char** argv;
	void* membase;
	int memsize;
} quakeparms_t;


//=============================================================================

//
// host
//
extern quakeparms_t host_parms;

extern qboolean host_initialized;			// true if into command execution
extern double realtime;					// not bounded in any way, changed at
										// start of every frame, never reset

void SV_Error(const char* error, ...);
void SV_Init(quakeparms_t* parms);

void Con_Printf(const char* fmt, ...);
void Con_DPrintf(const char* fmt, ...);

extern unsigned int defLosses;	// Defenders losses in Siege
extern unsigned int attLosses;	// Attackers Losses in Siege
