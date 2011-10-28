/*
Copyright (C) 1997-2001 Id Software, Inc.

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
// cl_view.c -- player rendering positioning

#include "client.h"

//=============
//
// development tools for weapons
//
int			gun_frame;
qhandle_t	gun_model;

//=============

Cvar*		crosshair;
Cvar*		cl_testparticles;
Cvar*		cl_testentities;
Cvar*		cl_testlights;
Cvar*		cl_testblend;

Cvar*		cl_polyblend;

char cl_weaponmodels[MAX_CLIENTWEAPONMODELS][MAX_QPATH];
int num_cl_weaponmodels;

float		v_blend[4];			// final blending color

/*
================
V_TestParticles

If cl_testparticles is set, create 4096 particles in the view
================
*/
void V_TestParticles (void)
{
	for (int i = 0; i < MAX_PARTICLES; i++)
	{
		float d = i * 0.25;
		float r = 4 * ((i & 7) - 3.5);
		float u = 4 * (((i >> 3) & 7) - 3.5);

		vec3_t origin;
		for (int j = 0; j < 3; j++)
		{
			origin[j] = cl.refdef.vieworg[j] + cl.refdef.viewaxis[0][j] * d -
				cl.refdef.viewaxis[1][j] * r + cl.refdef.viewaxis[2][j] * u;
		}

		R_AddParticleToScene(origin, r_palette[8][0], r_palette[8][1], r_palette[8][2], (int)(cl_testparticles->value * 255), 1, PARTTEX_Default);
	}
}

/*
================
V_TestEntities

If cl_testentities is set, create 32 player models
================
*/
void V_TestEntities()
{
	for (int i = 0; i < 32; i++)
	{
		refEntity_t	ent;
		Com_Memset(&ent, 0, sizeof(ent));

		float r = 64 * ((i % 4) - 1.5);
		float f = 64 * (i / 4) + 128;

		for (int j = 0; j < 3; j++)
		{
			ent.origin[j] = cl.refdef.vieworg[j] + cl.refdef.viewaxis[0][j] * f -
				cl.refdef.viewaxis[1][j] * r;
		}
		AxisClear(ent.axis);

		ent.hModel = cl.baseclientinfo.model;
		ent.customSkin = R_GetImageHandle(cl.baseclientinfo.skin);
		R_AddRefEntityToScene(&ent);
	}
}

/*
================
V_TestLights

If cl_testlights is set, create 32 lights models
================
*/
void V_TestLights (void)
{
	for (int i = 0; i < 32; i++)
	{
		float r = 64 * ( (i%4) - 1.5 );
		float f = 64 * (i/4) + 128;

		vec3_t origin;
		for (int j = 0; j < 3; j++)
		{
			origin[j] = cl.refdef.vieworg[j] + cl.refdef.viewaxis[0][j] * f -
				cl.refdef.viewaxis[1][j] * r;
		}
		R_AddLightToScene(origin, 200, ((i % 6) + 1) & 1, (((i % 6) + 1) & 2) >> 1, (((i % 6) + 1) & 4) >> 2);
	}
}

//===================================================================

/*
@@@@@@@@@@@@@@@@@@@@@
R_BeginRegistrationAndLoadWorld

Specifies the model that will be used as the world
@@@@@@@@@@@@@@@@@@@@@
*/
static void R_BeginRegistrationAndLoadWorld (const char *model)
{
	char	fullname[MAX_QPATH];

	String::Sprintf (fullname, sizeof(fullname), "maps/%s.bsp", model);

	R_Shutdown(false);
	CL_InitRenderStuff();

	R_LoadWorld(fullname);

}

/*
=================
CL_PrepRefresh

Call before entering a new level, or after changing dlls
=================
*/
void CL_PrepRefresh (void)
{
	char		mapname[32];
	int			i;
	char		name[MAX_QPATH];
	float		rotate;
	vec3_t		axis;

	if (!cl.configstrings[CS_MODELS+1][0])
		return;		// no map loaded

	// let the render dll load the map
	String::Cpy(mapname, cl.configstrings[CS_MODELS+1] + 5);	// skip "maps/"
	mapname[String::Length(mapname)-4] = 0;		// cut off ".bsp"

	// register models, pics, and skins
	Com_Printf ("Map: %s\r", mapname); 
	SCR_UpdateScreen ();
	R_BeginRegistrationAndLoadWorld (mapname);
	Com_Printf ("                                     \r");

	// precache status bar pics
	Com_Printf ("pics\r"); 
	SCR_UpdateScreen ();
	SCR_TouchPics ();
	Com_Printf ("                                     \r");

	CLQ2_RegisterTEntModels ();

	num_cl_weaponmodels = 1;
	String::Cpy(cl_weaponmodels[0], "weapon.md2");

	for (i=1 ; i<MAX_MODELS && cl.configstrings[CS_MODELS+i][0] ; i++)
	{
		String::Cpy(name, cl.configstrings[CS_MODELS+i]);
		name[37] = 0;	// never go beyond one line
		if (name[0] != '*')
			Com_Printf ("%s\r", name); 
		SCR_UpdateScreen ();
		Sys_SendKeyEvents ();	// pump message loop
		IN_ProcessEvents();
		if (name[0] == '#')
		{
			// special player weapon model
			if (num_cl_weaponmodels < MAX_CLIENTWEAPONMODELS)
			{
				String::NCpy(cl_weaponmodels[num_cl_weaponmodels], cl.configstrings[CS_MODELS+i]+1,
					sizeof(cl_weaponmodels[num_cl_weaponmodels]) - 1);
				num_cl_weaponmodels++;
			}
		} 
		else
		{
			cl.model_draw[i] = R_RegisterModel(cl.configstrings[CS_MODELS+i]);
			if (name[0] == '*')
				cl.model_clip[i] = CM_InlineModel(String::Atoi(cl.configstrings[CS_MODELS + i] + 1));
			else
				cl.model_clip[i] = 0;
		}
		if (name[0] != '*')
			Com_Printf ("                                     \r");
	}

	Com_Printf ("images\r", i); 
	SCR_UpdateScreen ();
	for (i=1 ; i<MAX_IMAGES && cl.configstrings[CS_IMAGES+i][0] ; i++)
	{
		cl.image_precache[i] = R_RegisterPic (cl.configstrings[CS_IMAGES+i]);
		Sys_SendKeyEvents ();	// pump message loop
		IN_ProcessEvents();
	}
	
	Com_Printf ("                                     \r");
	for (i=0 ; i<MAX_CLIENTS ; i++)
	{
		if (!cl.configstrings[CS_PLAYERSKINS+i][0])
			continue;
		Com_Printf ("client %i\r", i); 
		SCR_UpdateScreen ();
		Sys_SendKeyEvents ();	// pump message loop
		IN_ProcessEvents();
		CL_ParseClientinfo (i);
		Com_Printf ("                                     \r");
	}

	CL_LoadClientinfo (&cl.baseclientinfo, "unnamed\\male/grunt");

	// set sky textures and speed
	Com_Printf ("sky\r", i); 
	SCR_UpdateScreen ();
	rotate = String::Atof(cl.configstrings[CS_SKYROTATE]);
	sscanf (cl.configstrings[CS_SKYAXIS], "%f %f %f", 
		&axis[0], &axis[1], &axis[2]);
	R_SetSky (cl.configstrings[CS_SKY], rotate, axis);
	Com_Printf ("                                     \r");

	R_EndRegistration();

	// clear any lines of console text
	Con_ClearNotify ();

	SCR_UpdateScreen ();
	cl.refresh_prepped = true;

	// start the cd track
	CDAudio_Play (String::Atoi(cl.configstrings[CS_CDTRACK]), true);
}

/*
====================
CalcFov
====================
*/
float CalcFov (float fov_x, float width, float height)
{
	float	a;
	float	x;

	if (fov_x < 1 || fov_x > 179)
		Com_Error (ERR_DROP, "Bad fov: %f", fov_x);

	x = width/tan(fov_x/360*M_PI);

	a = atan (height/x);

	a = a*360/M_PI;

	return a;
}

//============================================================================

// gun frame debugging functions
void V_Gun_Next_f (void)
{
	gun_frame++;
	Com_Printf ("frame %i\n", gun_frame);
}

void V_Gun_Prev_f (void)
{
	gun_frame--;
	if (gun_frame < 0)
		gun_frame = 0;
	Com_Printf ("frame %i\n", gun_frame);
}

void V_Gun_Model_f (void)
{
	char	name[MAX_QPATH];

	if (Cmd_Argc() != 2)
	{
		gun_model = 0;
		return;
	}
	String::Sprintf (name, sizeof(name), "models/%s/tris.md2", Cmd_Argv(1));
	gun_model = R_RegisterModel(name);
}

//============================================================================

/*
============
R_PolyBlend
============
*/
static void R_PolyBlend(refdef_t* fd)
{
	if (!cl_polyblend->value)
		return;
	if (!v_blend[3])
		return;

	R_Draw2DQuad(fd->x, fd->y, fd->width, fd->height, NULL, 0, 0, 0, 0, v_blend[0], v_blend[1], v_blend[2], v_blend[3]);
}

/*
=================
SCR_DrawCrosshair
=================
*/
void SCR_DrawCrosshair (void)
{
	if (!crosshair->value)
		return;

	if (crosshair->modified)
	{
		crosshair->modified = false;
		SCR_TouchPics ();
	}

	if (!crosshair_pic[0])
		return;

	UI_DrawNamedPic (scr_vrect.x + ((scr_vrect.width - crosshair_width)>>1)
	, scr_vrect.y + ((scr_vrect.height - crosshair_height)>>1), crosshair_pic);
}

/*
==================
V_RenderView

==================
*/
void V_RenderView(float stereo_separation)
{
	if (cls.state != ca_active)
		return;

	if (!cl.refresh_prepped)
		return;			// still loading

	if (cl_timedemo->value)
	{
		if (!cl.timedemo_start)
			cl.timedemo_start = Sys_Milliseconds_ ();
		cl.timedemo_frames++;
	}

	// an invalid frame will just use the exact previous refdef
	// we can't use the old frame if the video mode has changed, though...
	if (!cl.q2_frame.valid)
	{
		return;
	}

	R_ClearScene();

	CL_CalcViewValues();

	if (cl_add_entities->integer)
	{
		if (cl_testentities->integer)
		{
			V_TestEntities();
		}
		else
		{
			CL_AddPacketEntities(&cl.q2_frame);
			CL_AddTEnts();
		}
	}

	if (cl_add_particles->integer)
	{
		if (cl_testparticles->integer)
		{
			V_TestParticles();
		}
		else
		{
			CL_AddParticles();
		}
	}

	if (cl_testlights->integer)
	{
		V_TestLights();
	}
	else
	{
		CL_AddDLights();
	}

	CL_AddLightStyles();

	if (cl_testblend->value)
	{
		v_blend[0] = 1;
		v_blend[1] = 0.5;
		v_blend[2] = 0.25;
		v_blend[3] = 0.5;
	}

	// offset vieworg appropriately if we're doing stereo separation
	if ( stereo_separation != 0 )
	{
		vec3_t tmp;

		VectorScale( cl.refdef.viewaxis[1], -stereo_separation, tmp );
		VectorAdd(cl.refdef.vieworg, tmp, cl.refdef.vieworg);
	}

	// never let it sit exactly on a node line, because a water plane can
	// dissapear when viewed with the eye exactly on it.
	// the server protocol only specifies to 1/8 pixel, so add 1/16 in each axis
	cl.refdef.vieworg[0] += 1.0/16;
	cl.refdef.vieworg[1] += 1.0/16;
	cl.refdef.vieworg[2] += 1.0/16;

	cl.refdef.x = scr_vrect.x;
	cl.refdef.y = scr_vrect.y;
	cl.refdef.width = scr_vrect.width;
	cl.refdef.height = scr_vrect.height;
	cl.refdef.fov_y = CalcFov (cl.refdef.fov_x, cl.refdef.width, cl.refdef.height);
	cl.refdef.time = cl.serverTime;

	for (int i = 0; i < MAX_MAP_AREA_BYTES; i++)
	{
		cl.refdef.areamask[i] = ~cl.q2_frame.areabits[i];
	}

	if (!cl_add_blend->value)
	{
		VectorClear(v_blend);
	}

	cl.refdef.rdflags = 0;
	if (cl.q2_frame.playerstate.rdflags & Q2RDF_NOWORLDMODEL)
	{
		cl.refdef.rdflags |= RDF_NOWORLDMODEL;
	}
	if (cl.q2_frame.playerstate.rdflags & Q2RDF_IRGOGGLES)
	{
		cl.refdef.rdflags |= RDF_IRGOGGLES;
	}

	R_RenderScene(&cl.refdef);

	R_PolyBlend(&cl.refdef);

	SCR_DrawCrosshair();
}


/*
=============
V_Viewpos_f
=============
*/
void V_Viewpos_f (void)
{
	Com_Printf ("(%i %i %i) : %i\n", (int)cl.refdef.vieworg[0],
		(int)cl.refdef.vieworg[1], (int)cl.refdef.vieworg[2], 
		(int)(atan2(cl.refdef.viewaxis[0][1], cl.refdef.viewaxis[0][0]) * 180 / M_PI));
}

/*
=============
V_Init
=============
*/
void V_Init (void)
{
	Cmd_AddCommand ("gun_next", V_Gun_Next_f);
	Cmd_AddCommand ("gun_prev", V_Gun_Prev_f);
	Cmd_AddCommand ("gun_model", V_Gun_Model_f);

	Cmd_AddCommand ("viewpos", V_Viewpos_f);

	crosshair = Cvar_Get ("crosshair", "0", CVAR_ARCHIVE);

	cl_testblend = Cvar_Get ("cl_testblend", "0", 0);
	cl_testparticles = Cvar_Get ("cl_testparticles", "0", 0);
	cl_testentities = Cvar_Get ("cl_testentities", "0", 0);
	cl_testlights = Cvar_Get ("cl_testlights", "0", 0);

	cl_polyblend = Cvar_Get("cl_polyblend", "1", 0);
}
