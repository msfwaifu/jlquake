// cl_main.c  -- client main loop

/*
 * $Header: /H2 Mission Pack/CL_MAIN.C 12    3/16/98 5:32p Jweier $
 */

#include "quakedef.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

// we need to declare some mouse variables here, because the menu system
// references them even when on a unix system.

static float save_sensitivity;

/*
=====================
CL_ClearState

=====================
*/
void CL_ClearState(void)
{
	if (!sv.active)
	{
		Host_ClearMemory();
	}

// wipe the entire cl structure
	Com_Memset(&cl, 0, sizeof(cl));

	clc.netchan.message.Clear();

// clear other arrays
	Com_Memset(h2cl_entities, 0, sizeof(h2cl_entities));
	Com_Memset(clh2_baselines, 0, sizeof(clh2_baselines));
	CL_ClearDlights();
	CL_ClearLightStyles();
	CLH2_ClearTEnts();
	CLH2_ClearEffects();

	cl.h2_current_frame = cl.h2_current_sequence = 99;
	cl.h2_reference_frame = cl.h2_last_sequence = 199;
	cl.h2_need_build = 2;

	plaquemessage = "";

	SB_InvReset();
}

void CL_RemoveGIPFiles(const char* path)
{
	if (!fs_homepath)
	{
		return;
	}
	char* netpath;
	if (path)
	{
		netpath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, path);
	}
	else
	{
		netpath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, "");
		netpath[String::Length(netpath) - 1] = 0;
	}
	int numSysFiles;
	char** sysFiles = Sys_ListFiles(netpath, ".gip", NULL, &numSysFiles, false);
	for (int i = 0; i < numSysFiles; i++)
	{
		if (path)
		{
			netpath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, va("%s/%s", path, sysFiles[i]));
		}
		else
		{
			netpath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, sysFiles[i]);
		}
		FS_Remove(netpath);
	}
	Sys_FreeFileList(sysFiles);
}

void CL_CopyFiles(const char* source, const char* ext, const char* dest)
{
	char* netpath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, source);
	if (!source[0])
	{
		netpath[String::Length(netpath) - 1] = 0;
	}
	int numSysFiles;
	char** sysFiles = Sys_ListFiles(netpath, ext, NULL, &numSysFiles, false);
	for (int i = 0; i < numSysFiles; i++)
	{
		char* srcpath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, va("%s%s", source, sysFiles[i]));
		char* dstpath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, va("%s%s", dest, sysFiles[i]));
		FS_CopyFile(srcpath, dstpath);
	}
	Sys_FreeFileList(sysFiles);
}

/*
=====================
CL_Disconnect

Sends a disconnect message to the server
This is also called on Host_Error, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect(void)
{
	CL_ClearParticles();	//jfm: need to clear parts because some now check world
	S_StopAllSounds();	// stop sounds (especially looping!)
	loading_stage = 0;

// if running a local server, shut it down
	if (clc.demoplaying)
	{
		CL_StopPlayback();
	}
	else if (cls.state == CA_ACTIVE)
	{
		if (clc.demorecording)
		{
			CL_Stop_f();
		}

		Con_DPrintf("Sending h2clc_disconnect\n");
		clc.netchan.message.Clear();
		clc.netchan.message.WriteByte(h2clc_disconnect);
		NET_SendUnreliableMessage(cls.qh_netcon, &clc.netchan, &clc.netchan.message);
		clc.netchan.message.Clear();
		NET_Close(cls.qh_netcon, &clc.netchan);

		cls.state = CA_DISCONNECTED;
		if (sv.active)
		{
			Host_ShutdownServer(false);
		}

		CL_RemoveGIPFiles(NULL);
	}

	clc.demoplaying = cls.qh_timedemo = false;
	clc.qh_signon = 0;
}

void CL_Disconnect_f(void)
{
	CL_Disconnect();
	if (sv.active)
	{
		Host_ShutdownServer(false);
	}
}




/*
=====================
CL_EstablishConnection

Host should be either "local" or a net address to be passed on
=====================
*/
void CL_EstablishConnection(const char* host)
{
	if (cls.state == CA_DEDICATED)
	{
		return;
	}

	if (clc.demoplaying)
	{
		return;
	}

	CL_Disconnect();

	Com_Memset(&clc.netchan, 0, sizeof(clc.netchan));
	clc.netchan.sock = NS_CLIENT;
	clc.netchan.message.InitOOB(clc.netchan.messageBuffer, 1024);
	cls.qh_netcon = NET_Connect(host, &clc.netchan);
	if (!cls.qh_netcon)
	{
		Host_Error("CL_Connect: connect failed\n");
	}
	clc.netchan.lastReceived = net_time * 1000;
	Con_DPrintf("CL_EstablishConnection: connected to %s\n", host);

	cls.qh_demonum = -1;			// not in the demo loop now
	cls.state = CA_ACTIVE;
	clc.qh_signon = 0;				// need all the signon messages before playing
}

/*
=====================
CL_NextDemo

Called to play the next demo in the demo loop
=====================
*/
void CL_NextDemo(void)
{
	char str[1024];

	if (cls.qh_demonum == -1)
	{
		return;		// don't play demos

	}
	SCR_BeginLoadingPlaque();

	if (!cls.qh_demos[cls.qh_demonum][0] || cls.qh_demonum == MAX_DEMOS)
	{
		cls.qh_demonum = 0;
		if (!cls.qh_demos[cls.qh_demonum][0])
		{
			Con_Printf("No demos listed with startdemos\n");
			cls.qh_demonum = -1;
			return;
		}
	}

	sprintf(str,"playdemo %s\n", cls.qh_demos[cls.qh_demonum]);
	Cbuf_InsertText(str);
	cls.qh_demonum++;
}

/*
==============
CL_PrintEntities_f
==============
*/
void CL_PrintEntities_f(void)
{
	h2entity_t* ent;
	int i;

	for (i = 0,ent = h2cl_entities; i < cl.qh_num_entities; i++,ent++)
	{
		Con_Printf("%3i:",i);
		if (!ent->state.modelindex)
		{
			Con_Printf("EMPTY\n");
			continue;
		}
		Con_Printf("%s:%2i  (%5.1f,%5.1f,%5.1f) [%5.1f %5.1f %5.1f]\n",
			R_ModelName(cl.model_draw[ent->state.modelindex]),ent->state.frame, ent->state.origin[0], ent->state.origin[1], ent->state.origin[2], ent->state.angles[0], ent->state.angles[1], ent->state.angles[2]);
	}
}

bool CL_IsServerActive()
{
	return !!sv.active;
}

/*
===============
CL_RelinkEntities
===============
*/
void CL_RelinkEntities(void)
{
	h2entity_t* ent;
	int i, j;
	float frac, f, d;
	vec3_t delta;
	//float		bobjrotate;
	vec3_t oldorg;
	int c;

	c = 0;
// determine partial update time
	frac = CLQH_LerpPoint();

	R_ClearScene();

//
// interpolate player info
//
	for (i = 0; i < 3; i++)
		cl.qh_velocity[i] = cl.qh_mvelocity[1][i] +
							frac * (cl.qh_mvelocity[0][i] - cl.qh_mvelocity[1][i]);

	if (clc.demoplaying && !intro_playing)
	{
		// interpolate the angles
		for (j = 0; j < 3; j++)
		{
			d = cl.qh_mviewangles[0][j] - cl.qh_mviewangles[1][j];
			if (d > 180)
			{
				d -= 360;
			}
			else if (d < -180)
			{
				d += 360;
			}
			cl.viewangles[j] = cl.qh_mviewangles[1][j] + frac * d;
		}
	}

	//bobjrotate = AngleMod(100*(cl.time+ent->origin[0]+ent->origin[1]));

// start on the entity after the world
	for (i = 1,ent = h2cl_entities + 1; i < cl.qh_num_entities; i++,ent++)
	{
		if (!ent->state.modelindex)
		{
			// empty slot
			continue;
		}

// if the object wasn't included in the last packet, remove it
		if (ent->msgtime != cl.qh_mtime[0] && !(clh2_baselines[i].flags & BE_ON))
		{
			ent->state.modelindex = 0;
			continue;
		}

		VectorCopy(ent->state.origin, oldorg);

		if (ent->msgtime != cl.qh_mtime[0])
		{	// the entity was not updated in the last message
			// so move to the final spot
			VectorCopy(ent->msg_origins[0], ent->state.origin);
			VectorCopy(ent->msg_angles[0], ent->state.angles);
		}
		else
		{	// if the delta is large, assume a teleport and don't lerp
			f = frac;
			for (j = 0; j < 3; j++)
			{
				delta[j] = ent->msg_origins[0][j] - ent->msg_origins[1][j];

				if (delta[j] > 100 || delta[j] < -100)
				{
					f = 1;		// assume a teleportation, not a motion
				}
			}

			// interpolate the origin and angles
			for (j = 0; j < 3; j++)
			{
				ent->state.origin[j] = ent->msg_origins[1][j] + f * delta[j];

				d = ent->msg_angles[0][j] - ent->msg_angles[1][j];
				if (d > 180)
				{
					d -= 360;
				}
				else if (d < -180)
				{
					d += 360;
				}
				ent->state.angles[j] = ent->msg_angles[1][j] + f * d;
			}

		}

// rotate binary objects locally
// BG: Moved to r_alias.c / gl_rmain.c
		//if(ent->model->flags&H2MDLEF_ROTATE)
		//{
		//	ent->angles[1] = AngleMod(ent->origin[0]+ent->origin[1]
		//		+(100*cl.time));
		//}


		c++;
		if (ent->state.effects & EF_DARKFIELD)
		{
			CLH2_DarkFieldParticles(ent->state.origin);
		}
		if (ent->state.effects & EF_MUZZLEFLASH)
		{
			if (cl_prettylights->value)
			{
				CLH2_MuzzleFlashLight(i, ent->state.origin, ent->state.angles, true);
			}
		}
		if (ent->state.effects & EF_BRIGHTLIGHT)
		{
			if (cl_prettylights->value)
			{
				CLH2_BrightLight(i, ent->state.origin);
			}
		}
		if (ent->state.effects & EF_DIMLIGHT)
		{
			if (cl_prettylights->value)
			{
				CLH2_DimLight(i, ent->state.origin);
			}
		}
		if (ent->state.effects & EF_DARKLIGHT)
		{
			if (cl_prettylights->value)
			{
				CLH2_DarkLight(i, ent->state.origin);
			}
		}
		if (ent->state.effects & EF_LIGHT)
		{
			if (cl_prettylights->value)
			{
				CLH2_Light(i, ent->state.origin);
			}
		}

		int ModelFlags = R_ModelFlags(cl.model_draw[ent->state.modelindex]);
		if (ModelFlags & H2MDLEF_GIB)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin, rt_blood);
		}
		else if (ModelFlags & H2MDLEF_ZOMGIB)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin, rt_slight_blood);
		}
		else if (ModelFlags & H2MDLEF_BLOODSHOT)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin, rt_bloodshot);
		}
		else if (ModelFlags & H2MDLEF_TRACER)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin, rt_tracer);
		}
		else if (ModelFlags & H2MDLEF_TRACER2)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin, rt_tracer2);
		}
		else if (ModelFlags & H2MDLEF_ROCKET)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin, rt_rocket_trail);
		}
		else if (ModelFlags & H2MDLEF_FIREBALL)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin, rt_fireball);
			if (cl_prettylights->value)
			{
				CLH2_FireBallLight(i, ent->state.origin);
			}
		}
		else if (ModelFlags & H2MDLEF_ACIDBALL)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin, rt_acidball);
			if (cl_prettylights->value)
			{
				CLH2_FireBallLight(i, ent->state.origin);
			}
		}
		else if (ModelFlags & H2MDLEF_ICE)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin, rt_ice);
		}
		else if (ModelFlags & H2MDLEF_SPIT)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin, rt_spit);
			if (cl_prettylights->value)
			{
				CLH2_SpitLight(i, ent->state.origin);
			}
		}
		else if (ModelFlags & H2MDLEF_SPELL)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin, rt_spell);
		}
		else if (ModelFlags & H2MDLEF_GRENADE)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin, rt_smoke);
		}
		else if (ModelFlags & H2MDLEF_TRACER3)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin, rt_voor_trail);
		}
		else if (ModelFlags & H2MDLEF_VORP_MISSILE)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin, rt_vorpal);
		}
		else if (ModelFlags & H2MDLEF_SET_STAFF)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin,rt_setstaff);
		}
		else if (ModelFlags & H2MDLEF_MAGICMISSILE)
		{
			if ((rand() & 3) < 1)
			{
				CLH2_TrailParticles(oldorg, ent->state.origin, rt_magicmissile);
			}
		}
		else if (ModelFlags & H2MDLEF_BONESHARD)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin, rt_boneshard);
		}
		else if (ModelFlags & H2MDLEF_SCARAB)
		{
			CLH2_TrailParticles(oldorg, ent->state.origin, rt_scarab);
		}

		if (ent->state.effects & EF_NODRAW)
		{
			continue;
		}

		refEntity_t rent;
		Com_Memset(&rent, 0, sizeof(rent));
		rent.reType = RT_MODEL;
		VectorCopy(ent->state.origin, rent.origin);
		rent.hModel = cl.model_draw[ent->state.modelindex];
		rent.frame = ent->state.frame;
		rent.syncBase = ent->syncbase;
		rent.skinNum = ent->state.skinnum;
		CLH2_SetRefEntAxis(&rent, ent->state.angles, vec3_origin, ent->state.scale, ent->state.colormap, ent->state.abslight, ent->state.drawflags);
		CLH2_HandleCustomSkin(&rent, i <= cl.qh_maxclients ? i - 1 : -1);
		if (i == cl.viewentity && !chase_active->value)
		{
			rent.renderfx |= RF_THIRD_PERSON;
		}
		R_AddRefEntityToScene(&rent);
	}

/*	if (c != lastc)
    {
        Con_Printf("Number of entities: %d\n",c);
        lastc = c;
    }*/
}

static void CL_LinkStaticEntities()
{
	h2entity_t* pent = h2cl_static_entities;
	for (int i = 0; i < cl.qh_num_statics; i++, pent++)
	{
		refEntity_t rent;
		Com_Memset(&rent, 0, sizeof(rent));
		rent.reType = RT_MODEL;
		VectorCopy(pent->state.origin, rent.origin);
		rent.hModel = cl.model_draw[pent->state.modelindex];
		rent.frame = pent->state.frame;
		rent.syncBase = pent->syncbase;
		rent.skinNum = pent->state.skinnum;
		CLH2_SetRefEntAxis(&rent, pent->state.angles, vec3_origin, pent->state.scale, pent->state.colormap, pent->state.abslight, pent->state.drawflags);
		CLH2_HandleCustomSkin(&rent, -1);
		R_AddRefEntityToScene(&rent);
	}
}

/*
===============
CL_ReadFromServer

Read all incoming data from the server
===============
*/
int CL_ReadFromServer(void)
{
	int ret;

	cl.qh_oldtime = cl.qh_serverTimeFloat;
	cl.qh_serverTimeFloat += host_frametime;
	cl.serverTime = (int)(cl.qh_serverTimeFloat * 1000);

	do
	{
		ret = CL_GetMessage();
		if (ret == -1)
		{
			Host_Error("CL_ReadFromServer: lost server connection");
		}
		if (!ret)
		{
			break;
		}

		cl.qh_last_received_message = realtime;
		CL_ParseServerMessage();
	}
	while (ret && cls.state == CA_ACTIVE);

	if (cl_shownet->value)
	{
		Con_Printf("\n");
	}

	CL_RelinkEntities();
	CLH2_UpdateTEnts();
	CL_LinkStaticEntities();

//
// bring the links up to date
//
	return 0;
}

void CL_Sensitivity_save_f(void)
{
	if (Cmd_Argc() != 2)
	{
		Con_Printf("sensitivity_save <save/restore>\n");
		return;
	}

	if (String::ICmp(Cmd_Argv(1),"save") == 0)
	{
		save_sensitivity = cl_sensitivity->value;
	}
	else if (String::ICmp(Cmd_Argv(1),"restore") == 0)
	{
		Cvar_SetValue("sensitivity", save_sensitivity);
	}
}
/*
=================
CL_Init
=================
*/
void CL_Init(void)
{
	CL_SharedInit();

	CL_InitInput();


//
// register our commands
//
	clqh_name = Cvar_Get("_cl_name", "player", CVAR_ARCHIVE);
	clqh_color = Cvar_Get("_cl_color", "0", CVAR_ARCHIVE);
	clh2_playerclass = Cvar_Get("_cl_playerclass", "5", CVAR_ARCHIVE);
	clqh_nolerp = Cvar_Get("cl_nolerp", "0", 0);

	cl_prettylights = Cvar_Get("cl_prettylights", "1", 0);

	Cmd_AddCommand("entities", CL_PrintEntities_f);
	Cmd_AddCommand("disconnect", CL_Disconnect_f);
	Cmd_AddCommand("record", CL_Record_f);
	Cmd_AddCommand("stop", CL_Stop_f);
	Cmd_AddCommand("playdemo", CL_PlayDemo_f);
	Cmd_AddCommand("timedemo", CL_TimeDemo_f);
	Cmd_AddCommand("sensitivity_save", CL_Sensitivity_save_f);
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
	return h2cl_entities[cl.viewentity].state.origin;
}
