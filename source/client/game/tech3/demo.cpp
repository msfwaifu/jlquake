//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "../../client.h"
#include "local.h"

/*
=======================================================================

CLIENT SIDE DEMO RECORDING

=======================================================================
*/

void CLT3_Record(const char* demoName, const char* name)
{
	int i;
	char* s;
	int len;

	// open the demo file
	common->Printf("recording to %s.\n", name);
	clc.demofile = FS_FOpenFileWrite(name);
	if (!clc.demofile)
	{
		common->Printf("ERROR: couldn't open.\n");
		return;
	}
	clc.demorecording = true;
	if (GGameType & GAME_Quake3)
	{
		if (Cvar_VariableValue("ui_recordSPDemo"))
		{
			clc.q3_spDemoRecording = true;
		}
		else
		{
			clc.q3_spDemoRecording = false;
		}
	}

	String::NCpyZ(clc.q3_demoName, demoName, sizeof(clc.q3_demoName));
	if (GGameType & GAME_ET)
	{
		Cvar_Set("cl_demorecording", "1");	// fretn
		Cvar_Set("cl_demofilename", clc.q3_demoName);	// bani
		Cvar_Set("cl_demooffset", "0");		// bani
	}

	// don't start saving messages until a non-delta compressed message is received
	clc.q3_demowaiting = true;

	// write out the gamestate message
	QMsg buf;
	byte bufData[MAX_MSGLEN];
	buf.Init(bufData, GGameType & GAME_Quake3 ? MAX_MSGLEN_Q3 : MAX_MSGLEN_WOLF);
	buf.Bitstream();

	// NOTE, MRE: all server->client messages now acknowledge
	buf.WriteLong(clc.q3_reliableSequence);

	buf.WriteByte(q3svc_gamestate);
	buf.WriteLong(clc.q3_serverCommandSequence);

	if (GGameType & GAME_Quake3)
	{
		// configstrings
		for (i = 0; i < MAX_CONFIGSTRINGS_Q3; i++)
		{
			if (!cl.q3_gameState.stringOffsets[i])
			{
				continue;
			}
			s = cl.q3_gameState.stringData + cl.q3_gameState.stringOffsets[i];
			buf.WriteByte(q3svc_configstring);
			buf.WriteShort(i);
			buf.WriteBigString(s);
		}

		// baselines
		q3entityState_t nullstate = {};
		for (i = 0; i < MAX_GENTITIES_Q3; i++)
		{
			q3entityState_t* ent = &cl.q3_entityBaselines[i];
			if (!ent->number)
			{
				continue;
			}
			buf.WriteByte(q3svc_baseline);
			MSGQ3_WriteDeltaEntity(&buf, &nullstate, ent, true);
		}
	}
	else if (GGameType & GAME_WolfSP)
	{
		// configstrings
		for (i = 0; i < MAX_CONFIGSTRINGS_WS; i++)
		{
			if (!cl.ws_gameState.stringOffsets[i])
			{
				continue;
			}
			s = cl.ws_gameState.stringData + cl.ws_gameState.stringOffsets[i];
			buf.WriteByte(q3svc_configstring);
			buf.WriteShort(i);
			buf.WriteBigString(s);
		}

		// baselines
		wsentityState_t nullstate = {};
		for (i = 0; i < MAX_GENTITIES_Q3; i++)
		{
			wsentityState_t* ent = &cl.ws_entityBaselines[i];
			if (!ent->number)
			{
				continue;
			}
			buf.WriteByte(q3svc_baseline);
			MSGWS_WriteDeltaEntity(&buf, &nullstate, ent, true);
		}
	}
	else if (GGameType & GAME_WolfMP)
	{
		// configstrings
		for (i = 0; i < MAX_CONFIGSTRINGS_WM; i++)
		{
			if (!cl.wm_gameState.stringOffsets[i])
			{
				continue;
			}
			s = cl.wm_gameState.stringData + cl.wm_gameState.stringOffsets[i];
			buf.WriteByte(q3svc_configstring);
			buf.WriteShort(i);
			buf.WriteBigString(s);
		}

		// baselines
		wmentityState_t nullstate = {};
		for (i = 0; i < MAX_GENTITIES_Q3; i++)
		{
			wmentityState_t* ent = &cl.wm_entityBaselines[i];
			if (!ent->number)
			{
				continue;
			}
			buf.WriteByte(q3svc_baseline);
			MSGWM_WriteDeltaEntity(&buf, &nullstate, ent, true);
		}
	}
	else
	{
		// configstrings
		for (i = 0; i < MAX_CONFIGSTRINGS_ET; i++)
		{
			if (!cl.et_gameState.stringOffsets[i])
			{
				continue;
			}
			s = cl.et_gameState.stringData + cl.et_gameState.stringOffsets[i];
			buf.WriteByte(q3svc_configstring);
			buf.WriteShort(i);
			buf.WriteBigString(s);
		}

		// baselines
		etentityState_t nullstate = {};
		for (i = 0; i < MAX_GENTITIES_Q3; i++)
		{
			etentityState_t* ent = &cl.et_entityBaselines[i];
			if (!ent->number)
			{
				continue;
			}
			buf.WriteByte(q3svc_baseline);
			MSGET_WriteDeltaEntity(&buf, &nullstate, ent, true);
		}
	}

	buf.WriteByte(q3svc_EOF);

	// finished writing the gamestate stuff

	// write the client num
	buf.WriteLong(clc.q3_clientNum);
	// write the checksum feed
	buf.WriteLong(clc.q3_checksumFeed);

	// finished writing the client packet
	buf.WriteByte(q3svc_EOF);

	// write it to the demo file
	len = LittleLong(clc.q3_serverMessageSequence - 1);
	FS_Write(&len, 4, clc.demofile);

	len = LittleLong(buf.cursize);
	FS_Write(&len, 4, clc.demofile);
	FS_Write(buf._data, buf.cursize, clc.demofile);

	// the rest of the demo file will be copied from net messages
}

//	Dumps the current net message, prefixed by the length
void CLT3_WriteDemoMessage(QMsg* msg, int headerBytes)
{
	// write the packet sequence
	int len = clc.q3_serverMessageSequence;
	int swlen = LittleLong(len);
	FS_Write(&swlen, 4, clc.demofile);

	// skip the packet sequencing information
	len = msg->cursize - headerBytes;
	swlen = LittleLong(len);
	FS_Write(&swlen, 4, clc.demofile);
	FS_Write(msg->_data + headerBytes, len, clc.demofile);
}

//	stop recording a demo
void CLT3_StopRecord_f()
{
	if (!clc.demorecording)
	{
		common->Printf("Not recording a demo.\n");
		return;
	}

	// finish up
	int len = -1;
	FS_Write(&len, 4, clc.demofile);
	FS_Write(&len, 4, clc.demofile);
	FS_FCloseFile(clc.demofile);
	clc.demofile = 0;
	clc.demorecording = false;
	if (GGameType & GAME_Quake3)
	{
		clc.q3_spDemoRecording = false;
	}
	if (GGameType & GAME_ET)
	{
		Cvar_Set("cl_demorecording", "0");
		Cvar_Set("cl_demofilename", "");
		Cvar_Set("cl_demooffset", "0");
	}
	common->Printf("Stopped demo.\n");
}

static void CLT3_DemoFilename(int number, char* fileName)
{
	if (number < 0 || number > 9999)
	{
		String::Sprintf(fileName, MAX_QPATH, "demo9999");
		return;
	}

	String::Sprintf(fileName, MAX_QPATH, "demo%04i", number);
}

//	Begins recording a demo from the current position
void CLT3_Record_f()
{
	if (Cmd_Argc() > 2)
	{
		common->Printf("record <demoname>\n");
		return;
	}

	if (clc.demorecording)
	{
		if (!(GGameType & GAME_Quake3) || !clc.q3_spDemoRecording)
		{
			common->Printf("Already recording.\n");
		}
		return;
	}

	if (cls.state != CA_ACTIVE)
	{
		common->Printf("You must be in a level to record.\n");
		return;
	}

	// sync 0 doesn't prevent recording, so not forcing it off .. everyone does g_sync 1 ; record ; g_sync 0 ..
	if (!Cvar_VariableValue("g_synchronousClients"))
	{
		common->Printf(S_COLOR_YELLOW "WARNING: You should set 'g_synchronousClients 1' for smoother demo recording\n");
	}

	int protocolVersion = GGameType & GAME_WolfSP ? WSPROTOCOL_VERSION :
		GGameType & GAME_WolfMP ? WMPROTOCOL_VERSION :
		GGameType & GAME_ET ? ETPROTOCOL_VERSION : Q3PROTOCOL_VERSION;
	char demoName[MAX_QPATH];
	char name[MAX_OSPATH];
	if (Cmd_Argc() == 2)
	{
		const char* s = Cmd_Argv(1);
		String::NCpyZ(demoName, s, sizeof(demoName));
		String::Sprintf(name, sizeof(name), "demos/%s.dm_%d", demoName, protocolVersion);
	}
	else
	{
		// scan for a free demo name
		for (int number = 0; number <= 9999; number++)
		{
			CLT3_DemoFilename(number, demoName);
			String::Sprintf(name, sizeof(name), "demos/%s.dm_%d", demoName, protocolVersion);

			int len = FS_ReadFile(name, NULL);
			if (len <= 0)
			{
				break;	// file doesn't exist
			}
		}
	}

	CLT3_Record(demoName, name);
}

/*
=======================================================================

CLIENT SIDE DEMO PLAYBACK

=======================================================================
*/

void CLT3_DemoCompleted()
{
	if (cl_timedemo && cl_timedemo->integer)
	{
		int time = Sys_Milliseconds() - clc.q3_timeDemoStart;
		if (time > 0)
		{
			common->Printf("%i frames, %3.1f seconds: %3.1f fps\n", clc.q3_timeDemoFrames,
				time / 1000.0, clc.q3_timeDemoFrames * 1000.0 / time);
		}
	}

	// fretn
	if (GGameType & GAME_ET && clc.wm_waverecording)
	{
		CL_WriteWaveClose();
		clc.wm_waverecording = false;
	}

	CL_Disconnect(true);
	CL_NextDemo();
}