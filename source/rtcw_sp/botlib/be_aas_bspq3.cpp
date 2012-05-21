/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


/*****************************************************************************
 * name:		be_aas_bspq3.c
 *
 * desc:		BSP, Environment Sampling
 *
 *
 *****************************************************************************/

#include "../game/q_shared.h"
#include "l_memory.h"
#include "../game/botlib.h"
#include "be_interface.h"
#include "../game/be_aas.h"
#include "be_aas_funcs.h"
#include "be_aas_def.h"

//===========================================================================
// traces axial boxes of any size through the world
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bsp_trace_t AAS_Trace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask)
{
	bsp_trace_t bsptrace;
	botimport.Trace(&bsptrace, start, mins, maxs, end, passent, contentmask);
	return bsptrace;
}	//end of the function AAS_Trace
//===========================================================================
// returns the contents at the given point
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_PointContents(vec3_t point)
{
	return botimport.PointContents(point);
}	//end of the function AAS_PointContents
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean AAS_EntityCollision(int entnum,
	vec3_t start, vec3_t boxmins, vec3_t boxmaxs, vec3_t end,
	int contentmask, bsp_trace_t* trace)
{
	bsp_trace_t enttrace;

	botimport.EntityTrace(&enttrace, start, boxmins, boxmaxs, end, entnum, contentmask);
	if (enttrace.fraction < trace->fraction)
	{
		memcpy(trace, &enttrace, sizeof(bsp_trace_t));
		return qtrue;
	}	//end if
	return qfalse;
}	//end of the function AAS_EntityCollision
//===========================================================================
// returns true if in Potentially Hearable Set
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean AAS_inPVS(vec3_t p1, vec3_t p2)
{
	return botimport.inPVS(p1, p2);
}	//end of the function AAS_InPVS
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_BSPModelMinsMaxsOrigin(int modelnum, vec3_t angles, vec3_t mins, vec3_t maxs, vec3_t origin)
{
	BotImport_BSPModelMinsMaxsOrigin(modelnum, angles, mins, maxs, origin);
}	//end of the function AAS_BSPModelMinsMaxs
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_NextBSPEntity(int ent)
{
	ent++;
	if (ent >= 1 && ent < bspworld.numentities)
	{
		return ent;
	}
	return 0;
}	//end of the function AAS_NextBSPEntity
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_BSPEntityInRange(int ent)
{
	if (ent <= 0 || ent >= bspworld.numentities)
	{
		BotImport_Print(PRT_MESSAGE, "bsp entity out of range\n");
		return qfalse;
	}	//end if
	return qtrue;
}	//end of the function AAS_BSPEntityInRange
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_ValueForBSPEpairKey(int ent, const char* key, char* value, int size)
{
	bsp_epair_t* epair;

	value[0] = '\0';
	if (!AAS_BSPEntityInRange(ent))
	{
		return qfalse;
	}
	for (epair = bspworld.entities[ent].epairs; epair; epair = epair->next)
	{
		if (!String::Cmp(epair->key, key))
		{
			String::NCpy(value, epair->value, size - 1);
			value[size - 1] = '\0';
			return qtrue;
		}	//end if
	}	//end for
	return qfalse;
}	//end of the function AAS_FindBSPEpair
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_VectorForBSPEpairKey(int ent, const char* key, vec3_t v)
{
	char buf[MAX_EPAIRKEY];
	double v1, v2, v3;

	VectorClear(v);
	if (!AAS_ValueForBSPEpairKey(ent, key, buf, MAX_EPAIRKEY))
	{
		return qfalse;
	}
	//scanf into doubles, then assign, so it is vec_t size independent
	v1 = v2 = v3 = 0;
	sscanf(buf, "%lf %lf %lf", &v1, &v2, &v3);
	v[0] = v1;
	v[1] = v2;
	v[2] = v3;
	return qtrue;
}	//end of the function AAS_VectorForBSPEpairKey
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_FloatForBSPEpairKey(int ent, const char* key, float* value)
{
	char buf[MAX_EPAIRKEY];

	*value = 0;
	if (!AAS_ValueForBSPEpairKey(ent, key, buf, MAX_EPAIRKEY))
	{
		return qfalse;
	}
	*value = String::Atof(buf);
	return qtrue;
}	//end of the function AAS_FloatForBSPEpairKey
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_IntForBSPEpairKey(int ent, const char* key, int* value)
{
	char buf[MAX_EPAIRKEY];

	*value = 0;
	if (!AAS_ValueForBSPEpairKey(ent, key, buf, MAX_EPAIRKEY))
	{
		return qfalse;
	}
	*value = String::Atoi(buf);
	return qtrue;
}	//end of the function AAS_IntForBSPEpairKey
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_FreeBSPEntities(void)
{
// RF, optimized memory allocation
/*
    int i;
    bsp_entity_t *ent;
    bsp_epair_t *epair, *nextepair;

    for (i = 1; i < bspworld.numentities; i++)
    {
        ent = &bspworld.entities[i];
        for (epair = ent->epairs; epair; epair = nextepair)
        {
            nextepair = epair->next;
            //
            if (epair->key) FreeMemory(epair->key);
            if (epair->value) FreeMemory(epair->value);
            FreeMemory(epair);
        } //end for
    } //end for
*/
	if (bspworld.ebuffer)
	{
		FreeMemory(bspworld.ebuffer);
	}
	bspworld.numentities = 0;
}	//end of the function AAS_FreeBSPEntities
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_ParseBSPEntities(void)
{
	script_t* script;
	token_t token;
	bsp_entity_t* ent;
	bsp_epair_t* epair;
	byte* buffer, * buftrav;
	int bufsize;

	// RF, modified this, so that it first gathers up memory requirements, then allocates a single chunk,
	// and places the strings all in there

	bspworld.ebuffer = NULL;

	script = LoadScriptMemory(bspworld.dentdata, bspworld.entdatasize, "entdata");
	SetScriptFlags(script, SCFL_NOSTRINGWHITESPACES | SCFL_NOSTRINGESCAPECHARS);	//SCFL_PRIMITIVE);

	bufsize = 0;

	while (PS_ReadToken(script, &token))
	{
		if (String::Cmp(token.string, "{"))
		{
			ScriptError(script, "invalid %s\n", token.string);
			AAS_FreeBSPEntities();
			FreeScript(script);
			return;
		}	//end if
		if (bspworld.numentities >= MAX_BSPENTITIES)
		{
			BotImport_Print(PRT_MESSAGE, "too many entities in BSP file\n");
			break;
		}	//end if
		while (PS_ReadToken(script, &token))
		{
			if (!String::Cmp(token.string, "}"))
			{
				break;
			}
			bufsize += sizeof(bsp_epair_t);
			if (token.type != TT_STRING)
			{
				ScriptError(script, "invalid %s\n", token.string);
				AAS_FreeBSPEntities();
				FreeScript(script);
				return;
			}	//end if
			StripDoubleQuotes(token.string);
			bufsize += String::Length(token.string) + 1;
			if (!PS_ExpectTokenType(script, TT_STRING, 0, &token))
			{
				AAS_FreeBSPEntities();
				FreeScript(script);
				return;
			}	//end if
			StripDoubleQuotes(token.string);
			bufsize += String::Length(token.string) + 1;
		}	//end while
		if (String::Cmp(token.string, "}"))
		{
			ScriptError(script, "missing }\n");
			AAS_FreeBSPEntities();
			FreeScript(script);
			return;
		}	//end if
	}	//end while
	FreeScript(script);

	buffer = (byte*)GetClearedHunkMemory(bufsize);
	buftrav = buffer;
	bspworld.ebuffer = buffer;

	// RF, now parse the entities into memory
	// RF, NOTE: removed error checks for speed, no need to do them twice

	script = LoadScriptMemory(bspworld.dentdata, bspworld.entdatasize, "entdata");
	SetScriptFlags(script, SCFL_NOSTRINGWHITESPACES | SCFL_NOSTRINGESCAPECHARS);	//SCFL_PRIMITIVE);

	bspworld.numentities = 1;

	while (PS_ReadToken(script, &token))
	{
		ent = &bspworld.entities[bspworld.numentities];
		bspworld.numentities++;
		ent->epairs = NULL;
		while (PS_ReadToken(script, &token))
		{
			if (!String::Cmp(token.string, "}"))
			{
				break;
			}
			epair = (bsp_epair_t*)buftrav; buftrav += sizeof(bsp_epair_t);
			epair->next = ent->epairs;
			ent->epairs = epair;
			StripDoubleQuotes(token.string);
			epair->key = (char*)buftrav; buftrav += (String::Length(token.string) + 1);
			String::Cpy(epair->key, token.string);
			if (!PS_ExpectTokenType(script, TT_STRING, 0, &token))
			{
				AAS_FreeBSPEntities();
				FreeScript(script);
				return;
			}	//end if
			StripDoubleQuotes(token.string);
			epair->value = (char*)buftrav; buftrav += (String::Length(token.string) + 1);
			String::Cpy(epair->value, token.string);
		}	//end while
	}	//end while
	FreeScript(script);
}	//end of the function AAS_ParseBSPEntities
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_DumpBSPData(void)
{
	AAS_FreeBSPEntities();

	if (bspworld.dentdata)
	{
		FreeMemory(bspworld.dentdata);
	}
	bspworld.dentdata = NULL;
	bspworld.entdatasize = 0;
	//
	bspworld.loaded = qfalse;
	memset(&bspworld, 0, sizeof(bspworld));
}	//end of the function AAS_DumpBSPData
//===========================================================================
// load an bsp file
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_LoadBSPFile(void)
{
	AAS_DumpBSPData();
	bspworld.entdatasize = String::Length(CM_EntityString()) + 1;
	bspworld.dentdata = (char*)GetClearedHunkMemory(bspworld.entdatasize);
	memcpy(bspworld.dentdata, CM_EntityString(), bspworld.entdatasize);
	AAS_ParseBSPEntities();
	bspworld.loaded = qtrue;
	return BLERR_NOERROR;
}	//end of the function AAS_LoadBSPFile
