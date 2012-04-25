/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the included (GNU.txt) GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "quakedef.h"

void CL_FinishTimeDemo(void);

/*
==============================================================================

DEMO CODE

When a demo is playing back, all NET_SendMessages are skipped, and
NET_GetMessages are read from the demo file.

Whenever cl.time gets past the last received message, another message is
read from the demo file.
==============================================================================
*/

/*
==============
CL_StopPlayback

Called when a demo file runs out, or the user starts a game
==============
*/
void CL_StopPlayback(void)
{
	if (!clc.demoplaying)
	{
		return;
	}

	FS_FCloseFile(clc.demofile);
	clc.demofile = 0;
	cls.state = CA_DISCONNECTED;
	clc.demoplaying = 0;

	if (cls.qh_timedemo)
	{
		CL_FinishTimeDemo();
	}
}

#define dem_cmd     0
#define dem_read    1
#define dem_set     2

/*
====================
CL_WriteDemoCmd

Writes the current user cmd
====================
*/
void CL_WriteDemoCmd(qwusercmd_t* pcmd)
{
	int i;
	float fl;
	byte c;
	qwusercmd_t cmd;

//Con_Printf("write: %ld bytes, %4.4f\n", msg->cursize, realtime);

	fl = LittleFloat((float)realtime);
	FS_Write(&fl, sizeof(fl), clc.demofile);

	c = dem_cmd;
	FS_Write(&c, sizeof(c), clc.demofile);

	// correct for byte order, bytes don't matter
	cmd = *pcmd;

	for (i = 0; i < 3; i++)
		cmd.angles[i] = LittleFloat(cmd.angles[i]);
	cmd.forwardmove = LittleShort(cmd.forwardmove);
	cmd.sidemove    = LittleShort(cmd.sidemove);
	cmd.upmove      = LittleShort(cmd.upmove);

	FS_Write(&cmd, sizeof(cmd), clc.demofile);

	for (i = 0; i < 3; i++)
	{
		fl = LittleFloat(cl.viewangles[i]);
		FS_Write(&fl, 4, clc.demofile);
	}

	FS_Flush(clc.demofile);
}

/*
====================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length and view angles
====================
*/
void CL_WriteDemoMessage(QMsg* msg)
{
	int len;
	float fl;
	byte c;

//Con_Printf("write: %ld bytes, %4.4f\n", msg->cursize, realtime);

	if (!clc.demorecording)
	{
		return;
	}

	fl = LittleFloat((float)realtime);
	FS_Write(&fl, sizeof(fl), clc.demofile);

	c = dem_read;
	FS_Write(&c, sizeof(c), clc.demofile);

	len = LittleLong(msg->cursize);
	FS_Write(&len, 4, clc.demofile);
	FS_Write(msg->_data, msg->cursize, clc.demofile);

	FS_Flush(clc.demofile);
}

/*
====================
CL_GetDemoMessage

  FIXME...
====================
*/
qboolean CL_GetDemoMessage(void)
{
	int r, i, j;
	float f;
	float demotime;
	byte c;
	qwusercmd_t* pcmd;

	// read the time from the packet
	FS_Read(&demotime, sizeof(demotime), clc.demofile);
	demotime = LittleFloat(demotime);

// decide if it is time to grab the next message
	if (cls.qh_timedemo)
	{
		if (cls.qh_td_lastframe < 0)
		{
			cls.qh_td_lastframe = demotime;
		}
		else if (demotime > cls.qh_td_lastframe)
		{
			cls.qh_td_lastframe = demotime;
			// rewind back to time
			FS_Seek(clc.demofile, FS_FTell(clc.demofile) - sizeof(demotime),
				FS_SEEK_SET);
			return 0;		// allready read this frame's message
		}
		if (!cls.qh_td_starttime && cls.state == CA_ACTIVE)
		{
			cls.qh_td_starttime = Sys_DoubleTime();
			cls.qh_td_startframe = host_framecount;
		}
		realtime = demotime;// warp
	}
	else if (!cl.qh_paused && cls.state >= CA_LOADING)		// allways grab until fully connected
	{
		if (realtime + 1.0 < demotime)
		{
			// too far back
			realtime = demotime - 1.0;
			// rewind back to time
			FS_Seek(clc.demofile, FS_FTell(clc.demofile) - sizeof(demotime),
				FS_SEEK_SET);
			return 0;
		}
		else if (realtime < demotime)
		{
			// rewind back to time
			FS_Seek(clc.demofile, FS_FTell(clc.demofile) - sizeof(demotime),
				FS_SEEK_SET);
			return 0;		// don't need another message yet
		}
	}
	else
	{
		realtime = demotime;// we're warping

	}
	if (cls.state == CA_DISCONNECTED)
	{
		Host_FatalError("CL_GetDemoMessage: cls.state != ca_active");
	}

	// get the msg type
	FS_Read(&c, sizeof(c), clc.demofile);

	switch (c)
	{
	case dem_cmd:
		// user sent input
		i = clc.netchan.outgoingSequence & UPDATE_MASK_QW;
		pcmd = &cl.qw_frames[i].cmd;
		r = FS_Read(pcmd, sizeof(*pcmd), clc.demofile);
		if (r != sizeof(*pcmd))
		{
			CL_StopPlayback();
			return 0;
		}
		// byte order stuff
		for (j = 0; j < 3; j++)
			pcmd->angles[j] = LittleFloat(pcmd->angles[j]);
		pcmd->forwardmove = LittleShort(pcmd->forwardmove);
		pcmd->sidemove    = LittleShort(pcmd->sidemove);
		pcmd->upmove      = LittleShort(pcmd->upmove);
		cl.qw_frames[i].senttime = demotime;
		cl.qw_frames[i].receivedtime = -1;		// we haven't gotten a reply yet
		clc.netchan.outgoingSequence++;
		for (i = 0; i < 3; i++)
		{
			r = FS_Read(&f, 4, clc.demofile);
			cl.viewangles[i] = LittleFloat(f);
		}
		break;

	case dem_read:
		// get the next message
		FS_Read(&net_message.cursize, 4, clc.demofile);
		net_message.cursize = LittleLong(net_message.cursize);
		//Con_Printf("read: %ld bytes\n", net_message.cursize);
		if (net_message.cursize > MAX_MSGLEN_QW)
		{
			Sys_Error("Demo message > MAX_MSGLEN_QW");
		}
		r = FS_Read(net_message._data, net_message.cursize, clc.demofile);
		if (r != net_message.cursize)
		{
			CL_StopPlayback();
			return 0;
		}
		break;

	case dem_set:
		FS_Read(&i, 4, clc.demofile);
		clc.netchan.outgoingSequence = LittleLong(i);
		FS_Read(&i, 4, clc.demofile);
		clc.netchan.incomingSequence = LittleLong(i);
		break;

	default:
		Con_Printf("Corrupted demo.\n");
		CL_StopPlayback();
		return 0;
	}

	return 1;
}

/*
====================
CL_GetMessage

Handles recording and playback of demos, on top of NET_ code
====================
*/
qboolean CL_GetMessage(void)
{
	if  (clc.demoplaying)
	{
		return CL_GetDemoMessage();
	}

	if (!NET_GetPacket())
	{
		return false;
	}

	CL_WriteDemoMessage(&net_message);

	return true;
}


/*
====================
CL_Stop_f

stop recording a demo
====================
*/
void CL_Stop_f(void)
{
	if (!clc.demorecording)
	{
		Con_Printf("Not recording a demo.\n");
		return;
	}

// write a disconnect message to the demo file
	net_message.Clear();
	net_message.WriteLong(-1);	// -1 sequence means out of band
	net_message.WriteByte(q1svc_disconnect);
	net_message.WriteString2("EndOfDemo");
	CL_WriteDemoMessage(&net_message);

// finish up
	FS_FCloseFile(clc.demofile);
	clc.demofile = 0;
	clc.demorecording = false;
	Con_Printf("Completed demo\n");
}


/*
====================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length and view angles
====================
*/
void CL_WriteRecordDemoMessage(QMsg* msg, int seq)
{
	int len;
	int i;
	float fl;
	byte c;

//Con_Printf("write: %ld bytes, %4.4f\n", msg->cursize, realtime);

	if (!clc.demorecording)
	{
		return;
	}

	fl = LittleFloat((float)realtime);
	FS_Write(&fl, sizeof(fl), clc.demofile);

	c = dem_read;
	FS_Write(&c, sizeof(c), clc.demofile);

	len = LittleLong(msg->cursize + 8);
	FS_Write(&len, 4, clc.demofile);

	i = LittleLong(seq);
	FS_Write(&i, 4, clc.demofile);
	FS_Write(&i, 4, clc.demofile);

	FS_Write(msg->_data, msg->cursize, clc.demofile);

	FS_Flush(clc.demofile);
}


void CL_WriteSetDemoMessage(void)
{
	int len;
	float fl;
	byte c;

//Con_Printf("write: %ld bytes, %4.4f\n", msg->cursize, realtime);

	if (!clc.demorecording)
	{
		return;
	}

	fl = LittleFloat((float)realtime);
	FS_Write(&fl, sizeof(fl), clc.demofile);

	c = dem_set;
	FS_Write(&c, sizeof(c), clc.demofile);

	len = LittleLong(clc.netchan.outgoingSequence);
	FS_Write(&len, 4, clc.demofile);
	len = LittleLong(clc.netchan.incomingSequence);
	FS_Write(&len, 4, clc.demofile);

	FS_Flush(clc.demofile);
}




/*
====================
CL_Record_f

record <demoname> <server>
====================
*/
void CL_Record_f(void)
{
	int c;
	char name[MAX_OSPATH];
	QMsg buf;
	byte buf_data[MAX_MSGLEN_QW];
	int n, i, j;
	char* s;
	q1entity_t* ent;
	q1entity_state_t* es, blankes;
	q1player_info_t* player;
	extern char gamedirfile[];
	int seq = 1;

	c = Cmd_Argc();
	if (c != 2)
	{
		Con_Printf("record <demoname>\n");
		return;
	}

	if (cls.state != CA_ACTIVE)
	{
		Con_Printf("You must be connected to record.\n");
		return;
	}

	if (clc.demorecording)
	{
		CL_Stop_f();
	}

	String::Cpy(name, Cmd_Argv(1));

//
// open the demo file
//
	String::DefaultExtension(name, sizeof(name), ".qwd");

	clc.demofile = FS_FOpenFileWrite(name);
	if (!clc.demofile)
	{
		Con_Printf("ERROR: couldn't open.\n");
		return;
	}

	Con_Printf("recording to %s.\n", name);
	clc.demorecording = true;

/*-------------------------------------------------*/

// serverdata
	// send the info about the new client to all connected clients
	buf.InitOOB(buf_data, sizeof(buf_data));

// send the serverdata
	buf.WriteByte(qwsvc_serverdata);
	buf.WriteLong(PROTOCOL_VERSION);
	buf.WriteLong(cl.servercount);
	buf.WriteString2(gamedirfile);

	if (cl.qh_spectator)
	{
		buf.WriteByte(cl.playernum | 128);
	}
	else
	{
		buf.WriteByte(cl.playernum);
	}

	// send full levelname
	buf.WriteString2(cl.qh_levelname);

	// send the movevars
	buf.WriteFloat(movevars.gravity);
	buf.WriteFloat(movevars.stopspeed);
	buf.WriteFloat(movevars.maxspeed);
	buf.WriteFloat(movevars.spectatormaxspeed);
	buf.WriteFloat(movevars.accelerate);
	buf.WriteFloat(movevars.airaccelerate);
	buf.WriteFloat(movevars.wateraccelerate);
	buf.WriteFloat(movevars.friction);
	buf.WriteFloat(movevars.waterfriction);
	buf.WriteFloat(movevars.entgravity);

	// send music
	buf.WriteByte(q1svc_cdtrack);
	buf.WriteByte(0);	// none in demos

	// send server info string
	buf.WriteByte(q1svc_stufftext);
	buf.WriteString2(va("fullserverinfo \"%s\"\n", cl.qh_serverinfo));

	// flush packet
	CL_WriteRecordDemoMessage(&buf, seq++);
	buf.Clear();

// soundlist
	buf.WriteByte(qwsvc_soundlist);
	buf.WriteByte(0);

	n = 0;
	s = cl.qh_sound_name[n + 1];
	while (*s)
	{
		buf.WriteString2(s);
		if (buf.cursize > MAX_MSGLEN_QW / 2)
		{
			buf.WriteByte(0);
			buf.WriteByte(n);
			CL_WriteRecordDemoMessage(&buf, seq++);
			buf.Clear();
			buf.WriteByte(qwsvc_soundlist);
			buf.WriteByte(n + 1);
		}
		n++;
		s = cl.qh_sound_name[n + 1];
	}
	if (buf.cursize)
	{
		buf.WriteByte(0);
		buf.WriteByte(0);
		CL_WriteRecordDemoMessage(&buf, seq++);
		buf.Clear();
	}

// modellist
	buf.WriteByte(qwsvc_modellist);
	buf.WriteByte(0);

	n = 0;
	s = cl.qh_model_name[n + 1];
	while (*s)
	{
		buf.WriteString2(s);
		if (buf.cursize > MAX_MSGLEN_QW / 2)
		{
			buf.WriteByte(0);
			buf.WriteByte(n);
			CL_WriteRecordDemoMessage(&buf, seq++);
			buf.Clear();
			buf.WriteByte(qwsvc_modellist);
			buf.WriteByte(n + 1);
		}
		n++;
		s = cl.qh_model_name[n + 1];
	}
	if (buf.cursize)
	{
		buf.WriteByte(0);
		buf.WriteByte(0);
		CL_WriteRecordDemoMessage(&buf, seq++);
		buf.Clear();
	}

// spawnstatic

	for (i = 0; i < cl.qh_num_statics; i++)
	{
		ent = clq1_static_entities + i;

		buf.WriteByte(q1svc_spawnstatic);

		buf.WriteByte(ent->state.modelindex);
		buf.WriteByte(ent->state.frame);
		buf.WriteByte(0);
		buf.WriteByte(ent->state.skinnum);
		for (j = 0; j < 3; j++)
		{
			buf.WriteCoord(ent->state.origin[j]);
			buf.WriteAngle(ent->state.angles[j]);
		}

		if (buf.cursize > MAX_MSGLEN_QW / 2)
		{
			CL_WriteRecordDemoMessage(&buf, seq++);
			buf.Clear();
		}
	}

// spawnstaticsound
	// static sounds are skipped in demos, life is hard

// baselines

	Com_Memset(&blankes, 0, sizeof(blankes));
	for (i = 0; i < MAX_EDICTS_Q1; i++)
	{
		es = clq1_baselines + i;

		if (memcmp(es, &blankes, sizeof(blankes)))
		{
			buf.WriteByte(q1svc_spawnbaseline);
			buf.WriteShort(i);

			buf.WriteByte(es->modelindex);
			buf.WriteByte(es->frame);
			buf.WriteByte(es->colormap);
			buf.WriteByte(es->skinnum);
			for (j = 0; j < 3; j++)
			{
				buf.WriteCoord(es->origin[j]);
				buf.WriteAngle(es->angles[j]);
			}

			if (buf.cursize > MAX_MSGLEN_QW / 2)
			{
				CL_WriteRecordDemoMessage(&buf, seq++);
				buf.Clear();
			}
		}
	}

	buf.WriteByte(q1svc_stufftext);
	buf.WriteString2(va("cmd spawn %i 0\n", cl.servercount));

	if (buf.cursize)
	{
		CL_WriteRecordDemoMessage(&buf, seq++);
		buf.Clear();
	}

// send current status of all other players

	for (i = 0; i < MAX_CLIENTS_QW; i++)
	{
		player = cl.q1_players + i;

		buf.WriteByte(q1svc_updatefrags);
		buf.WriteByte(i);
		buf.WriteShort(player->frags);

		buf.WriteByte(qwsvc_updateping);
		buf.WriteByte(i);
		buf.WriteShort(player->ping);

		buf.WriteByte(qwsvc_updatepl);
		buf.WriteByte(i);
		buf.WriteByte(player->pl);

		buf.WriteByte(qwsvc_updateentertime);
		buf.WriteByte(i);
		buf.WriteFloat(player->entertime);

		buf.WriteByte(qwsvc_updateuserinfo);
		buf.WriteByte(i);
		buf.WriteLong(player->userid);
		buf.WriteString2(player->userinfo);

		if (buf.cursize > MAX_MSGLEN_QW / 2)
		{
			CL_WriteRecordDemoMessage(&buf, seq++);
			buf.Clear();
		}
	}

// send all current light styles
	for (i = 0; i < MAX_LIGHTSTYLES_Q1; i++)
	{
		buf.WriteByte(q1svc_lightstyle);
		buf.WriteByte((char)i);
		buf.WriteString2(cl_lightstyle[i].mapStr);
	}

	for (i = 0; i < MAX_CL_STATS; i++)
	{
		buf.WriteByte(qwsvc_updatestatlong);
		buf.WriteByte(i);
		buf.WriteLong(cl.qh_stats[i]);
		if (buf.cursize > MAX_MSGLEN_QW / 2)
		{
			CL_WriteRecordDemoMessage(&buf, seq++);
			buf.Clear();
		}
	}

#if 0
	buf.WriteByte(qwsvc_updatestatlong);
	buf.WriteByte(STAT_TOTALMONSTERS);
	buf.WriteLong(cl.stats[STAT_TOTALMONSTERS]);

	buf.WriteByte(qwsvc_updatestatlong);
	buf.WriteByte(STAT_SECRETS);
	buf.WriteLong(cl.stats[STAT_SECRETS]);

	buf.WriteByte(qwsvc_updatestatlong);
	buf.WriteByte(STAT_MONSTERS);
	buf.WriteLong(cl.stats[STAT_MONSTERS]);
#endif

	// get the client to check and download skins
	// when that is completed, a begin command will be issued
	buf.WriteByte(q1svc_stufftext);
	buf.WriteString2(va("skins\n"));

	CL_WriteRecordDemoMessage(&buf, seq++);

	CL_WriteSetDemoMessage();

	// done
}

/*
====================
CL_ReRecord_f

record <demoname>
====================
*/
void CL_ReRecord_f(void)
{
	int c;
	char name[MAX_OSPATH];

	c = Cmd_Argc();
	if (c != 2)
	{
		Con_Printf("rerecord <demoname>\n");
		return;
	}

	if (!*cls.servername)
	{
		Con_Printf("No server to reconnect to...\n");
		return;
	}

	if (clc.demorecording)
	{
		CL_Stop_f();
	}

	String::Cpy(name, Cmd_Argv(1));

//
// open the demo file
//
	String::DefaultExtension(name, sizeof(name), ".qwd");

	clc.demofile = FS_FOpenFileWrite(name);
	if (!clc.demofile)
	{
		Con_Printf("ERROR: couldn't open.\n");
		return;
	}

	Con_Printf("recording to %s.\n", name);
	clc.demorecording = true;

	CL_Disconnect();
	CL_BeginServerConnect();
}


/*
====================
CL_PlayDemo_f

play [demoname]
====================
*/
void CL_PlayDemo_f(void)
{
	char name[256];

	if (Cmd_Argc() != 2)
	{
		Con_Printf("play <demoname> : plays a demo\n");
		return;
	}

//
// disconnect from server
//
	CL_Disconnect();

//
// open the demo file
//
	String::Cpy(name, Cmd_Argv(1));
	String::DefaultExtension(name, sizeof(name), ".qwd");

	Con_Printf("Playing demo from %s.\n", name);
	FS_FOpenFileRead(name, &clc.demofile, true);
	if (!clc.demofile)
	{
		Con_Printf("ERROR: couldn't open.\n");
		cls.qh_demonum = -1;		// stop demo loop
		return;
	}

	clc.demoplaying = true;
	cls.state = CA_DEMOSTART;
	Netchan_Setup(NS_CLIENT, &clc.netchan, net_from, 0);
	realtime = 0;
}

/*
====================
CL_FinishTimeDemo

====================
*/
void CL_FinishTimeDemo(void)
{
	int frames;
	float time;

	cls.qh_timedemo = false;

// the first frame didn't count
	frames = (host_framecount - cls.qh_td_startframe) - 1;
	time = Sys_DoubleTime() - cls.qh_td_starttime;
	if (!time)
	{
		time = 1;
	}
	Con_Printf("%i frames %5.1f seconds %5.1f fps\n", frames, time, frames / time);
}

/*
====================
CL_TimeDemo_f

timedemo [demoname]
====================
*/
void CL_TimeDemo_f(void)
{
	if (Cmd_Argc() != 2)
	{
		Con_Printf("timedemo <demoname> : gets demo speeds\n");
		return;
	}

	CL_PlayDemo_f();

	if (cls.state != CA_DEMOSTART)
	{
		return;
	}

// cls.td_starttime will be grabbed at the second frame of the demo, so
// all the loading time doesn't get counted

	cls.qh_timedemo = true;
	cls.qh_td_starttime = 0;
	cls.qh_td_startframe = host_framecount;
	cls.qh_td_lastframe = -1;		// get a new message this frame
}
