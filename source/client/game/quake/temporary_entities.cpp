//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
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

enum { MAX_BEAMS_Q1 = 24 };

struct q1beam_t
{
	int entity;
	qhandle_t model;
	float endtime;
	vec3_t start;
	vec3_t end;
};

static q1beam_t clq1_beams[MAX_BEAMS_Q1];
q1explosion_t cl_explosions[MAX_EXPLOSIONS_Q1];

static sfxHandle_t clq1_sfx_wizhit;
static sfxHandle_t clq1_sfx_knighthit;
static sfxHandle_t clq1_sfx_tink1;
static sfxHandle_t clq1_sfx_ric1;
static sfxHandle_t clq1_sfx_ric2;
static sfxHandle_t clq1_sfx_ric3;
static sfxHandle_t clq1_sfx_r_exp3;

void CLQ1_InitTEnts()
{
	clq1_sfx_wizhit = S_RegisterSound("wizard/hit.wav");
	clq1_sfx_knighthit = S_RegisterSound("hknight/hit.wav");
	clq1_sfx_tink1 = S_RegisterSound("weapons/tink1.wav");
	clq1_sfx_ric1 = S_RegisterSound("weapons/ric1.wav");
	clq1_sfx_ric2 = S_RegisterSound("weapons/ric2.wav");
	clq1_sfx_ric3 = S_RegisterSound("weapons/ric3.wav");
	clq1_sfx_r_exp3 = S_RegisterSound("weapons/r_exp3.wav");
}

void CLQ1_ClearTEnts()
{
	Com_Memset(clq1_beams, 0, sizeof(clq1_beams));
	Com_Memset(cl_explosions, 0, sizeof(cl_explosions));
}

void CLQ1_ParseBeam(QMsg& message, qhandle_t model)
{
	int entity = message.ReadShort();

	vec3_t start;
	start[0] = message.ReadCoord();
	start[1] = message.ReadCoord();
	start[2] = message.ReadCoord();

	vec3_t end;
	end[0] = message.ReadCoord();
	end[1] = message.ReadCoord();
	end[2] = message.ReadCoord();

	// override any beam with the same entity
	q1beam_t* beam = clq1_beams;
	for (int i = 0; i < MAX_BEAMS_Q1; i++, beam++)
	{
		if (beam->entity == entity)
		{
			beam->entity = entity;
			beam->model = model;
			beam->endtime = cl_common->serverTime * 0.001 + 0.2;
			VectorCopy(start, beam->start);
			VectorCopy(end, beam->end);
			return;
		}
	}

	// find a free beam
	beam = clq1_beams;
	for (int i = 0; i < MAX_BEAMS_Q1; i++, beam++)
	{
		if (!beam->model || beam->endtime < cl_common->serverTime * 0.001)
		{
			beam->entity = entity;
			beam->model = model;
			beam->endtime = cl_common->serverTime * 0.001 + 0.2;
			VectorCopy (start, beam->start);
			VectorCopy (end, beam->end);
			return;
		}
	}
	Log::write("beam list overflow!\n");
}

void CLQ1_UpdateBeams()
{
	// update lightning
	q1beam_t* beam = clq1_beams;
	for (int i = 0; i < MAX_BEAMS_Q1; i++, beam++)
	{
		if (!beam->model || beam->endtime < cl_common->serverTime * 0.001)
		{
			continue;
		}

		// if coming from the player, update the start position
		if (beam->entity == CL_GetViewEntity())
		{
			VectorCopy(CL_GetSimOrg(), beam->start);
		}

		// calculate pitch and yaw
		vec3_t direction;
		VectorSubtract(beam->end, beam->start, direction);

		float yaw, pitch;
		if (direction[1] == 0 && direction[0] == 0)
		{
			yaw = 0;
			if (direction[2] > 0)
			{
				pitch = 90;
			}
			else
			{
				pitch = 270;
			}
		}
		else
		{
			yaw = (int)(atan2(direction[1], direction[0]) * 180 / M_PI);
			if (yaw < 0)
			{
				yaw += 360;
			}

			float forward = sqrt(direction[0] * direction[0] + direction[1] * direction[1]);
			pitch = (int)(atan2(direction[2], forward) * 180 / M_PI);
			if (pitch < 0)
			{
				pitch += 360;
			}
		}

		// add new entities for the lightning
		vec3_t origin;
		VectorCopy(beam->start, origin);
		float distance = VectorNormalize(direction);
		while (distance > 0)
		{
			refEntity_t entity;
			Com_Memset(&entity, 0, sizeof(entity));
			entity.reType = RT_MODEL;
			VectorCopy(origin, entity.origin);
			entity.hModel = beam->model;
			vec3_t angles;
			angles[0] = pitch;
			angles[1] = yaw;
			angles[2] = rand() % 360;
			CLQ1_SetRefEntAxis(&entity, angles);
			R_AddRefEntityToScene(&entity);

			for (int j = 0; j < 3; j++)
			{
				origin[j] += direction[j] * 30;
			}
			distance -= 30;
		}
	}
}

q1explosion_t* CLQ1_AllocExplosion()
{
	for (int i = 0; i < MAX_EXPLOSIONS_Q1; i++)
	{
		if (!cl_explosions[i].model)
		{
			return &cl_explosions[i];
		}
	}

	// find the oldest explosion
	float time = cl_common->serverTime * 0.001;
	int index = 0;
	for (int i = 0; i < MAX_EXPLOSIONS_Q1; i++)
	{
		if (cl_explosions[i].start < time)
		{
			time = cl_explosions[i].start;
			index = i;
		}
	}
	return &cl_explosions[index];
}

void CLQ1_UpdateExplosions()
{
	q1explosion_t* ex = cl_explosions;
	for (int i = 0; i < MAX_EXPLOSIONS_Q1; i++, ex++)
	{
		if (!ex->model)
		{
			continue;
		}
		int f = 10 * (cl_common->serverTime * 0.001 - ex->start);
		if (f >= R_ModelNumFrames(ex->model))
		{
			ex->model = 0;
			continue;
		}

		refEntity_t ent;
		Com_Memset(&ent, 0, sizeof(ent));
		ent.reType = RT_MODEL;
		VectorCopy(ex->origin, ent.origin);
		ent.hModel = ex->model;
		ent.frame = f;
		CLQ1_SetRefEntAxis(&ent, vec3_origin);
		R_AddRefEntityToScene(&ent);
	}
}

void CLQ1_ParseWizSpike(QMsg& message)
{
	vec3_t pos;
	pos[0] = message.ReadCoord();
	pos[1] = message.ReadCoord();
	pos[2] = message.ReadCoord();

	CLQ1_RunParticleEffect(pos, vec3_origin, 20, 30);
	S_StartSound(pos, -1, 0, clq1_sfx_wizhit, 1, 1);
}

void CLQ1_ParseKnightSpike(QMsg& message)
{
	vec3_t pos;
	pos[0] = message.ReadCoord();
	pos[1] = message.ReadCoord();
	pos[2] = message.ReadCoord();

	CLQ1_RunParticleEffect(pos, vec3_origin, 226, 20);
	S_StartSound(pos, -1, 0, clq1_sfx_knighthit, 1, 1);
}

void CLQ1_ParseSpike(QMsg& message)
{
	vec3_t pos;
	pos[0] = message.ReadCoord();
	pos[1] = message.ReadCoord();
	pos[2] = message.ReadCoord();

	CLQ1_RunParticleEffect (pos, vec3_origin, 0, 10);
	if (rand() % 5)
	{
		S_StartSound(pos, -1, 0, clq1_sfx_tink1, 1, 1);
	}
	else
	{
		int rnd = rand() & 3;
		if (rnd == 1)
		{
			S_StartSound(pos, -1, 0, clq1_sfx_ric1, 1, 1);
		}
		else if (rnd == 2)
		{
			S_StartSound(pos, -1, 0, clq1_sfx_ric2, 1, 1);
		}
		else
		{
			S_StartSound(pos, -1, 0, clq1_sfx_ric3, 1, 1);
		}
	}
}

void CLQ1_SuperParseSpike(QMsg& message)
{
	vec3_t pos;
	pos[0] = message.ReadCoord();
	pos[1] = message.ReadCoord();
	pos[2] = message.ReadCoord();

	CLQ1_RunParticleEffect(pos, vec3_origin, 0, 20);
	if (rand() % 5)
	{
		S_StartSound(pos, -1, 0, clq1_sfx_tink1, 1, 1);
	}
	else
	{
		int rnd = rand() & 3;
		if (rnd == 1)
		{
			S_StartSound(pos, -1, 0, clq1_sfx_ric1, 1, 1);
		}
		else if (rnd == 2)
		{
			S_StartSound(pos, -1, 0, clq1_sfx_ric2, 1, 1);
		}
		else
		{
			S_StartSound(pos, -1, 0, clq1_sfx_ric3, 1, 1);
		}
	}
}

void CLQ1_ParseExplosion(QMsg& message)
{
	vec3_t pos;
	pos[0] = message.ReadCoord();
	pos[1] = message.ReadCoord();
	pos[2] = message.ReadCoord();

	CLQ1_ParticleExplosion(pos);
	CLQ1_ExplosionLight(pos);
	S_StartSound(pos, -1, 0, clq1_sfx_r_exp3, 1, 1);

	if (GGameType & GAME_QuakeWorld)
	{
		q1explosion_t* ex = CLQ1_AllocExplosion();
		VectorCopy(pos, ex->origin);
		ex->start = cl_common->serverTime * 0.001;
		ex->model = R_RegisterModel("progs/s_explod.spr");
	}
}

void CLQ1_ParseTarExplosion(QMsg& message)
{
	vec3_t pos;
	pos[0] = message.ReadCoord();
	pos[1] = message.ReadCoord();
	pos[2] = message.ReadCoord();

	CLQ1_BlobExplosion(pos);
	S_StartSound(pos, -1, 0, clq1_sfx_r_exp3, 1, 1);
}

void CLQ1_ParseExplosion2(QMsg& message)
{
	vec3_t pos;
	pos[0] = message.ReadCoord();
	pos[1] = message.ReadCoord();
	pos[2] = message.ReadCoord();
	int colorStart = message.ReadByte();
	int colorLength = message.ReadByte();

	CLQ1_ParticleExplosion2(pos, colorStart, colorLength);
	CLQ1_ExplosionLight(pos);
	S_StartSound(pos, -1, 0, clq1_sfx_r_exp3, 1, 1);
}

void CLQ1_ParseLavaSplash(QMsg& message)
{
	vec3_t pos;
	pos[0] = message.ReadCoord();
	pos[1] = message.ReadCoord();
	pos[2] = message.ReadCoord();

	CLQ1_LavaSplash(pos);
}

void CLQ1_ParseTeleportSplash(QMsg& message)
{
	vec3_t pos;
	pos[0] = message.ReadCoord();
	pos[1] = message.ReadCoord();
	pos[2] = message.ReadCoord();

	CLQ1_TeleportSplash(pos);
}

void CLQ1_ParseGunShot(QMsg& message)
{
	int cnt = 1;
	if (GGameType & GAME_QuakeWorld)
	{
		cnt = message.ReadByte();
	}
	vec3_t pos;
	pos[0] = message.ReadCoord();
	pos[1] = message.ReadCoord();
	pos[2] = message.ReadCoord();

	CLQ1_RunParticleEffect(pos, vec3_origin, 0, 20 * cnt);
}

void CLQ1_ParseBlood(QMsg& message)
{
	int cnt = message.ReadByte();
	vec3_t pos;
	pos[0] = message.ReadCoord();
	pos[1] = message.ReadCoord();
	pos[2] = message.ReadCoord();

	CLQ1_RunParticleEffect(pos, vec3_origin, 73, 20 * cnt);
}

void CLQ1_ParseLightningBlood(QMsg& message)
{
	vec3_t pos;
	pos[0] = message.ReadCoord();
	pos[1] = message.ReadCoord();
	pos[2] = message.ReadCoord();

	CLQ1_RunParticleEffect(pos, vec3_origin, 225, 50);
}
