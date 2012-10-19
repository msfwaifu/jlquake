/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// cl_main.c  -- client main loop

#include "client.h"
#include <limits.h>
#include "../../server/public.h"

void BotDrawDebugPolygons(void (* drawPoly)(int color, int numPoints, float* points), int value);

Cvar* rcon_client_password;
Cvar* rconAddress;

Cvar* cl_timeout;
Cvar* cl_timeNudge;
Cvar* cl_freezeDemo;

Cvar* cl_avidemo;
Cvar* cl_forceavidemo;

Cvar* cl_motdString;

Cvar* cl_trn;

void CL_ShowIP_f(void);

/*
=======================================================================

CLIENT RELIABLE COMMAND COMMUNICATION

=======================================================================
*/

/*
======================
CL_ChangeReliableCommand
======================
*/
void CL_ChangeReliableCommand(void)
{
	int r, index, l;

	r = clc.q3_reliableSequence - (random() * 5);
	index = clc.q3_reliableSequence & (MAX_RELIABLE_COMMANDS_Q3 - 1);
	l = String::Length(clc.q3_reliableCommands[index]);
	if (l >= MAX_STRING_CHARS - 1)
	{
		l = MAX_STRING_CHARS - 2;
	}
	clc.q3_reliableCommands[index][l] = '\n';
	clc.q3_reliableCommands[index][l + 1] = '\0';
}

/*
=================
CL_ReadDemoMessage
=================
*/
void CL_ReadDemoMessage(void)
{
	int r;
	QMsg buf;
	byte bufData[MAX_MSGLEN_Q3];
	int s;

	if (!clc.demofile)
	{
		CLT3_DemoCompleted();
		return;
	}

	// get the sequence number
	r = FS_Read(&s, 4, clc.demofile);
	if (r != 4)
	{
		CLT3_DemoCompleted();
		return;
	}
	clc.q3_serverMessageSequence = LittleLong(s);

	// init the message
	buf.Init(bufData, sizeof(bufData));

	// get the length
	r = FS_Read(&buf.cursize, 4, clc.demofile);
	if (r != 4)
	{
		CLT3_DemoCompleted();
		return;
	}
	buf.cursize = LittleLong(buf.cursize);
	if (buf.cursize == -1)
	{
		CLT3_DemoCompleted();
		return;
	}
	if (buf.cursize > buf.maxsize)
	{
		common->Error("CL_ReadDemoMessage: demoMsglen > MAX_MSGLEN_Q3");
	}
	r = FS_Read(buf._data, buf.cursize, clc.demofile);
	if (r != buf.cursize)
	{
		common->Printf("Demo file was truncated.\n");
		CLT3_DemoCompleted();
		return;
	}

	clc.q3_lastPacketTime = cls.realtime;
	buf.readcount = 0;
	CLT3_ParseServerMessage(&buf);
}

/*
====================
CL_WalkDemoExt
====================
*/
static void CL_WalkDemoExt(char* arg, char* name, int* demofile)
{
	int i = 0;
	*demofile = 0;
	while (demo_protocols[i])
	{
		String::Sprintf(name, MAX_OSPATH, "demos/%s.dm_%d", arg, demo_protocols[i]);
		FS_FOpenFileRead(name, demofile, true);
		if (*demofile)
		{
			common->Printf("Demo file: %s\n", name);
			break;
		}
		else
		{
			common->Printf("Not found: %s\n", name);
		}
		i++;
	}
}

/*
====================
CL_PlayDemo_f

demo <demoname>

====================
*/
void CL_PlayDemo_f(void)
{
	char name[MAX_OSPATH];
	char* arg, * ext_test;
	int protocol, i;
	char retry[MAX_OSPATH];

	if (Cmd_Argc() != 2)
	{
		common->Printf("playdemo <demoname>\n");
		return;
	}

	// make sure a local server is killed
	Cvar_Set("sv_killserver", "1");

	CL_Disconnect(true);

	// open the demo file
	arg = Cmd_Argv(1);

	// check for an extension .dm_?? (?? is protocol)
	ext_test = arg + String::Length(arg) - 6;
	if ((String::Length(arg) > 6) && (ext_test[0] == '.') && ((ext_test[1] == 'd') || (ext_test[1] == 'D')) && ((ext_test[2] == 'm') || (ext_test[2] == 'M')) && (ext_test[3] == '_'))
	{
		protocol = String::Atoi(ext_test + 4);
		i = 0;
		while (demo_protocols[i])
		{
			if (demo_protocols[i] == protocol)
			{
				break;
			}
			i++;
		}
		if (demo_protocols[i])
		{
			String::Sprintf(name, sizeof(name), "demos/%s", arg);
			FS_FOpenFileRead(name, &clc.demofile, true);
		}
		else
		{
			common->Printf("Protocol %d not supported for demos\n", protocol);
			String::NCpyZ(retry, arg, sizeof(retry));
			retry[String::Length(retry) - 6] = 0;
			CL_WalkDemoExt(retry, name, &clc.demofile);
		}
	}
	else
	{
		CL_WalkDemoExt(arg, name, &clc.demofile);
	}

	if (!clc.demofile)
	{
		common->Error("couldn't open %s", name);
		return;
	}
	String::NCpyZ(clc.q3_demoName, Cmd_Argv(1), sizeof(clc.q3_demoName));

	Con_Close();

	cls.state = CA_CONNECTED;
	clc.demoplaying = true;
	String::NCpyZ(cls.servername, Cmd_Argv(1), sizeof(cls.servername));

	// read demo messages until connected
	while (cls.state >= CA_CONNECTED && cls.state < CA_PRIMED)
	{
		CL_ReadDemoMessage();
	}
	// don't get the first snapshot this frame, to prevent the long
	// time from the gamestate load from messing causing a time skip
	clc.q3_firstDemoFrameSkipped = false;
}

/*
==================
CL_NextDemo

Called when a demo or cinematic finishes
If the "nextdemo" cvar is set, that command will be issued
==================
*/
void CL_NextDemo(void)
{
	char v[MAX_STRING_CHARS];

	String::NCpyZ(v, Cvar_VariableString("nextdemo"), sizeof(v));
	v[MAX_STRING_CHARS - 1] = 0;
	common->DPrintf("CL_NextDemo: %s\n", v);
	if (!v[0])
	{
		return;
	}

	Cvar_Set("nextdemo","");
	Cbuf_AddText(v);
	Cbuf_AddText("\n");
	Cbuf_Execute();
}

/*
======================================================================

CONSOLE COMMANDS

======================================================================
*/

/*
==================
CL_Setenv_f

Mostly for controlling voodoo environment variables
==================
*/
void CL_Setenv_f(void)
{
	int argc = Cmd_Argc();

	if (argc > 2)
	{
		char buffer[1024];
		int i;

		String::Cpy(buffer, Cmd_Argv(1));
		String::Cat(buffer, sizeof(buffer), "=");

		for (i = 2; i < argc; i++)
		{
			String::Cat(buffer, sizeof(buffer), Cmd_Argv(i));
			String::Cat(buffer, sizeof(buffer), " ");
		}

		putenv(buffer);
	}
	else if (argc == 2)
	{
		char* env = getenv(Cmd_Argv(1));

		if (env)
		{
			common->Printf("%s=%s\n", Cmd_Argv(1), env);
		}
		else
		{
			common->Printf("%s undefined\n", Cmd_Argv(1), env);
		}
	}
}


/*
==================
CL_Disconnect_f
==================
*/
void CL_Disconnect_f(void)
{
	SCR_StopCinematic();
	Cvar_Set("ui_singlePlayerActive", "0");
	if (cls.state != CA_DISCONNECTED && cls.state != CA_CINEMATIC)
	{
		Com_Error(ERR_DISCONNECT, "Disconnected from server");
	}
}

/*
=====================
CL_Rcon_f

  Send the rest of the command line over as
  an unconnected command.
=====================
*/
void CL_Rcon_f(void)
{
	char message[1024];
	netadr_t to;

	if (!rcon_client_password->string)
	{
		common->Printf("You must set 'rconpassword' before\n"
				   "issuing an rcon command.\n");
		return;
	}

	message[0] = -1;
	message[1] = -1;
	message[2] = -1;
	message[3] = -1;
	message[4] = 0;

	String::Cat(message, sizeof(message), "rcon ");

	String::Cat(message, sizeof(message), rcon_client_password->string);
	String::Cat(message, sizeof(message), " ");

	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=543
	String::Cat(message, sizeof(message), Cmd_Cmd() + 5);

	if (cls.state >= CA_CONNECTED)
	{
		to = clc.netchan.remoteAddress;
	}
	else
	{
		if (!String::Length(rconAddress->string))
		{
			common->Printf("You must either be connected,\n"
					   "or set the 'rconAddress' cvar\n"
					   "to issue rcon commands\n");

			return;
		}
		SOCK_StringToAdr(rconAddress->string, &to, Q3PORT_SERVER);
	}

	NET_SendPacket(NS_CLIENT, String::Length(message) + 1, message, to);
}

/*
=================
CL_ResetPureClientAtServer
=================
*/
void CL_ResetPureClientAtServer(void)
{
	CL_AddReliableCommand(va("vdr"));
}

/*
=================
CL_Vid_Restart_f

Restart the video subsystem

we also have to reload the UI and CGame because the renderer
doesn't know what graphics to reload
=================
*/
void CL_Vid_Restart_f(void)
{

	// don't let them loop during the restart
	S_StopAllSounds();
	// shutdown the UI
	CLT3_ShutdownUI();
	// shutdown the CGame
	CLT3_ShutdownCGame();
	// shutdown the renderer and clear the renderer interface
	CL_ShutdownRef();
	// client is no longer pure untill new checksums are sent
	CL_ResetPureClientAtServer();
	// clear pak references
	FS_ClearPakReferences(FS_UI_REF | FS_CGAME_REF);
	// reinitialize the filesystem if the game directory or checksum has changed
	FS_ConditionalRestart(clc.q3_checksumFeed);

	cls.q3_rendererStarted = false;
	cls.q3_uiStarted = false;
	cls.q3_cgameStarted = false;
	cls.q3_soundRegistered = false;

	// unpause so the cgame definately gets a snapshot and renders a frame
	Cvar_Set("cl_paused", "0");

	CIN_CloseAllVideos();

	// initialize the renderer interface
	CL_InitRef();

	// startup all the client stuff
	CLT3_StartHunkUsers();

	// start the cgame if connected
	if (cls.state > CA_CONNECTED && cls.state != CA_CINEMATIC)
	{
		cls.q3_cgameStarted = true;
		CLT3_InitCGame();
		// send pure checksums
		CLT3_SendPureChecksums();
	}
}

/*
=================
CL_Snd_Restart_f

Restart the sound subsystem
The cgame and game must also be forced to restart because
handles will be invalid
=================
*/
void CL_Snd_Restart_f(void)
{
	S_Shutdown();
	S_Init();

	CL_Vid_Restart_f();
}


/*
==================
CL_PK3List_f
==================
*/
void CL_OpenedPK3List_f(void)
{
	common->Printf("Opened PK3 Names: %s\n", FS_LoadedPakNames());
}

/*
==================
CL_PureList_f
==================
*/
void CL_ReferencedPK3List_f(void)
{
	common->Printf("Referenced PK3 Names: %s\n", FS_ReferencedPakNames());
}

/*
==================
CL_Configstrings_f
==================
*/
void CL_Configstrings_f(void)
{
	int i;
	int ofs;

	if (cls.state != CA_ACTIVE)
	{
		common->Printf("Not connected to a server.\n");
		return;
	}

	for (i = 0; i < MAX_CONFIGSTRINGS_Q3; i++)
	{
		ofs = cl.q3_gameState.stringOffsets[i];
		if (!ofs)
		{
			continue;
		}
		common->Printf("%4i: %s\n", i, cl.q3_gameState.stringData + ofs);
	}
}

/*
==============
CL_Clientinfo_f
==============
*/
void CL_Clientinfo_f(void)
{
	common->Printf("--------- Client Information ---------\n");
	common->Printf("state: %i\n", cls.state);
	common->Printf("Server: %s\n", cls.servername);
	common->Printf("User info settings:\n");
	Info_Print(Cvar_InfoString(CVAR_USERINFO, MAX_INFO_STRING_Q3));
	common->Printf("--------------------------------------\n");
}


//====================================================================

/*
=================
CLT3_PacketEvent

A packet has arrived from the main event loop
=================
*/
void CLT3_PacketEvent(netadr_t from, QMsg* msg)
{
	int headerBytes;

	clc.q3_lastPacketTime = cls.realtime;

	if (msg->cursize >= 4 && *(int*)msg->_data == -1)
	{
		CLT3_ConnectionlessPacket(from, msg);
		return;
	}

	if (cls.state < CA_CONNECTED)
	{
		return;		// can't be a valid sequenced packet
	}

	if (msg->cursize < 4)
	{
		common->Printf("%s: Runt packet\n",SOCK_AdrToString(from));
		return;
	}

	//
	// packet from server
	//
	if (!SOCK_CompareAdr(from, clc.netchan.remoteAddress))
	{
		common->DPrintf("%s:sequenced packet without connection\n",
			SOCK_AdrToString(from));
		// FIXME: send a client disconnect?
		return;
	}

	if (!CLT3_Netchan_Process(&clc.netchan, msg))
	{
		return;		// out of order, duplicated, etc
	}

	// the header is different lengths for reliable and unreliable messages
	headerBytes = msg->readcount;

	// track the last message received so it can be returned in
	// client messages, allowing the server to detect a dropped
	// gamestate
	clc.q3_serverMessageSequence = LittleLong(*(int*)msg->_data);

	clc.q3_lastPacketTime = cls.realtime;
	CLT3_ParseServerMessage(msg);

	//
	// we don't know if it is ok to save a demo message until
	// after we have parsed the frame
	//
	if (clc.demorecording && !clc.q3_demowaiting)
	{
		CLT3_WriteDemoMessage(msg, headerBytes);
	}
}

/*
==================
CL_CheckTimeout

==================
*/
void CL_CheckTimeout(void)
{
	//
	// check timeout
	//
	if ((!cl_paused->integer || !sv_paused->integer) &&
		cls.state >= CA_CONNECTED && cls.state != CA_CINEMATIC &&
		cls.realtime - clc.q3_lastPacketTime > cl_timeout->value * 1000)
	{
		if (++cl.timeoutcount > 5)		// timeoutcount saves debugger
		{
			common->Printf("\nServer connection timed out.\n");
			CL_Disconnect(true);
			return;
		}
	}
	else
	{
		cl.timeoutcount = 0;
	}
}


//============================================================================

/*
==================
CL_CheckUserinfo

==================
*/
void CL_CheckUserinfo(void)
{
	// don't add reliable commands when not yet connected
	if (cls.state < CA_CHALLENGING)
	{
		return;
	}
	// don't overflow the reliable command buffer when paused
	if (cl_paused->integer)
	{
		return;
	}
	// send a reliable userinfo update if needed
	if (cvar_modifiedFlags & CVAR_USERINFO)
	{
		cvar_modifiedFlags &= ~CVAR_USERINFO;
		CL_AddReliableCommand(va("userinfo \"%s\"", Cvar_InfoString(CVAR_USERINFO, MAX_INFO_STRING_Q3)));
	}

}

/*
==================
CL_Frame

==================
*/
void CL_Frame(int msec)
{

	if (!com_cl_running->integer)
	{
		return;
	}

	if (cls.state == CA_DISCONNECTED && !(in_keyCatchers & KEYCATCH_UI) &&
			 !com_sv_running->integer)
	{
		// if disconnected, bring up the menu
		S_StopAllSounds();
		UI_SetMainMenu();
	}

	// if recording an avi, lock to a fixed fps
	if (cl_avidemo->integer && msec)
	{
		// save the current screen
		if (cls.state == CA_ACTIVE || cl_forceavidemo->integer)
		{
			Cbuf_ExecuteText(EXEC_NOW, "screenshot silent\n");
		}
		// fixed time for next frame'
		msec = (1000 / cl_avidemo->integer) * com_timescale->value;
		if (msec == 0)
		{
			msec = 1;
		}
	}

	// save the msec before checking pause
	cls.realFrametime = msec;

	// decide the simulation time
	cls.frametime = msec;

	cls.realtime += cls.frametime;

	if (cl_timegraph->integer)
	{
		SCR_DebugGraph(cls.realFrametime * 0.25, 0);
	}

	// see if we need to update any userinfo
	CL_CheckUserinfo();

	// if we haven't gotten a packet in a long time,
	// drop the connection
	CL_CheckTimeout();

	// send intentions now
	CLT3_SendCmd();

	// resend a connection request if necessary
	CLT3_CheckForResend();

	// decide on the serverTime to render
	CL_SetCGameTime();

	// update the screen
	SCR_UpdateScreen();

	// update audio
	S_Update();

	// advance local effects for next frame
	SCR_RunCinematic();

	Con_RunConsole();

	cls.framecount++;
}


//============================================================================

/*
============
CL_ShutdownRef
============
*/
void CL_ShutdownRef()
{
	R_Shutdown(true);
}

/*
============
CL_InitRef
============
*/
void CL_InitRef(void)
{
	common->Printf("----- Initializing Renderer ----\n");

	BotDrawDebugPolygonsFunc = BotDrawDebugPolygons;

	common->Printf("-------------------------------\n");

	// unpause so the cgame definately gets a snapshot and renders a frame
	Cvar_Set("cl_paused", "0");
}


//===========================================================================================


void CL_SetModel_f(void)
{
	char* arg;
	char name[256];

	arg = Cmd_Argv(1);
	if (arg[0])
	{
		Cvar_Set("model", arg);
		Cvar_Set("headmodel", arg);
	}
	else
	{
		Cvar_VariableStringBuffer("model", name, sizeof(name));
		common->Printf("model is set to %s\n", name);
	}
}

/*
====================
CL_Init
====================
*/
void CL_Init(void)
{
	common->Printf("----- Client Initialization -----\n");

	CL_SharedInit();

	Con_Init();

	CL_ClearState();

	cls.state = CA_DISCONNECTED;	// no longer CA_UNINITIALIZED

	cls.realtime = 0;

	CL_InitInput();

	CLT3_InitServerLists();

	//
	// register our variables
	//
	clt3_motd = Cvar_Get("cl_motd", "1", 0);

	cl_timeout = Cvar_Get("cl_timeout", "200", 0);

	cl_timeNudge = Cvar_Get("cl_timeNudge", "0", CVAR_TEMP);
	clt3_showServerCommands = Cvar_Get("cl_showServerCommands", "0", 0);
	clt3_showSend = Cvar_Get("cl_showSend", "0", CVAR_TEMP);
	clt3_showTimeDelta = Cvar_Get("cl_showTimeDelta", "0", CVAR_TEMP);
	cl_freezeDemo = Cvar_Get("cl_freezeDemo", "0", CVAR_TEMP);
	rcon_client_password = Cvar_Get("rconPassword", "", CVAR_TEMP);
	clt3_activeAction = Cvar_Get("activeAction", "", CVAR_TEMP);

	cl_timedemo = Cvar_Get("timedemo", "0", 0);
	cl_avidemo = Cvar_Get("cl_avidemo", "0", 0);
	cl_forceavidemo = Cvar_Get("cl_forceavidemo", "0", 0);

	rconAddress = Cvar_Get("rconAddress", "", 0);

	clt3_maxpackets = Cvar_Get("cl_maxpackets", "30", CVAR_ARCHIVE);
	clt3_packetdup = Cvar_Get("cl_packetdup", "1", CVAR_ARCHIVE);

	clt3_allowDownload = Cvar_Get("cl_allowDownload", "0", CVAR_ARCHIVE);

	// init autoswitch so the ui will have it correctly even
	// if the cgame hasn't been started
	Cvar_Get("cg_autoswitch", "1", CVAR_ARCHIVE);

	cl_motdString = Cvar_Get("cl_motdString", "", CVAR_ROM);

	Cvar_Get("cl_maxPing", "800", CVAR_ARCHIVE);


	// userinfo
	Cvar_Get("name", "UnnamedPlayer", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("rate", "3000", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("snaps", "20", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("model", "sarge", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("headmodel", "sarge", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("team_model", "james", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("team_headmodel", "*james", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("g_redTeam", "Stroggs", CVAR_SERVERINFO | CVAR_ARCHIVE);
	Cvar_Get("g_blueTeam", "Pagans", CVAR_SERVERINFO | CVAR_ARCHIVE);
	Cvar_Get("color1",  "4", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("color2", "5", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("handicap", "100", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("teamtask", "0", CVAR_USERINFO);
	Cvar_Get("sex", "male", CVAR_USERINFO | CVAR_ARCHIVE);
	Cvar_Get("cl_anonymous", "0", CVAR_USERINFO | CVAR_ARCHIVE);

	Cvar_Get("password", "", CVAR_USERINFO);
	Cvar_Get("cg_predictItems", "1", CVAR_USERINFO | CVAR_ARCHIVE);


	// cgame might not be initialized before menu is used
	Cvar_Get("cg_viewsize", "100", CVAR_ARCHIVE);

	//
	// register our commands
	//
	Cmd_AddCommand("configstrings", CL_Configstrings_f);
	Cmd_AddCommand("clientinfo", CL_Clientinfo_f);
	Cmd_AddCommand("snd_restart", CL_Snd_Restart_f);
	Cmd_AddCommand("vid_restart", CL_Vid_Restart_f);
	Cmd_AddCommand("disconnect", CL_Disconnect_f);
	Cmd_AddCommand("record", CLT3_Record_f);
	Cmd_AddCommand("demo", CL_PlayDemo_f);
	Cmd_AddCommand("cinematic", CL_PlayCinematic_f);
	Cmd_AddCommand("stoprecord", CLT3_StopRecord_f);
	Cmd_AddCommand("connect", CLT3_Connect_f);
	Cmd_AddCommand("reconnect", CLT3_Reconnect_f);
	Cmd_AddCommand("rcon", CL_Rcon_f);
	Cmd_AddCommand("setenv", CL_Setenv_f);
	Cmd_AddCommand("showip", CL_ShowIP_f);
	Cmd_AddCommand("fs_openedList", CL_OpenedPK3List_f);
	Cmd_AddCommand("fs_referencedList", CL_ReferencedPK3List_f);
	Cmd_AddCommand("model", CL_SetModel_f);
	CL_InitRef();

	SCR_Init();

	Cbuf_Execute();

	Cvar_Set("cl_running", "1");

	common->Printf("----- Client Initialization Complete -----\n");
}


/*
===============
CL_Shutdown

===============
*/
void CL_Shutdown(void)
{
	static qboolean recursive = false;

	common->Printf("----- CL_Shutdown -----\n");

	if (recursive)
	{
		printf("recursive shutdown\n");
		return;
	}
	recursive = true;

	CL_Disconnect(true);

	S_Shutdown();
	CL_ShutdownRef();

	CLT3_ShutdownUI();

	Cmd_RemoveCommand("cmd");
	Cmd_RemoveCommand("configstrings");
	Cmd_RemoveCommand("userinfo");
	Cmd_RemoveCommand("snd_restart");
	Cmd_RemoveCommand("vid_restart");
	Cmd_RemoveCommand("disconnect");
	Cmd_RemoveCommand("record");
	Cmd_RemoveCommand("demo");
	Cmd_RemoveCommand("cinematic");
	Cmd_RemoveCommand("stoprecord");
	Cmd_RemoveCommand("connect");
	Cmd_RemoveCommand("localservers");
	Cmd_RemoveCommand("globalservers");
	Cmd_RemoveCommand("rcon");
	Cmd_RemoveCommand("setenv");
	Cmd_RemoveCommand("ping");
	Cmd_RemoveCommand("serverstatus");
	Cmd_RemoveCommand("showip");
	Cmd_RemoveCommand("model");

	Cvar_Set("cl_running", "0");

	recursive = false;

	Com_Memset(&cls, 0, sizeof(cls));

	common->Printf("-----------------------\n");

}

/*
==================
CL_ShowIP_f
==================
*/
void CL_ShowIP_f(void)
{
	SOCK_ShowIP();
}

float* CL_GetSimOrg()
{
	return NULL;
}
