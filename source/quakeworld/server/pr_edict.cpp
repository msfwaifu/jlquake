/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// sv_edict.c -- entity dictionary

#include "qwsvdef.h"

globalvars_t* pr_global_struct;

func_t SpectatorConnect;
func_t SpectatorThink;
func_t SpectatorDisconnect;


/*
=================
ED_Alloc

Either finds a free edict, or allocates a new one.
Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
qhedict_t* ED_Alloc(void)
{
	int i;
	qhedict_t* e;

	for (i = MAX_CLIENTS_QW + 1; i < sv.qh_num_edicts; i++)
	{
		e = QH_EDICT_NUM(i);
		// the first couple seconds of server time can involve a lot of
		// freeing and allocating, so relax the replacement policy
		if (e->free && (e->freetime < 2 || sv.qh_time - e->freetime > 0.5))
		{
			ED_ClearEdict(e);
			return e;
		}
	}

	if (i == MAX_EDICTS_QH)
	{
		Con_Printf("WARNING: ED_Alloc: no free edicts\n");
		i--;	// step on whatever is the last edict
		e = QH_EDICT_NUM(i);
		SV_UnlinkEdict(e);
	}
	else
	{
		sv.qh_num_edicts++;
	}
	e = QH_EDICT_NUM(i);
	ED_ClearEdict(e);

	return e;
}

/*
=================
ED_Free

Marks the edict as free
FIXME: walk all entities and NULL out references to this entity
=================
*/
void ED_Free(qhedict_t* ed)
{
	SV_UnlinkEdict(ed);			// unlink from world bsp

	ed->free = true;
	ed->SetModel(0);
	ed->SetTakeDamage(0);
	ed->v.modelindex = 0;
	ed->SetColorMap(0);
	ed->SetSkin(0);
	ed->SetFrame(0);
	VectorCopy(vec3_origin, ed->GetOrigin());
	ed->SetAngles(vec3_origin);
	ed->SetNextThink(-1);
	ed->SetSolid(0);

	ed->freetime = sv.qh_time;
}

/*
================
ED_LoadFromFile

The entities are directly placed in the array, rather than allocated with
ED_Alloc, because otherwise an error loading the map would have entity
number references out of order.

Creates a server's entity / program execution context by
parsing textual entity definitions out of an ent file.

Used for both fresh maps and savegame loads.  A fresh map would also need
to call ED_CallSpawnFunctions () to let the objects initialize themselves.
================
*/
void ED_LoadFromFile(const char* data)
{
	qhedict_t* ent;
	int inhibit;
	dfunction_t* func;

	ent = NULL;
	inhibit = 0;
	pr_global_struct->time = sv.qh_time;

// parse ents
	while (1)
	{
// parse the opening brace
		char* token = String::Parse2(&data);
		if (!data)
		{
			break;
		}
		if (token[0] != '{')
		{
			SV_Error("ED_LoadFromFile: found %s when expecting {",token);
		}

		if (!ent)
		{
			ent = QH_EDICT_NUM(0);
		}
		else
		{
			ent = ED_Alloc();
		}
		data = ED_ParseEdict(data, ent);

// remove things from different skill levels or deathmatch
		if (((int)ent->GetSpawnFlags() & SPAWNFLAG_NOT_DEATHMATCH))
		{
			ED_Free(ent);
			inhibit++;
			continue;
		}

//
// immediately call spawn function
//
		if (!ent->GetClassName())
		{
			Con_Printf("No classname for:\n");
			ED_Print(ent);
			ED_Free(ent);
			continue;
		}

		// look for the spawn function
		func = ED_FindFunction(PR_GetString(ent->GetClassName()));

		if (!func)
		{
			Con_Printf("No spawn function for:\n");
			ED_Print(ent);
			ED_Free(ent);
			continue;
		}

		pr_global_struct->self = EDICT_TO_PROG(ent);
		PR_ExecuteProgram(func - pr_functions);
		SV_FlushSignon();
	}

	Con_DPrintf("%i entities inhibited\n", inhibit);
}


/*
===============
PR_LoadProgs
===============
*/
void PR_LoadProgs(void)
{
	int i;
	char num[32];
	dfunction_t* f;

	ED_ClearGEFVCache();

	progs = (dprograms_t*)COM_LoadHunkFile("qwprogs.dat");
	if (!progs)
	{
		progs = (dprograms_t*)COM_LoadHunkFile("progs.dat");
	}
	if (!progs)
	{
		SV_Error("PR_LoadProgs: couldn't load progs.dat");
	}
	Con_DPrintf("Programs occupy %iK.\n", com_filesize / 1024);

// add prog crc to the serverinfo
	sprintf(num, "%i", CRC_Block((byte*)progs, com_filesize));
	Info_SetValueForKey(svs.qh_info, "*progs", num, MAX_SERVERINFO_STRING, 64, 64, !sv_highchars->value);

// byte swap the header
	for (i = 0; i < (int)sizeof(*progs) / 4; i++)
		((int*)progs)[i] = LittleLong(((int*)progs)[i]);

	if (progs->version != PROG_VERSION)
	{
		SV_Error("progs.dat has wrong version number (%i should be %i)", progs->version, PROG_VERSION);
	}
	if (progs->crc != PROGHEADER_CRC)
	{
		SV_Error("You must have the progs.dat from QuakeWorld installed");
	}

	pr_functions = (dfunction_t*)((byte*)progs + progs->ofs_functions);
	pr_strings = (char*)progs + progs->ofs_strings;
	pr_globaldefs = (ddef_t*)((byte*)progs + progs->ofs_globaldefs);
	pr_fielddefs = (ddef_t*)((byte*)progs + progs->ofs_fielddefs);
	pr_statements = (dstatement_t*)((byte*)progs + progs->ofs_statements);

	PR_ClearStringMap();

	pr_global_struct = (globalvars_t*)((byte*)progs + progs->ofs_globals);
	pr_globals = (float*)pr_global_struct;

	pr_edict_size = progs->entityfields * 4 + sizeof(qhedict_t) - sizeof(entvars_t);

// byte swap the lumps
	for (i = 0; i < progs->numstatements; i++)
	{
		pr_statements[i].op = LittleShort(pr_statements[i].op);
		pr_statements[i].a = LittleShort(pr_statements[i].a);
		pr_statements[i].b = LittleShort(pr_statements[i].b);
		pr_statements[i].c = LittleShort(pr_statements[i].c);
	}

	for (i = 0; i < progs->numfunctions; i++)
	{
		pr_functions[i].first_statement = LittleLong(pr_functions[i].first_statement);
		pr_functions[i].parm_start = LittleLong(pr_functions[i].parm_start);
		pr_functions[i].s_name = LittleLong(pr_functions[i].s_name);
		pr_functions[i].s_file = LittleLong(pr_functions[i].s_file);
		pr_functions[i].numparms = LittleLong(pr_functions[i].numparms);
		pr_functions[i].locals = LittleLong(pr_functions[i].locals);
	}

	for (i = 0; i < progs->numglobaldefs; i++)
	{
		pr_globaldefs[i].type = LittleShort(pr_globaldefs[i].type);
		pr_globaldefs[i].ofs = LittleShort(pr_globaldefs[i].ofs);
		pr_globaldefs[i].s_name = LittleLong(pr_globaldefs[i].s_name);
	}

	for (i = 0; i < progs->numfielddefs; i++)
	{
		pr_fielddefs[i].type = LittleShort(pr_fielddefs[i].type);
		if (pr_fielddefs[i].type & DEF_SAVEGLOBAL)
		{
			SV_Error("PR_LoadProgs: pr_fielddefs[i].type & DEF_SAVEGLOBAL");
		}
		pr_fielddefs[i].ofs = LittleShort(pr_fielddefs[i].ofs);
		pr_fielddefs[i].s_name = LittleLong(pr_fielddefs[i].s_name);
	}

	for (i = 0; i < progs->numglobals; i++)
		((int*)pr_globals)[i] = LittleLong(((int*)pr_globals)[i]);

	// Zoid, find the spectator functions
	SpectatorConnect = SpectatorThink = SpectatorDisconnect = 0;

	if ((f = ED_FindFunction("SpectatorConnect")) != NULL)
	{
		SpectatorConnect = (func_t)(f - pr_functions);
	}
	if ((f = ED_FindFunction("SpectatorThink")) != NULL)
	{
		SpectatorThink = (func_t)(f - pr_functions);
	}
	if ((f = ED_FindFunction("SpectatorDisconnect")) != NULL)
	{
		SpectatorDisconnect = (func_t)(f - pr_functions);
	}

	ED_InitEntityFields();
}


/*
===============
PR_Init
===============
*/
void PR_Init(void)
{
	Cmd_AddCommand("edict", ED_PrintEdict_f);
	Cmd_AddCommand("edicts", ED_PrintEdicts);
	Cmd_AddCommand("edictcount", ED_Count);
	Cmd_AddCommand("profile", PR_Profile_f);
}
