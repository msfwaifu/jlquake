/*
 * $Header: /H2 Mission Pack/Host_cmd.c 25    4/01/98 4:53p Jmonroe $
 */

#include "quakedef.h"

#ifdef _WIN32
#include <windows.h>
#undef GetClassName
#endif
#include <time.h>

static double old_time;

void RestoreClients(void);

#define TESTSAVE

int LoadGamestate(char* level, char* startspot, int ClientsMode);

/*
==================
Host_Quit_f
==================
*/

extern void M_Menu_Quit_f(void);

void Host_Quit_f(void)
{
#ifndef DEDICATED
	if (!(in_keyCatchers & KEYCATCH_CONSOLE) && cls.state != CA_DEDICATED)
	{
		M_Menu_Quit_f();
		return;
	}
	CL_Disconnect();
#endif
	Host_ShutdownServer(false);

	Sys_Quit();
}


/*
==================
Host_Status_f
==================
*/
void Host_Status_f(void)
{
	client_t* client;
	int seconds;
	int minutes;
	int hours = 0;
	int j;

	if (sv.state == SS_DEAD)
	{
		Cmd_ForwardToServer();
		return;
	}

	common->Printf("host:    %s\n", Cvar_VariableString("hostname"));
	common->Printf("version: " JLQUAKE_VERSION_STRING "\n");
	SOCK_ShowIP();
	common->Printf("map:     %s\n", sv.name);
	common->Printf("players: %i active (%i max)\n\n", net_activeconnections, svs.qh_maxclients);
	for (j = 0, client = svs.clients; j < svs.qh_maxclients; j++, client++)
	{
		if (client->state < CS_CONNECTED)
		{
			continue;
		}
		seconds = (int)(net_time - client->qh_netconnection->connecttime);
		minutes = seconds / 60;
		if (minutes)
		{
			seconds -= (minutes * 60);
			hours = minutes / 60;
			if (hours)
			{
				minutes -= (hours * 60);
			}
		}
		else
		{
			hours = 0;
		}
		common->Printf("#%-2u %-16.16s  %3i  %2i:%02i:%02i\n", j + 1, client->name, (int)client->qh_edict->GetFrags(), hours, minutes, seconds);
		common->Printf("   %s\n", client->qh_netconnection->address);
	}
}

#ifndef DEDICATED

/*
==================
Host_God_f

Sets client to godmode
==================
*/
void Host_God_f(void)
{
	Cmd_ForwardToServer();
}

void Host_Notarget_f(void)
{
	Cmd_ForwardToServer();
}

void Host_Noclip_f(void)
{
	Cmd_ForwardToServer();
}

/*
==================
Host_Ping_f

==================
*/
void Host_Ping_f(void)
{
	Cmd_ForwardToServer();
}
#endif

/*
===============================================================================

SERVER TRANSITIONS

===============================================================================
*/


/*
======================
Host_Map_f

handle a
map <servername>
command from the console.  Active clients are kicked off.
======================
*/
void Host_Map_f(void)
{
	int i;
	char name[MAX_QPATH];

	if (Cmd_Argc() < 2)		//no map name given
	{
#ifndef DEDICATED
		common->Printf("map <levelname>: start a new server\nCurrently on: %s\n",cl.qh_levelname);
#endif
		return;
	}

#ifndef DEDICATED
	cls.qh_demonum = -1;		// stop demo loop in case this fails

	CL_Disconnect();
#endif
	Host_ShutdownServer(false);

#ifndef DEDICATED
	in_keyCatchers = 0;			// remove console or menu
	SCR_BeginLoadingPlaque();
#endif

	info_mask = 0;
	if (!svqh_coop->value && svqh_deathmatch->value)
	{
		info_mask2 = 0x80000000;
	}
	else
	{
		info_mask2 = 0;
	}

	svs.qh_serverflags = 0;			// haven't completed an episode yet
	String::Cpy(name, Cmd_Argv(1));
	SVQH_SpawnServer(name, NULL);
	if (sv.state == SS_DEAD)
	{
		return;
	}

#ifndef DEDICATED
	if (cls.state != CA_DEDICATED)
	{
		loading_stage = 2;

		String::Cpy(cls.qh_spawnparms, "");

		for (i = 2; i < Cmd_Argc(); i++)
		{
			String::Cat(cls.qh_spawnparms, sizeof(cls.qh_spawnparms), Cmd_Argv(i));
			String::Cat(cls.qh_spawnparms, sizeof(cls.qh_spawnparms), " ");
		}

		Cmd_ExecuteString("connect local");
	}
#endif
}

/*
==================
Host_Changelevel_f

Goes to a new map, taking all clients along
==================
*/
void Host_Changelevel_f(void)
{
	char level[MAX_QPATH];
	char _startspot[MAX_QPATH];
	char* startspot;

	if (Cmd_Argc() < 2)
	{
		common->Printf("changelevel <levelname> : continue game on a new level\n");
		return;
	}
#ifdef DEDICATED
	if (sv.state == SS_DEAD)
#else
	if (sv.state == SS_DEAD || clc.demoplaying)
#endif
	{
		common->Printf("Only the server may changelevel\n");
		return;
	}

	String::Cpy(level, Cmd_Argv(1));
	if (Cmd_Argc() == 2)
	{
		startspot = NULL;
	}
	else
	{
		String::Cpy(_startspot, Cmd_Argv(2));
		startspot = _startspot;
	}

	SVQH_SaveSpawnparms();
	SVQH_SpawnServer(level, startspot);

	//updatePlaqueMessage();
}

/*
==================
Host_Restart_f

Restarts the current server for a dead player
==================
*/
void Host_Restart_f(void)
{
	char mapname[MAX_QPATH];
	char startspot[MAX_QPATH];

#ifdef DEDICATED
	if (sv.state == SS_DEAD)
#else
	if (clc.demoplaying || sv.state == SS_DEAD)
#endif
	{
		return;
	}

	String::Cpy(mapname, sv.name);	// must copy out, because it gets cleared
	String::Cpy(startspot, sv.h2_startspot);

	if (Cmd_Argc() == 2 && String::ICmp(Cmd_Argv(1),"restore") == 0)
	{
		if (LoadGamestate(mapname, startspot, 3))
		{
			SVQH_SpawnServer(mapname, startspot);
			RestoreClients();
		}
	}
	else
	{
		// in sv_spawnserver
		SVQH_SpawnServer(mapname, startspot);
	}

//	updatePlaqueMessage();
}

#ifndef DEDICATED
/*
==================
Host_Reconnect_f

This command causes the client to wait for the signon messages again.
This is sent just before a server changes levels
==================
*/
void Host_Reconnect_f(void)
{
	CL_ClearParticles();	//jfm: for restarts which didn't use to clear parts.

	//updatePlaqueMessage();

	SCR_BeginLoadingPlaque();
	clc.qh_signon = 0;		// need new connection messages
}

/*
=====================
Host_Connect_f

User command to connect to server
=====================
*/
void Host_Connect_f(void)
{
	char name[MAX_QPATH];

	cls.qh_demonum = -1;		// stop demo loop in case this fails
	if (clc.demoplaying)
	{
		CL_StopPlayback();
		CL_Disconnect();
	}
	String::Cpy(name, Cmd_Argv(1));
	CL_EstablishConnection(name);
	Host_Reconnect_f();
}
#endif

/*
===============================================================================

LOAD / SAVE GAME

===============================================================================
*/

/*
===============
Host_Savegame_f
===============
*/
void Host_Savegame_f(void)
{
	fileHandle_t f;
	int i;
	char comment[SAVEGAME_COMMENT_LENGTH + 1];
	char dest[MAX_OSPATH];

	if (sv.state == SS_DEAD)
	{
		common->Printf("Not playing a local game.\n");
		return;
	}

#ifndef DEDICATED
	if (cl.qh_intermission)
	{
		common->Printf("Can't save in intermission.\n");
		return;
	}
#endif

#ifndef TESTSAVE
	if (svs.qh_maxclients != 1)
	{
		common->Printf("Can't save multiplayer games.\n");
		return;
	}
#endif

	if (Cmd_Argc() != 2)
	{
		common->Printf("save <savename> : save a game\n");
		return;
	}

	if (strstr(Cmd_Argv(1), ".."))
	{
		common->Printf("Relative pathnames are not allowed.\n");
		return;
	}

	for (i = 0; i < svs.qh_maxclients; i++)
	{
		if (svs.clients[i].state >= CS_CONNECTED && (svs.clients[i].qh_edict->GetHealth() <= 0))
		{
			common->Printf("Can't savegame with a dead player\n");
			return;
		}
	}


	SVH2_SaveGamestate(false);

#ifndef DEDICATED
	CL_RemoveGIPFiles(Cmd_Argv(1));
#endif

	char* netname = FS_BuildOSPath(fs_homepath->string, fs_gamedir, "clients.gip");
	FS_Remove(netname);

	sprintf(dest, "%s/", Cmd_Argv(1));
	common->Printf("Saving game to %s...\n", Cmd_Argv(1));

#ifndef DEDICATED
	CL_CopyFiles("", ".gip", dest);
#endif

	sprintf(dest,"%s/info.dat", Cmd_Argv(1));
	f = FS_FOpenFileWrite(dest);
	if (!f)
	{
		common->Printf("ERROR: couldn't open.\n");
		return;
	}

	FS_Printf(f, "%i\n", H2_SAVEGAME_VERSION);
	SVH2_SavegameComment(comment);
	FS_Printf(f, "%s\n", comment);
	for (i = 0; i < NUM_SPAWN_PARMS; i++)
		FS_Printf(f, "%f\n", svs.clients->qh_spawn_parms[i]);
	FS_Printf(f, "%d\n", svqh_current_skill);
	FS_Printf(f, "%s\n", sv.name);
	FS_Printf(f, "%f\n",sv.qh_time);
	FS_Printf(f, "%d\n",svs.qh_maxclients);
	FS_Printf(f, "%f\n",svqh_deathmatch->value);
	FS_Printf(f, "%f\n",svqh_coop->value);
	FS_Printf(f, "%f\n",svqh_teamplay->value);
	FS_Printf(f, "%f\n",h2_randomclass->value);
#ifndef DEDICATED
	FS_Printf(f, "%f\n",clh2_playerclass->value);
#endif
	FS_Printf(f, "%d\n",info_mask);
	FS_Printf(f, "%d\n",info_mask2);

	FS_FCloseFile(f);
}


static char* GetLine(char*& ReadPos)
{
	char* Line = ReadPos;
	while (*ReadPos)
	{
		if (*ReadPos == '\r')
		{
			*ReadPos = 0;
			ReadPos++;
		}
		if (*ReadPos == '\n')
		{
			*ReadPos = 0;
			ReadPos++;
			break;
		}
		ReadPos++;
	}
	return Line;
}

/*
===============
Host_Loadgame_f
===============
*/
void Host_Loadgame_f(void)
{
	char mapname[MAX_QPATH];
	float time;
	int i;
	qhedict_t* ent;
	int version;
	float tempf;
	int tempi;
	float spawn_parms[NUM_SPAWN_PARMS];
	char dest[MAX_OSPATH];

	if (Cmd_Argc() != 2)
	{
		common->Printf("load <savename> : load a game\n");
		return;
	}

#ifndef DEDICATED
	cls.qh_demonum = -1;		// stop demo loop in case this fails
	CL_Disconnect();
	CL_RemoveGIPFiles(NULL);
#endif

	common->Printf("Loading game from %s...\n", Cmd_Argv(1));

	sprintf(dest, "%s/info.dat", Cmd_Argv(1));

	Array<byte> Buffer;
	int Len = FS_ReadFile(dest, Buffer);
	if (Len <= 0)
	{
		common->Printf("ERROR: couldn't open.\n");
		return;
	}
	Buffer.Append(0);

	char* ReadPos = (char*)Buffer.Ptr();
	version = String::Atoi(GetLine(ReadPos));

	if (version != H2_SAVEGAME_VERSION)
	{
		common->Printf("Savegame is version %i, not %i\n", version, H2_SAVEGAME_VERSION);
		return;
	}
	GetLine(ReadPos);
	for (i = 0; i < NUM_SPAWN_PARMS; i++)
	{
		spawn_parms[i] = String::Atof(GetLine(ReadPos));
	}
	// this silliness is so we can load 1.06 save files, which have float skill values
	svqh_current_skill = (int)(String::Atof(GetLine(ReadPos)) + 0.1);
	Cvar_SetValue("skill", (float)svqh_current_skill);

	Cvar_SetValue("deathmatch", 0);
	Cvar_SetValue("coop", 0);
	Cvar_SetValue("teamplay", 0);
	Cvar_SetValue("randomclass", 0);

	String::Cpy(mapname, GetLine(ReadPos));
	time = String::Atof(GetLine(ReadPos));

	tempi = -1;
	tempi = String::Atoi(GetLine(ReadPos));
	if (tempi >= 1)
	{
		svs.qh_maxclients = tempi;
	}

	tempf = String::Atof(GetLine(ReadPos));
	if (tempf >= 0)
	{
		Cvar_SetValue("deathmatch", tempf);
	}

	tempf = String::Atof(GetLine(ReadPos));
	if (tempf >= 0)
	{
		Cvar_SetValue("coop", tempf);
	}

	tempf = String::Atof(GetLine(ReadPos));
	if (tempf >= 0)
	{
		Cvar_SetValue("teamplay", tempf);
	}

	tempf = String::Atof(GetLine(ReadPos));
	if (tempf >= 0)
	{
		Cvar_SetValue("randomclass", tempf);
	}

	tempf = String::Atof(GetLine(ReadPos));
	if (tempf >= 0)
	{
		Cvar_SetValue("_cl_playerclass", tempf);
	}

	info_mask = String::Atoi(GetLine(ReadPos));
	info_mask2 = String::Atoi(GetLine(ReadPos));

#ifndef DEDICATED
	sprintf(dest, "%s/", Cmd_Argv(1));
	CL_CopyFiles(dest, ".gip", "");
#endif

	LoadGamestate(mapname, NULL, 2);

	SVQH_SaveSpawnparms();

	ent = QH_EDICT_NUM(1);

	Cvar_SetValue("_cl_playerclass", ent->GetPlayerClass());//this better be the same as above...

#ifdef MISSIONPACK
	// this may be rudundant with the setting in PR_LoadProgs, but not sure so its here too
	*pr_globalVars.cl_playerclass = ent->GetPlayerClass();
#endif

	svs.clients->h2_playerclass = ent->GetPlayerClass();

	sv.qh_paused = true;		// pause until all clients connect
	sv.loadgame = true;

#ifndef DEDICATED
	if (cls.state != CA_DEDICATED)
	{
		CL_EstablishConnection("local");
		Host_Reconnect_f();
	}
#endif
}

void RestoreClients(void)
{
	int i,j;
	qhedict_t* ent;
	double time_diff;

	if (LoadGamestate(NULL,NULL,1))
	{
		return;
	}

	time_diff = sv.qh_time - old_time;

	for (i = 0,host_client = svs.clients; i < svs.qh_maxclients; i++, host_client++)
		if (host_client->state >= CS_CONNECTED)
		{
			ent = host_client->qh_edict;

			//ent->v.colormap = QH_NUM_FOR_EDICT(ent);
			ent->SetTeam((host_client->qh_colors & 15) + 1);
			ent->SetNetName(PR_SetString(host_client->name));
			ent->SetPlayerClass(host_client->h2_playerclass);

			// copy spawn parms out of the client_t

			for (j = 0; j < NUM_SPAWN_PARMS; j++)
				pr_globalVars.parm1[j] = host_client->qh_spawn_parms[j];

			// call the spawn function

			*pr_globalVars.time = sv.qh_time;
			*pr_globalVars.self = EDICT_TO_PROG(ent);
			G_FLOAT(OFS_PARM0) = time_diff;
			PR_ExecuteProgram(*pr_globalVars.ClientReEnter);
		}
	SVH2_SaveGamestate(true);
}

int LoadGamestate(char* level, char* startspot, int ClientsMode)
{
	char name[MAX_OSPATH];
	char mapname[MAX_QPATH];
	float time, sk;
	int i;
	qhedict_t* ent;
	int entnum;
	int version;
//	float	spawn_parms[NUM_SPAWN_PARMS];
	qboolean auto_correct = false;

	if (ClientsMode == 1)
	{
		sprintf(name, "clients.gip");
	}
	else
	{
		sprintf(name, "%s.gip", level);

		if (ClientsMode != 2 && ClientsMode != 3)
		{
			common->Printf("Loading game from %s...\n", name);
		}
	}

	Array<byte> Buffer;
	int len = FS_ReadFile(name, Buffer);
	if (len <= 0)
	{
		if (ClientsMode == 2)
		{
			common->Printf("ERROR: couldn't open.\n");
		}

		return -1;
	}
	Buffer.Append(0);

	char* ReadPos = (char*)Buffer.Ptr();
	version = String::Atoi(GetLine(ReadPos));

	if (version != H2_SAVEGAME_VERSION)
	{
		common->Printf("Savegame is version %i, not %i\n", version, H2_SAVEGAME_VERSION);
		return -1;
	}

	if (ClientsMode != 1)
	{
		GetLine(ReadPos);
		sk = String::Atof(GetLine(ReadPos));
		Cvar_SetValue("skill", sk);

		String::Cpy(mapname, GetLine(ReadPos));
		time = String::Atof(GetLine(ReadPos));

		SVQH_SpawnServer(mapname, startspot);

		if (sv.state == SS_DEAD)
		{
			common->Printf("Couldn't load map\n");
			return -1;
		}

		// load the light styles
		for (i = 0; i < MAX_LIGHTSTYLES_Q1; i++)
		{
			char* Style = GetLine(ReadPos);
			char* Tmp = (char*)Hunk_Alloc(String::Length(Style) + 1);
			String::Cpy(Tmp, Style);
			sv.qh_lightstyles[i] = Tmp;
		}
		ReadPos = const_cast<char*>(SVH2_LoadEffects(ReadPos));
	}

	// load the edicts out of the savegame file
	const char* start = ReadPos;
	while (start)
	{
		char* token = String::Parse1(&start);
		if (!start)
		{
			break;		// end of file
		}
		entnum = String::Atoi(token);
		token = String::Parse1(&start);
		if (String::Cmp(token, "{"))
		{
			common->FatalError("First token isn't a brace");
		}

		// parse an edict

		if (entnum == -1)
		{
			start = ED_ParseGlobals(start);
			// Need to restore this
			*pr_globalVars.startspot = PR_SetString(sv.h2_startspot);
		}
		else
		{
			ent = QH_EDICT_NUM(entnum);
			Com_Memset(&ent->v, 0, progs->entityfields * 4);
			//ent->free = false;
			start = ED_ParseEdict(start, ent);

			if (ClientsMode == 1 || ClientsMode == 2 || ClientsMode == 3)
			{
				ent->SetStatsRestored(true);
			}

			// link it into the bsp tree
			if (!ent->free)
			{
				SVQH_LinkEdict(ent, false);
				if (ent->v.modelindex && ent->GetModel())
				{
					i = SVQH_ModelIndex(PR_GetString(ent->GetModel()));
					if (i != ent->v.modelindex)
					{
						ent->v.modelindex = i;
						auto_correct = true;
					}
				}
			}
		}
	}

	//sv.num_edicts = entnum;
	if (ClientsMode == 0)
	{
		sv.qh_time = time;
		sv.qh_paused = true;

		*pr_globalVars.serverflags = svs.qh_serverflags;

		RestoreClients();
	}
	else if (ClientsMode == 2)
	{
		sv.qh_time = time;
	}
	else if (ClientsMode == 3)
	{
		sv.qh_time = time;

		*pr_globalVars.serverflags = svs.qh_serverflags;

		RestoreClients();
	}

	if (ClientsMode != 1 && auto_correct)
	{
		common->DPrintf("*** Auto-corrected model indexes!\n");
	}

//	for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
//		svs.clients->spawn_parms[i] = spawn_parms[i];

	return 0;
}

// changing levels within a unit
void Host_Changelevel2_f(void)
{
	char level[MAX_QPATH];
	char _startspot[MAX_QPATH];
	char* startspot;

	if (Cmd_Argc() < 2)
	{
		common->Printf("changelevel2 <levelname> : continue game on a new level in the unit\n");
		return;
	}
#ifdef DEDICATED
	if (sv.state == SS_DEAD)
#else
	if (sv.state == SS_DEAD || clc.demoplaying)
#endif
	{
		common->Printf("Only the server may changelevel\n");
		return;
	}

	String::Cpy(level, Cmd_Argv(1));
	if (Cmd_Argc() == 2)
	{
		startspot = NULL;
	}
	else
	{
		String::Cpy(_startspot, Cmd_Argv(2));
		startspot = _startspot;
	}

	SVQH_SaveSpawnparms();

	// save the current level's state
	old_time = sv.qh_time;
	SVH2_SaveGamestate(false);

	// try to restore the new level
	if (LoadGamestate(level, startspot, 0))
	{
		SVQH_SpawnServer(level, startspot);
		RestoreClients();
	}
}


//============================================================================

#ifndef DEDICATED
/*
======================
Host_Name_f
======================
*/
void Host_Name_f(void)
{
	char* newName;

	if (Cmd_Argc() == 1)
	{
		common->Printf("\"name\" is \"%s\"\n", clqh_name->string);
		return;
	}
	if (Cmd_Argc() == 2)
	{
		newName = Cmd_Argv(1);
	}
	else
	{
		newName = Cmd_ArgsUnmodified();
	}
	newName[15] = 0;

	if (GGameType & GAME_Hexen2)
	{
		//this is for the fuckers who put braces in the name causing loadgame to crash.
		char* pdest = strchr(newName,'{');
		if (pdest)
		{
			*pdest = 0;	//zap the brace
			common->Printf("Illegal char in name removed!\n");
		}
	}

	if (String::Cmp(clqh_name->string, newName) == 0)
	{
		return;
	}
	Cvar_Set("_cl_name", newName);
	if (cls.state == CA_ACTIVE)
	{
		Cmd_ForwardToServer();
	}
}

extern const char* ClassNames[NUM_CLASSES];	//from menu.c
void Host_Class_f(void)
{
	float newClass;

	if (Cmd_Argc() == 1)
	{
		if (!(int)clh2_playerclass->value)
		{
			common->Printf("\"playerclass\" is %d (\"unknown\")\n", (int)clh2_playerclass->value);
		}
		else
		{
			common->Printf("\"playerclass\" is %d (\"%s\")\n", (int)clh2_playerclass->value,ClassNames[(int)clh2_playerclass->value - 1]);
		}
		return;
	}
	if (Cmd_Argc() == 2)
	{
		newClass = String::Atof(Cmd_Argv(1));
	}
	else
	{
		newClass = String::Atof(Cmd_ArgsUnmodified());
	}

	Cvar_SetValue("_cl_playerclass", newClass);

	if (GGameType & GAME_H2Portals)
	{
		// when classes changes after map load, update cl_playerclass, cl_playerclass should
		// probably only be used in worldspawn, though
		if (pr_globalVars.cl_playerclass)
		{
			*pr_globalVars.cl_playerclass = newClass;
		}
	}

	if (cls.state == CA_ACTIVE)
	{
		Cmd_ForwardToServer();
	}
}
#endif

void Host_Version_f(void)
{
	common->Printf("Version %4.2f\n", HEXEN2_VERSION);
	common->Printf("Exe: "__TIME__ " "__DATE__ "\n");
}

#ifndef DEDICATED

void Host_Say_f()
{
	Cmd_ForwardToServer();
}

void Host_Say_Team_f(void)
{
	Cmd_ForwardToServer();
}

#endif

void Host_ConSay_f()
{
	client_t* client;
	int j;
	char* p;
	char text[64];

	if (Cmd_Argc() < 2)
	{
		return;
	}

	p = Cmd_ArgsUnmodified();
	// remove quotes if present
	if (*p == '"')
	{
		p++;
		p[String::Length(p) - 1] = 0;
	}

	// turn on color set 1
	sprintf(text, "%c<%s> ", 1, sv_hostname->string);

	j = sizeof(text) - 2 - String::Length(text);	// -2 for /n and null terminator
	if (String::Length(p) > j)
	{
		p[j] = 0;
	}

	String::Cat(text, sizeof(text), p);
	String::Cat(text, sizeof(text), "\n");

	for (j = 0, client = svs.clients; j < svs.qh_maxclients; j++, client++)
	{
		if (client->state != CS_ACTIVE)
		{
			continue;
		}
		SVQH_ClientPrintf(client, 0, "%s", text);
	}
}

#ifndef DEDICATED
void Host_Tell_f(void)
{
	Cmd_ForwardToServer();
}

void Host_Color_f(void)
{
	int top, bottom;
	int playercolor;

	if (Cmd_Argc() == 1)
	{
		common->Printf("\"color\" is \"%i %i\"\n", ((int)clqh_color->value) >> 4, ((int)clqh_color->value) & 0x0f);
		common->Printf("color <0-13> [0-13]\n");
		return;
	}

	if (Cmd_Argc() == 2)
	{
		top = bottom = String::Atoi(Cmd_Argv(1));
	}
	else
	{
		top = String::Atoi(Cmd_Argv(1));
		bottom = String::Atoi(Cmd_Argv(2));
	}

	top &= 15;
	if (top > 13)
	{
		top = 13;
	}
	bottom &= 15;
	if (bottom > 13)
	{
		bottom = 13;
	}

	playercolor = top * 16 + bottom;

	Cvar_SetValue("_cl_color", playercolor);
	if (cls.state == CA_ACTIVE)
	{
		Cmd_ForwardToServer();
	}
}

/*
==================
Host_Kill_f
==================
*/
void Host_Kill_f(void)
{
	Cmd_ForwardToServer();
}


/*
==================
Host_Pause_f
==================
*/
void Host_Pause_f(void)
{
	Cmd_ForwardToServer();
}
#endif

//===========================================================================

int strdiff(const char* s1, const char* s2)
{
	int L1,L2,i;

	L1 = String::Length(s1);
	L2 = String::Length(s2);

	for (i = 0; (i < L1 && i < L2); i++)
	{
		if (String::ToLower(s1[i]) != String::ToLower(s2[i]))
		{
			break;
		}
	}

	return i;
}

//===========================================================================


/*
==================
Host_Kick_f

Kicks a user off of the server
==================
*/
void Host_Kick_f(void)
{
	const char* who;
	const char* message = NULL;
	int i;
	qboolean byNumber = false;

	if (sv.state == SS_DEAD)
	{
		Cmd_ForwardToServer();
		return;
	}

	if (Cmd_Argc() > 2 && String::Cmp(Cmd_Argv(1), "#") == 0)
	{
		i = String::Atof(Cmd_Argv(2)) - 1;
		if (i < 0 || i >= svs.qh_maxclients)
		{
			return;
		}
		if (svs.clients[i].state < CS_CONNECTED)
		{
			return;
		}
		host_client = &svs.clients[i];
		byNumber = true;
	}
	else
	{
		for (i = 0, host_client = svs.clients; i < svs.qh_maxclients; i++, host_client++)
		{
			if (host_client->state < CS_CONNECTED)
			{
				continue;
			}
			if (String::ICmp(host_client->name, Cmd_Argv(1)) == 0)
			{
				break;
			}
		}
	}

	if (i < svs.qh_maxclients)
	{
#ifndef DEDICATED
		if (cls.state == CA_DEDICATED)
#endif
		{
			who = "Console";
		}
#ifndef DEDICATED
		else
		{
			who = clqh_name->string;
		}
#endif

		if (Cmd_Argc() > 2)
		{
			message = Cmd_ArgsUnmodified();
			String::Parse1(&message);
			if (byNumber)
			{
				message++;							// skip the #
				while (*message == ' ')				// skip white space
					message++;
				message += String::Length(Cmd_Argv(2));	// skip the number
			}
			while (*message && *message == ' ')
				message++;
		}
		if (message)
		{
			SVQH_ClientPrintf(host_client, 0, "Kicked by %s: %s\n", who, message);
		}
		else
		{
			SVQH_ClientPrintf(host_client, 0, "Kicked by %s\n", who);
		}
		SVQH_DropClient(host_client, false);
	}
}

/*
===============================================================================

DEBUGGING TOOLS

===============================================================================
*/

#ifndef DEDICATED
/*
==================
Host_Give_f
==================
*/
void Host_Give_f(void)
{
	Cmd_ForwardToServer();
}

qhedict_t* FindViewthing(void)
{
	int i;
	qhedict_t* e;

	for (i = 0; i < sv.qh_num_edicts; i++)
	{
		e = QH_EDICT_NUM(i);
		if (!String::Cmp(PR_GetString(e->GetClassName()), "viewthing"))
		{
			return e;
		}
	}
	common->Printf("No viewthing on map\n");
	return NULL;
}

/*
==================
Host_Viewmodel_f
==================
*/
void Host_Viewmodel_f(void)
{
	qhedict_t* e;
	qhandle_t m;

	e = FindViewthing();
	if (!e)
	{
		return;
	}

	m = R_RegisterModel(Cmd_Argv(1));
	if (!m)
	{
		common->Printf("Can't load %s\n", Cmd_Argv(1));
		return;
	}

	e->SetFrame(0);
	cl.model_draw[(int)e->v.modelindex] = m;
}

/*
==================
Host_Viewframe_f
==================
*/
void Host_Viewframe_f(void)
{
	qhedict_t* e;
	int f;
	qhandle_t m;

	e = FindViewthing();
	if (!e)
	{
		return;
	}
	m = cl.model_draw[(int)e->v.modelindex];

	f = String::Atoi(Cmd_Argv(1));
	if (f >= R_ModelNumFrames(m))
	{
		f = R_ModelNumFrames(m) - 1;
	}

	e->SetFrame(f);
}

/*
==================
Host_Viewnext_f
==================
*/
void Host_Viewnext_f(void)
{
	qhedict_t* e;
	qhandle_t m;

	e = FindViewthing();
	if (!e)
	{
		return;
	}
	m = cl.model_draw[(int)e->v.modelindex];

	e->SetFrame(e->GetFrame() + 1);
	if (e->GetFrame() >= R_ModelNumFrames(m))
	{
		e->SetFrame(R_ModelNumFrames(m) - 1);
	}

	R_PrintModelFrameName(m, e->GetFrame());
}

/*
==================
Host_Viewprev_f
==================
*/
void Host_Viewprev_f(void)
{
	qhedict_t* e;
	qhandle_t m;

	e = FindViewthing();
	if (!e)
	{
		return;
	}

	m = cl.model_draw[(int)e->v.modelindex];

	e->SetFrame(e->GetFrame() - 1);
	if (e->GetFrame() < 0)
	{
		e->SetFrame(0);
	}

	R_PrintModelFrameName(m, e->GetFrame());
}
#endif

/*
===============================================================================

DEMO LOOP CONTROL

===============================================================================
*/


/*
==================
Host_Startdemos_f
==================
*/
void Host_Startdemos_f(void)
{
#ifndef DEDICATED
	int i, c;

	if (cls.state == CA_DEDICATED)
#endif
	{
		if (sv.state == SS_DEAD)
		{
			Cbuf_AddText("map start\n");
		}
		return;
	}

#ifndef DEDICATED
	c = Cmd_Argc() - 1;
	if (c > MAX_DEMOS)
	{
		common->Printf("Max %i demos in demoloop\n", MAX_DEMOS);
		c = MAX_DEMOS;
	}
	common->Printf("%i demo(s) in loop\n", c);

	for (i = 1; i < c + 1; i++)
		String::NCpy(cls.qh_demos[i - 1], Cmd_Argv(i), sizeof(cls.qh_demos[0]) - 1);

	if (sv.state == SS_DEAD && cls.qh_demonum != -1 && !clc.demoplaying)
	{
		cls.qh_demonum = 0;
		CL_NextDemo();
	}
	else
	{
		cls.qh_demonum = -1;
	}
#endif
}

#ifndef DEDICATED
/*
==================
Host_Demos_f

Return to looping demos
==================
*/
void Host_Demos_f(void)
{
	if (cls.state == CA_DEDICATED)
	{
		return;
	}
	if (cls.qh_demonum == -1)
	{
		cls.qh_demonum = 1;
	}
	CL_Disconnect_f();
	CL_NextDemo();
}

/*
==================
Host_Stopdemo_f

Return to looping demos
==================
*/
void Host_Stopdemo_f(void)
{
	if (cls.state == CA_DEDICATED)
	{
		return;
	}
	if (!clc.demoplaying)
	{
		return;
	}
	CL_StopPlayback();
	CL_Disconnect();
}
#endif

//=============================================================================

/*
==================
Host_InitCommands
==================
*/
void Host_InitCommands(void)
{
	Cmd_AddCommand("status", Host_Status_f);
	Cmd_AddCommand("quit", Host_Quit_f);
#ifndef DEDICATED
	Cmd_AddCommand("god", Host_God_f);
	Cmd_AddCommand("notarget", Host_Notarget_f);
#endif
	Cmd_AddCommand("map", Host_Map_f);
	Cmd_AddCommand("restart", Host_Restart_f);
	Cmd_AddCommand("changelevel", Host_Changelevel_f);
	Cmd_AddCommand("changelevel2", Host_Changelevel2_f);
#ifndef DEDICATED
	Cmd_AddCommand("connect", Host_Connect_f);
	Cmd_AddCommand("reconnect", Host_Reconnect_f);
	Cmd_AddCommand("name", Host_Name_f);
	Cmd_AddCommand("playerclass", Host_Class_f);
	Cmd_AddCommand("noclip", Host_Noclip_f);
#endif
	Cmd_AddCommand("version", Host_Version_f);
	if (com_dedicated->integer)
	{
		Cmd_AddCommand("say", Host_ConSay_f);
	}
#ifndef DEDICATED
	else
	{
		Cmd_AddCommand("say", Host_Say_f);
		Cmd_AddCommand("say_team", Host_Say_Team_f);
	}
	Cmd_AddCommand("tell", Host_Tell_f);
	Cmd_AddCommand("color", Host_Color_f);
	Cmd_AddCommand("kill", Host_Kill_f);
	Cmd_AddCommand("pause", Host_Pause_f);
#endif
	Cmd_AddCommand("kick", Host_Kick_f);
#ifndef DEDICATED
	Cmd_AddCommand("ping", Host_Ping_f);
#endif
	Cmd_AddCommand("load", Host_Loadgame_f);
	Cmd_AddCommand("save", Host_Savegame_f);
#ifndef DEDICATED
	Cmd_AddCommand("give", Host_Give_f);
#endif

	Cmd_AddCommand("startdemos", Host_Startdemos_f);
#ifndef DEDICATED
	Cmd_AddCommand("demos", Host_Demos_f);
	Cmd_AddCommand("stopdemo", Host_Stopdemo_f);

	Cmd_AddCommand("viewmodel", Host_Viewmodel_f);
	Cmd_AddCommand("viewframe", Host_Viewframe_f);
	Cmd_AddCommand("viewnext", Host_Viewnext_f);
	Cmd_AddCommand("viewprev", Host_Viewprev_f);
#endif
}
