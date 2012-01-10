/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// cl_main.c  -- client main loop

#include "quakedef.h"

// we need to declare some mouse variables here, because the menu system
// references them even when on a unix system.

Cvar*	cl_shownet;
Cvar*	cl_nolerp;

Cvar*	lookspring;
Cvar*	lookstrafe;
Cvar*	sensitivity;

Cvar*	m_pitch;
Cvar*	m_yaw;
Cvar*	m_forward;
Cvar*	m_side;

client_static_t	cls;
clientConnection_t clc;
client_state_t	cl;

/*
=====================
CL_ClearState

=====================
*/
void CL_ClearState (void)
{
	if (!sv.active)
		Host_ClearMemory ();

// wipe the entire cl structure
	Com_Memset(&cl, 0, sizeof(cl));

	cls.message.Clear();

// clear other arrays	
	Com_Memset(clq1_entities, 0, sizeof(clq1_entities));
	Com_Memset(clq1_baselines, 0, sizeof(clq1_baselines));
	CL_ClearDlights();
	CL_ClearLightStyles();
	CLQ1_ClearTEnts();
}

/*
=====================
CL_Disconnect

Sends a disconnect message to the server
This is also called on Host_Error, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect (void)
{
// stop sounds (especially looping!)
	S_StopAllSounds();
	
// bring the console down and fade the colors back to normal
//	SCR_BringDownConsole ();

// if running a local server, shut it down
	if (cls.demoplayback)
		CL_StopPlayback ();
	else if (cls.state == ca_connected)
	{
		if (cls.demorecording)
			CL_Stop_f ();

		Con_DPrintf ("Sending q1clc_disconnect\n");
		cls.message.Clear();
		cls.message.WriteByte(q1clc_disconnect);
		NET_SendUnreliableMessage (cls.netcon, &clc.netchan, &cls.message);
		cls.message.Clear();
		NET_Close (cls.netcon, &clc.netchan);

		cls.state = ca_disconnected;
		if (sv.active)
			Host_ShutdownServer(false);
	}

	cls.demoplayback = cls.timedemo = false;
	cls.signon = 0;
}

void CL_Disconnect_f (void)
{
	CL_Disconnect ();
	if (sv.active)
		Host_ShutdownServer (false);
}




/*
=====================
CL_EstablishConnection

Host should be either "local" or a net address to be passed on
=====================
*/
void CL_EstablishConnection (const char *host)
{
	if (cls.state == ca_dedicated)
		return;

	if (cls.demoplayback)
		return;

	CL_Disconnect ();

	Com_Memset(&clc.netchan, 0, sizeof(clc.netchan));
	clc.netchan.sock = NS_CLIENT;
	cls.netcon = NET_Connect (host, &clc.netchan);
	if (!cls.netcon)
		Host_Error ("CL_Connect: connect failed\n");
	clc.netchan.lastReceived = net_time * 1000;
	Con_DPrintf ("CL_EstablishConnection: connected to %s\n", host);
	
	cls.demonum = -1;			// not in the demo loop now
	cls.state = ca_connected;
	cls.signon = 0;				// need all the signon messages before playing
}

/*
=====================
CL_SignonReply

An q1svc_signonnum has been received, perform a client side setup
=====================
*/
void CL_SignonReply (void)
{
	char 	str[8192];

Con_DPrintf ("CL_SignonReply: %i\n", cls.signon);

	switch (cls.signon)
	{
	case 1:
		cls.message.WriteByte(q1clc_stringcmd);
		cls.message.WriteString2("prespawn");
		break;
		
	case 2:		
		cls.message.WriteByte(q1clc_stringcmd);
		cls.message.WriteString2(va("name \"%s\"\n", clqh_name->string));
	
		cls.message.WriteByte(q1clc_stringcmd);
		cls.message.WriteString2(va("color %i %i\n", ((int)clqh_color->value)>>4, ((int)clqh_color->value)&15));
	
		cls.message.WriteByte(q1clc_stringcmd);
		sprintf (str, "spawn %s", cls.spawnparms);
		cls.message.WriteString2(str);
		break;
		
	case 3:	
		cls.message.WriteByte(q1clc_stringcmd);
		cls.message.WriteString2("begin");
		break;
		
	case 4:
		SCR_EndLoadingPlaque ();		// allow normal screen updates
		break;
	}
}

/*
=====================
CL_NextDemo

Called to play the next demo in the demo loop
=====================
*/
void CL_NextDemo (void)
{
	char	str[1024];

	if (cls.demonum == -1)
		return;		// don't play demos

	SCR_BeginLoadingPlaque ();

	if (!cls.demos[cls.demonum][0] || cls.demonum == MAX_DEMOS)
	{
		cls.demonum = 0;
		if (!cls.demos[cls.demonum][0])
		{
			Con_Printf ("No demos listed with startdemos\n");
			cls.demonum = -1;
			return;
		}
	}

	sprintf (str,"playdemo %s\n", cls.demos[cls.demonum]);
	Cbuf_InsertText (str);
	cls.demonum++;
}

/*
==============
CL_PrintEntities_f
==============
*/
void CL_PrintEntities_f (void)
{
	q1entity_t	*ent;
	int			i;
	
	for (i=0,ent=clq1_entities ; i<cl.qh_num_entities ; i++,ent++)
	{
		Con_Printf ("%3i:",i);
		if (!ent->state.modelindex)
		{
			Con_Printf ("EMPTY\n");
			continue;
		}
		Con_Printf ("%s:%2i  (%5.1f,%5.1f,%5.1f) [%5.1f %5.1f %5.1f]\n"
		,R_ModelName(cl.model_precache[ent->state.modelindex]),ent->state.frame, ent->state.origin[0], ent->state.origin[1], ent->state.origin[2], ent->state.angles[0], ent->state.angles[1], ent->state.angles[2]);
	}
}

/*
===============
CL_LerpPoint

Determines the fraction between the last two messages that the objects
should be put at.
===============
*/
float	CL_LerpPoint (void)
{
	float	f, frac;

	f = cl.mtime[0] - cl.mtime[1];
	
	if (!f || cl_nolerp->value || cls.timedemo || sv.active)
	{
		cl.serverTimeFloat = cl.mtime[0];
		cl.serverTime = (int)(cl.serverTimeFloat * 1000);
		return 1;
	}
		
	if (f > 0.1)
	{	// dropped packet, or start of demo
		cl.mtime[1] = cl.mtime[0] - 0.1;
		f = 0.1;
	}
	frac = (cl.serverTimeFloat - cl.mtime[1]) / f;
//Con_Printf ("frac: %f\n",frac);
	if (frac < 0)
	{
		if (frac < -0.01)
		{
			cl.serverTimeFloat = cl.mtime[1];
			cl.serverTime = (int)(cl.serverTimeFloat * 1000);
//				Con_Printf ("low frac\n");
		}
		frac = 0;
	}
	else if (frac > 1)
	{
		if (frac > 1.01)
		{
			cl.serverTimeFloat = cl.mtime[0];
			cl.serverTime = (int)(cl.serverTimeFloat * 1000);
//				Con_Printf ("high frac\n");
		}
		frac = 1;
	}
		
	return frac;
}

static void R_HandleRefEntColormap(refEntity_t* Ent, int ColorMap)
{
	// we can't dynamically colormap textures, so they are cached
	// seperately for the players.  Heads are just uncolored.
	if (ColorMap && !String::Cmp(R_ModelName(Ent->hModel), "progs/player.mdl"))
	{
	    Ent->customSkin = R_GetImageHandle(playertextures[ColorMap - 1]);
	}
}

/*
===============
CL_RelinkEntities
===============
*/
void CL_RelinkEntities (void)
{
	q1entity_t	*ent;
	int			i, j;
	float		frac, f, d;
	vec3_t		delta;
	float		bobjrotate;
	vec3_t		oldorg;

// determine partial update time	
	frac = CL_LerpPoint ();

	R_ClearScene();

//
// interpolate player info
//
	for (i=0 ; i<3 ; i++)
		cl.velocity[i] = cl.mvelocity[1][i] + 
			frac * (cl.mvelocity[0][i] - cl.mvelocity[1][i]);

	if (cls.demoplayback)
	{
	// interpolate the angles	
		for (j=0 ; j<3 ; j++)
		{
			d = cl.mviewangles[0][j] - cl.mviewangles[1][j];
			if (d > 180)
				d -= 360;
			else if (d < -180)
				d += 360;
			cl.viewangles[j] = cl.mviewangles[1][j] + frac*d;
		}
	}
	
	bobjrotate = AngleMod(100*cl.serverTimeFloat);
	
// start on the entity after the world
	for (i=1,ent=clq1_entities+1 ; i<cl.qh_num_entities ; i++,ent++)
	{
		if (!ent->state.modelindex)
		{
			// empty slot
			continue;
		}

// if the object wasn't included in the last packet, remove it
		if (ent->msgtime != cl.mtime[0])
		{
			ent->state.modelindex = 0;
			continue;
		}

		VectorCopy (ent->state.origin, oldorg);

		// if the delta is large, assume a teleport and don't lerp
		f = frac;
		for (j=0 ; j<3 ; j++)
		{
			delta[j] = ent->msg_origins[0][j] - ent->msg_origins[1][j];
			if (delta[j] > 100 || delta[j] < -100)
				f = 1;		// assume a teleportation, not a motion
		}

	// interpolate the origin and angles
		for (j=0 ; j<3 ; j++)
		{
			ent->state.origin[j] = ent->msg_origins[1][j] + f*delta[j];

			d = ent->msg_angles[0][j] - ent->msg_angles[1][j];
			if (d > 180)
				d -= 360;
			else if (d < -180)
				d += 360;
			ent->state.angles[j] = ent->msg_angles[1][j] + f*d;
		}
		

		int ModelFlags = R_ModelFlags(cl.model_precache[ent->state.modelindex]);
// rotate binary objects locally
		if (ModelFlags & Q1MDLEF_ROTATE)
			ent->state.angles[1] = bobjrotate;

		if (ent->state.effects & Q1EF_BRIGHTFIELD)
			CLQ1_BrightFieldParticles(ent->state.origin);
		if (ent->state.effects & Q1EF_MUZZLEFLASH)
		{
			CLQ1_MuzzleFlashLight(i, ent->state.origin, ent->state.angles);
		}
		if (ent->state.effects & Q1EF_BRIGHTLIGHT)
		{
			CLQ1_BrightLight(i, ent->state.origin);
		}
		if (ent->state.effects & Q1EF_DIMLIGHT)
		{			
			CLQ1_DimLight(i, ent->state.origin, 0);
		}

		if (ModelFlags & Q1MDLEF_GIB)
			CLQ1_TrailParticles (oldorg, ent->state.origin, 2);
		else if (ModelFlags & Q1MDLEF_ZOMGIB)
			CLQ1_TrailParticles (oldorg, ent->state.origin, 4);
		else if (ModelFlags & Q1MDLEF_TRACER)
			CLQ1_TrailParticles (oldorg, ent->state.origin, 3);
		else if (ModelFlags & Q1MDLEF_TRACER2)
			CLQ1_TrailParticles (oldorg, ent->state.origin, 5);
		else if (ModelFlags & Q1MDLEF_ROCKET)
		{
			CLQ1_TrailParticles (oldorg, ent->state.origin, 0);
			CLQ1_RocketLight(i, ent->state.origin);
		}
		else if (ModelFlags & Q1MDLEF_GRENADE)
			CLQ1_TrailParticles (oldorg, ent->state.origin, 1);
		else if (ModelFlags & Q1MDLEF_TRACER3)
			CLQ1_TrailParticles (oldorg, ent->state.origin, 6);

		refEntity_t rent;
		Com_Memset(&rent, 0, sizeof(rent));
		rent.reType = RT_MODEL;
		VectorCopy(ent->state.origin, rent.origin);
		rent.hModel = cl.model_precache[ent->state.modelindex];
		CLQ1_SetRefEntAxis(&rent, ent->state.angles);
		rent.frame = ent->state.frame;
		rent.shaderTime = ent->syncbase;
		R_HandleRefEntColormap(&rent, ent->state.colormap);
		rent.skinNum = ent->state.skinnum;
		if (i == cl.viewentity && !chase_active->value)
		{
			rent.renderfx |= RF_THIRD_PERSON;
		}
		R_AddRefEntityToScene(&rent);
	}
}

static void CL_LinkStaticEntities()
{
	q1entity_t* pent = clq1_static_entities;
	for (int i = 0; i < cl.qh_num_statics; i++, pent++)
	{
		refEntity_t rent;
		Com_Memset(&rent, 0, sizeof(rent));
		rent.reType = RT_MODEL;
		VectorCopy(pent->state.origin, rent.origin);
		rent.hModel = cl.model_precache[pent->state.modelindex];
		CLQ1_SetRefEntAxis(&rent, pent->state.angles);
		rent.frame = pent->state.frame;
		rent.shaderTime = pent->syncbase;
		rent.skinNum = pent->state.skinnum;
		R_AddRefEntityToScene(&rent);
	}
}

/*
===============
CL_ReadFromServer

Read all incoming data from the server
===============
*/
int CL_ReadFromServer (void)
{
	int		ret;

	cl.oldtime = cl.serverTimeFloat;
	cl.serverTimeFloat += host_frametime;
	cl.serverTime = (int)(cl.serverTimeFloat * 1000);
	
	do
	{
		ret = CL_GetMessage ();
		if (ret == -1)
			Host_Error ("CL_ReadFromServer: lost server connection");
		if (!ret)
			break;
		
		cl.last_received_message = realtime;
		CL_ParseServerMessage ();
	} while (ret && cls.state == ca_connected);
	
	if (cl_shownet->value)
		Con_Printf ("\n");

	CL_RelinkEntities ();
	CLQ1_UpdateTEnts ();
	CL_LinkStaticEntities();

//
// bring the links up to date
//
	return 0;
}

/*
=================
CL_SendCmd
=================
*/
void CL_SendCmd (void)
{
	q1usercmd_t		cmd;

	if (cls.state != ca_connected)
		return;

	if (cls.signon == SIGNONS)
	{
	// get basic movement from keyboard
		CL_BaseMove (&cmd);
	
	// allow mice or other external controllers to add to the move
		CL_MouseMove(&cmd);
	
	// send the unreliable message
		CL_SendMove (&cmd);
	
	}

	if (cls.demoplayback)
	{
		cls.message.Clear();
		return;
	}
	
// send the reliable message
	if (!cls.message.cursize)
		return;		// no message at all
	
	if (!NET_CanSendMessage (cls.netcon, &clc.netchan))
	{
		Con_DPrintf ("CL_WriteToServer: can't send\n");
		return;
	}

	if (NET_SendMessage (cls.netcon, &clc.netchan, &cls.message) == -1)
		Host_Error ("CL_WriteToServer: lost server connection");

	cls.message.Clear();
}

/*
=================
CL_Init
=================
*/
void CL_Init (void)
{	
	cls_common = &cls;
	clc_common = &clc;
	cl_common = &cl;
	cls.message.InitOOB(cls.message_buf, sizeof(cls.message_buf));

	CL_SharedInit();

	CL_InitInput ();

	//
	// register our commands
	//
	clqh_name = Cvar_Get("_cl_name", "player", CVAR_ARCHIVE);
	clqh_color = Cvar_Get("_cl_color", "0", CVAR_ARCHIVE);
	cl_upspeed = Cvar_Get("cl_upspeed", "200", 0);
	cl_forwardspeed = Cvar_Get("cl_forwardspeed", "200", CVAR_ARCHIVE);
	cl_backspeed = Cvar_Get("cl_backspeed", "200", CVAR_ARCHIVE);
	cl_sidespeed = Cvar_Get("cl_sidespeed","350", 0);
	cl_movespeedkey = Cvar_Get("cl_movespeedkey", "2.0", 0);
	cl_yawspeed = Cvar_Get("cl_yawspeed", "140", 0);
	cl_pitchspeed = Cvar_Get("cl_pitchspeed", "150", 0);
	cl_anglespeedkey = Cvar_Get("cl_anglespeedkey", "1.5", 0);
	cl_shownet = Cvar_Get("cl_shownet", "0", 0);	// can be 0, 1, or 2
	cl_nolerp = Cvar_Get("cl_nolerp", "0", 0);
	lookspring = Cvar_Get("lookspring", "0", CVAR_ARCHIVE);
	lookstrafe = Cvar_Get("lookstrafe", "0", CVAR_ARCHIVE);
	sensitivity = Cvar_Get("sensitivity", "3", CVAR_ARCHIVE);

	m_pitch = Cvar_Get("m_pitch", "0.022", CVAR_ARCHIVE);
	m_yaw = Cvar_Get("m_yaw", "0.022", CVAR_ARCHIVE);
	m_forward = Cvar_Get("m_forward", "1", CVAR_ARCHIVE);
	m_side = Cvar_Get("m_side", "0.8", CVAR_ARCHIVE);

	cl_doubleeyes = Cvar_Get("cl_doubleeyes", "1", 0);

	Cmd_AddCommand ("entities", CL_PrintEntities_f);
	Cmd_AddCommand ("disconnect", CL_Disconnect_f);
	Cmd_AddCommand ("record", CL_Record_f);
	Cmd_AddCommand ("stop", CL_Stop_f);
	Cmd_AddCommand ("playdemo", CL_PlayDemo_f);
	Cmd_AddCommand ("timedemo", CL_TimeDemo_f);
}

void CIN_StartedPlayback()
{
}

bool CIN_IsInCinematicState()
{
	return false;
}

void CIN_FinishCinematic()
{
}

float* CL_GetSimOrg()
{
	return clq1_entities[cl.viewentity].state.origin;
}
