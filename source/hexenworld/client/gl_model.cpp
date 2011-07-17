// models.c -- model loading and caching

// models are the only shared resource between a client and server running
// on the same machine.

#include "quakedef.h"
#include "glquake.h"

/*
===================
Mod_ClearAll
===================
*/
void Mod_ClearAll (void)
{
	R_FreeModels();
	R_ModelInit();
}
