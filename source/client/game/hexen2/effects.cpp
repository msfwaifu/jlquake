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

effect_entity_t EffectEntities[MAX_EFFECT_ENTITIES_H2];
static bool EntityUsed[MAX_EFFECT_ENTITIES_H2];
static int EffectEntityCount;

static sfxHandle_t clh2_fxsfx_bone;
static sfxHandle_t clh2_fxsfx_bonefpow;
static sfxHandle_t clh2_fxsfx_xbowshoot;
static sfxHandle_t clh2_fxsfx_xbowfshoot;
sfxHandle_t clh2_fxsfx_explode;
static sfxHandle_t clh2_fxsfx_mmfire;
static sfxHandle_t clh2_fxsfx_eidolon;
static sfxHandle_t clh2_fxsfx_scarabwhoosh;
sfxHandle_t clh2_fxsfx_scarabgrab;
sfxHandle_t clh2_fxsfx_scarabhome;
sfxHandle_t clh2_fxsfx_scarabbyebye;
static sfxHandle_t clh2_fxsfx_ravensplit;
static sfxHandle_t clh2_fxsfx_ravenfire;
sfxHandle_t clh2_fxsfx_ravengo;
static sfxHandle_t clh2_fxsfx_drillashoot;
sfxHandle_t clh2_fxsfx_drillaspin;
sfxHandle_t clh2_fxsfx_drillameat;

sfxHandle_t clh2_fxsfx_arr2flsh;
sfxHandle_t clh2_fxsfx_arr2wood;
sfxHandle_t clh2_fxsfx_met2stn;

static sfxHandle_t clh2_fxsfx_ripple;
static sfxHandle_t clh2_fxsfx_splash;

static unsigned int randomseed;

static void setseed(unsigned int seed)
{
	randomseed = seed;
}

//unsigned int seedrand(int max)
static float seedrand()
{
	randomseed = (randomseed * 877 + 573) % 9968;
	return (float)randomseed / 9968;
}

void CLHW_InitEffects()
{
	clh2_fxsfx_bone = S_RegisterSound("necro/bonefnrm.wav");
	clh2_fxsfx_bonefpow = S_RegisterSound("necro/bonefpow.wav");
	clh2_fxsfx_xbowshoot = S_RegisterSound("assassin/firebolt.wav");
	clh2_fxsfx_xbowfshoot = S_RegisterSound("assassin/firefblt.wav");
	clh2_fxsfx_explode = S_RegisterSound("weapons/explode.wav");
	clh2_fxsfx_mmfire = S_RegisterSound("necro/mmfire.wav");
	clh2_fxsfx_eidolon = S_RegisterSound("eidolon/spell.wav");
	clh2_fxsfx_scarabwhoosh = S_RegisterSound("misc/whoosh.wav");
	clh2_fxsfx_scarabgrab = S_RegisterSound("assassin/chn2flsh.wav");
	clh2_fxsfx_scarabhome = S_RegisterSound("assassin/chain.wav");
	clh2_fxsfx_scarabbyebye = S_RegisterSound("items/itmspawn.wav");
	clh2_fxsfx_ravensplit = S_RegisterSound("raven/split.wav");
	clh2_fxsfx_ravenfire = S_RegisterSound("raven/rfire1.wav");
	clh2_fxsfx_ravengo = S_RegisterSound("raven/ravengo.wav");
	clh2_fxsfx_drillashoot = S_RegisterSound("assassin/pincer.wav");
	clh2_fxsfx_drillaspin = S_RegisterSound("assassin/spin.wav");
	clh2_fxsfx_drillameat = S_RegisterSound("assassin/core.wav");

	clh2_fxsfx_arr2flsh = S_RegisterSound("assassin/arr2flsh.wav");
	clh2_fxsfx_arr2wood = S_RegisterSound("assassin/arr2wood.wav");
	clh2_fxsfx_met2stn = S_RegisterSound("weapons/met2stn.wav");

	clh2_fxsfx_ripple = S_RegisterSound("misc/drip.wav");
	clh2_fxsfx_splash = S_RegisterSound("raven/outwater.wav");
}

void CLH2_ClearEffects()
{
	Com_Memset(cl_common->h2_Effects, 0, sizeof(cl_common->h2_Effects));
	Com_Memset(EntityUsed, 0, sizeof(EntityUsed));
	EffectEntityCount = 0;
}

int CLH2_NewEffectEntity()
{
	if (EffectEntityCount == MAX_EFFECT_ENTITIES_H2)
	{
		return -1;
	}

	int counter;
	for (counter = 0; counter < MAX_EFFECT_ENTITIES_H2; counter++)
	{
		if (!EntityUsed[counter]) 
		{
			break;
		}
	}

	EntityUsed[counter] = true;
	EffectEntityCount++;
	effect_entity_t* ent = &EffectEntities[counter];
	Com_Memset(ent, 0, sizeof(*ent));

	return counter;
}

static void CLH2_FreeEffectEntity(int index)
{
	if (index == -1)
	{
		return;
	}
	EntityUsed[index] = false;
	EffectEntityCount--;
}

static void CLH2_FreeEffectSmoke(int index)
{
	CLH2_FreeEffectEntity(cl_common->h2_Effects[index].Smoke.entity_index);
}

static void CLH2_FreeEffectTeleSmoke1(int index)
{
	CLH2_FreeEffectEntity(cl_common->h2_Effects[index].Smoke.entity_index2);
	CLH2_FreeEffectSmoke(index);
}

static void CLH2_FreeEffectFlash(int index)
{
	CLH2_FreeEffectEntity(cl_common->h2_Effects[index].Flash.entity_index);
}

static void CLH2_FreeEffectTeleporterPuffs(int index)
{
	for (int i = 0; i < 8; i++)
	{
		CLH2_FreeEffectEntity(cl_common->h2_Effects[index].Teleporter.entity_index[i]);
	}
}

static void CLH2_FreeEffectTeleporterBody(int index)
{
	CLH2_FreeEffectEntity(cl_common->h2_Effects[index].Teleporter.entity_index[0]);
}

static void CLH2_FreeEffectMissile(int index)
{
	CLH2_FreeEffectEntity(cl_common->h2_Effects[index].Missile.entity_index);
}

static void CLH2_FreeEffectChunk(int index)
{
	for (int i = 0; i < cl_common->h2_Effects[index].Chunk.numChunks; i++)
	{
		if (cl_common->h2_Effects[index].Chunk.entity_index[i] != -1)
		{
			CLH2_FreeEffectEntity(cl_common->h2_Effects[index].Chunk.entity_index[i]);
		}
	}
}

static void CLH2_FreeEffectSheepinator(int index)
{
	for (int i = 0; i < 5; i++)
	{
		CLH2_FreeEffectEntity(cl_common->h2_Effects[index].Xbow.ent[i]);
	}
}

static void CLH2_FreeEffectXbow(int index)
{
	for (int i = 0; i < cl_common->h2_Effects[index].Xbow.bolts; i++)
	{
		CLH2_FreeEffectEntity(cl_common->h2_Effects[index].Xbow.ent[i]);
	}
}

static void CLH2_FreeEffectChain(int index)
{
	CLH2_FreeEffectEntity(cl_common->h2_Effects[index].Chain.ent1);
}

static void CLH2_FreeEffectEidolonStar(int index)
{
	CLH2_FreeEffectEntity(cl_common->h2_Effects[index].Star.ent1);
	CLH2_FreeEffectEntity(cl_common->h2_Effects[index].Star.entity_index);
}

static void CLH2_FreeEffectMissileStar(int index)
{
	CLH2_FreeEffectEntity(cl_common->h2_Effects[index].Star.ent2);
	CLH2_FreeEffectEidolonStar(index);
}

static void CLH2_FreeEffectEntityH2(int index)
{
	switch (cl_common->h2_Effects[index].type)
	{
	case H2CE_RAIN:
	case H2CE_SNOW:
	case H2CE_FOUNTAIN:
	case H2CE_QUAKE:
	case H2CE_RIDER_DEATH:
	case H2CE_GRAVITYWELL:
		break;

	case H2CE_WHITE_SMOKE:
	case H2CE_GREEN_SMOKE:
	case H2CE_GREY_SMOKE:
	case H2CE_RED_SMOKE:
	case H2CE_SLOW_WHITE_SMOKE:
	case H2CE_TELESMK1:
	case H2CE_TELESMK2:
	case H2CE_GHOST:
	case H2CE_REDCLOUD:
	case H2CE_ACID_MUZZFL:
	case H2CE_FLAMESTREAM:
	case H2CE_FLAMEWALL:
	case H2CE_FLAMEWALL2:
	case H2CE_ONFIRE:
	// Just go through animation and then remove
	case H2CE_SM_WHITE_FLASH:
	case H2CE_YELLOWRED_FLASH:
	case H2CE_BLUESPARK:
	case H2CE_YELLOWSPARK:
	case H2CE_SM_CIRCLE_EXP:
	case H2CE_BG_CIRCLE_EXP:
	case H2CE_SM_EXPLOSION:
	case H2CE_LG_EXPLOSION:
	case H2CE_FLOOR_EXPLOSION:
	case H2CE_FLOOR_EXPLOSION3:
	case H2CE_BLUE_EXPLOSION:
	case H2CE_REDSPARK:
	case H2CE_GREENSPARK:
	case H2CE_ICEHIT:
	case H2CE_MEDUSA_HIT:
	case H2CE_MEZZO_REFLECT:
	case H2CE_FLOOR_EXPLOSION2:
	case H2CE_XBOW_EXPLOSION:
	case H2CE_NEW_EXPLOSION:
	case H2CE_MAGIC_MISSILE_EXPLOSION:
	case H2CE_BONE_EXPLOSION:
	case H2CE_BLDRN_EXPL:
	case H2CE_BRN_BOUNCE:
	case H2CE_LSHOCK:
	case H2CE_ACID_HIT:
	case H2CE_ACID_SPLAT:
	case H2CE_ACID_EXPL:
	case H2CE_LBALL_EXPL:
	case H2CE_FBOOM:
	case H2CE_BOMB:
	case H2CE_FIREWALL_SMALL:
	case H2CE_FIREWALL_MEDIUM:
	case H2CE_FIREWALL_LARGE:
		CLH2_FreeEffectSmoke(index);
		break;

	// Go forward then backward through animation then remove
	case H2CE_WHITE_FLASH:
	case H2CE_BLUE_FLASH:
	case H2CE_SM_BLUE_FLASH:
	case H2CE_RED_FLASH:
		CLH2_FreeEffectFlash(index);
		break;

	case H2CE_TELEPORTERPUFFS:
		CLH2_FreeEffectTeleporterPuffs(index);
		break;

	case H2CE_TELEPORTERBODY:
		CLH2_FreeEffectTeleporterBody(index);
		break;

	case H2CE_BONESHARD:
	case H2CE_BONESHRAPNEL:
		CLH2_FreeEffectMissile(index);
		break;

	case H2CE_CHUNK:
		CLH2_FreeEffectChunk(index);
		break;
	}
}

static void CLH2_FreeEffectEntityHW(int index)
{
	switch (cl_common->h2_Effects[index].type)
	{
	case HWCE_RAIN:
	case HWCE_FOUNTAIN:
	case HWCE_QUAKE:
	case HWCE_DEATHBUBBLES:
	case HWCE_RIDER_DEATH:
		break;

	case HWCE_TELESMK1:
		CLH2_FreeEffectTeleSmoke1(index);
		break;

	case HWCE_WHITE_SMOKE:
	case HWCE_GREEN_SMOKE:
	case HWCE_GREY_SMOKE:
	case HWCE_RED_SMOKE:
	case HWCE_SLOW_WHITE_SMOKE:
	case HWCE_TELESMK2:
	case HWCE_GHOST:
	case HWCE_REDCLOUD:
	case HWCE_ACID_MUZZFL:
	case HWCE_FLAMESTREAM:
	case HWCE_FLAMEWALL:
	case HWCE_FLAMEWALL2:
	case HWCE_ONFIRE:
	case HWCE_RIPPLE:
	// Just go through animation and then remove
	case HWCE_SM_WHITE_FLASH:
	case HWCE_YELLOWRED_FLASH:
	case HWCE_BLUESPARK:
	case HWCE_YELLOWSPARK:
	case HWCE_SM_CIRCLE_EXP:
	case HWCE_BG_CIRCLE_EXP:
	case HWCE_SM_EXPLOSION:
	case HWCE_SM_EXPLOSION2:
	case HWCE_BG_EXPLOSION:
	case HWCE_FLOOR_EXPLOSION:
	case HWCE_BLUE_EXPLOSION:
	case HWCE_REDSPARK:
	case HWCE_GREENSPARK:
	case HWCE_ICEHIT:
	case HWCE_MEDUSA_HIT:
	case HWCE_MEZZO_REFLECT:
	case HWCE_FLOOR_EXPLOSION2:
	case HWCE_XBOW_EXPLOSION:
	case HWCE_NEW_EXPLOSION:
	case HWCE_MAGIC_MISSILE_EXPLOSION:
	case HWCE_BONE_EXPLOSION:
	case HWCE_BLDRN_EXPL:
	case HWCE_BRN_BOUNCE:
	case HWCE_LSHOCK:
	case HWCE_ACID_HIT:
	case HWCE_ACID_SPLAT:
	case HWCE_ACID_EXPL:
	case HWCE_LBALL_EXPL:
	case HWCE_FBOOM:
	case HWCE_BOMB:
	case HWCE_FIREWALL_SMALL:
	case HWCE_FIREWALL_MEDIUM:
	case HWCE_FIREWALL_LARGE:
		CLH2_FreeEffectSmoke(index);
		break;

	// Go forward then backward through animation then remove
	case HWCE_WHITE_FLASH:
	case HWCE_BLUE_FLASH:
	case HWCE_SM_BLUE_FLASH:
	case HWCE_HWSPLITFLASH:
	case HWCE_RED_FLASH:
		CLH2_FreeEffectFlash(index);
		break;

	case HWCE_TELEPORTERPUFFS:
		CLH2_FreeEffectTeleporterPuffs(index);
		break;

	case HWCE_TELEPORTERBODY:
		CLH2_FreeEffectTeleporterBody(index);
		break;

	case HWCE_HWSHEEPINATOR:
		CLH2_FreeEffectSheepinator(index);
		break;

	case HWCE_HWXBOWSHOOT:
		CLH2_FreeEffectXbow(index);
		break;

	case HWCE_HWDRILLA:
	case HWCE_BONESHARD:
	case HWCE_BONESHRAPNEL:
	case HWCE_HWBONEBALL:
	case HWCE_HWRAVENSTAFF:
	case HWCE_HWRAVENPOWER:
		CLH2_FreeEffectMissile(index);
		break;

	case HWCE_TRIPMINESTILL:
	case HWCE_SCARABCHAIN:
	case HWCE_TRIPMINE:
		CLH2_FreeEffectChain(index);
		break;

	case HWCE_HWMISSILESTAR:
		CLH2_FreeEffectMissileStar(index);
		break;

	case HWCE_HWEIDOLONSTAR:
		CLH2_FreeEffectEidolonStar(index);
		break;

	default:
		break;
	}
}

void CLH2_FreeEffect(int index)
{
	if (GGameType & GAME_HexenWorld)
	{
		CLH2_FreeEffectEntityHW(index);
	}
	else
	{
		CLH2_FreeEffectEntityH2(index);
	}

	Com_Memset(&cl_common->h2_Effects[index], 0, sizeof(h2EffectT));
}

static void CLH2_ParseEffectRain(int index, QMsg& message)
{
	message.ReadPos(cl_common->h2_Effects[index].Rain.min_org);
	message.ReadPos(cl_common->h2_Effects[index].Rain.max_org);
	message.ReadPos(cl_common->h2_Effects[index].Rain.e_size);
	message.ReadPos(cl_common->h2_Effects[index].Rain.dir);
	cl_common->h2_Effects[index].Rain.color = message.ReadShort();
	cl_common->h2_Effects[index].Rain.count = message.ReadShort();
	cl_common->h2_Effects[index].Rain.wait = message.ReadFloat();
}

static void CLH2_ParseEffectSnow(int index, QMsg& message)
{
	message.ReadPos(cl_common->h2_Effects[index].Snow.min_org);
	message.ReadPos(cl_common->h2_Effects[index].Snow.max_org);
	cl_common->h2_Effects[index].Snow.flags = message.ReadByte();
	message.ReadPos(cl_common->h2_Effects[index].Snow.dir);
	cl_common->h2_Effects[index].Snow.count = message.ReadByte();
}

static void CLH2_ParseEffectFountain(int index, QMsg& message)
{
	message.ReadPos(cl_common->h2_Effects[index].Fountain.pos);
	cl_common->h2_Effects[index].Fountain.angle[0] = message.ReadAngle();
	cl_common->h2_Effects[index].Fountain.angle[1] = message.ReadAngle();
	cl_common->h2_Effects[index].Fountain.angle[2] = message.ReadAngle();
	message.ReadPos(cl_common->h2_Effects[index].Fountain.movedir);
	cl_common->h2_Effects[index].Fountain.color = message.ReadShort();
	cl_common->h2_Effects[index].Fountain.cnt = message.ReadByte();

	AngleVectors(cl_common->h2_Effects[index].Fountain.angle, 
		cl_common->h2_Effects[index].Fountain.vforward,
		cl_common->h2_Effects[index].Fountain.vright,
		cl_common->h2_Effects[index].Fountain.vup);
}

static void CLH2_ParseEffectQuake(int index, QMsg& message)
{
	message.ReadPos(cl_common->h2_Effects[index].Quake.origin);
	cl_common->h2_Effects[index].Quake.radius = message.ReadFloat();
}

static bool CLH2_ParseEffectSmokeCommon(int index, QMsg& message, const char* modelName, int drawFlags)
{
	message.ReadPos(cl_common->h2_Effects[index].Smoke.origin);
	cl_common->h2_Effects[index].Smoke.velocity[0] = message.ReadFloat();
	cl_common->h2_Effects[index].Smoke.velocity[1] = message.ReadFloat();
	cl_common->h2_Effects[index].Smoke.velocity[2] = message.ReadFloat();
	cl_common->h2_Effects[index].Smoke.framelength = message.ReadFloat();
	if (!(GGameType & GAME_HexenWorld))
	{
		cl_common->h2_Effects[index].Smoke.frame = message.ReadFloat();
	}

	cl_common->h2_Effects[index].Smoke.entity_index = CLH2_NewEffectEntity();
	if (cl_common->h2_Effects[index].Smoke.entity_index == -1)
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Smoke.entity_index];
	VectorCopy(cl_common->h2_Effects[index].Smoke.origin, ent->state.origin);
	ent->model = R_RegisterModel(modelName);
	ent->state.drawflags = drawFlags;
	return true;
}

static bool CLH2_ParseEffectWhiteSmoke(int index, QMsg& message)
{
	return CLH2_ParseEffectSmokeCommon(index, message, "models/whtsmk1.spr", H2DRF_TRANSLUCENT);
}

static bool CLH2_ParseEffectGreenSmoke(int index, QMsg& message)
{
	return CLH2_ParseEffectSmokeCommon(index, message, "models/grnsmk1.spr", H2DRF_TRANSLUCENT);
}

static bool CLH2_ParseEffectGraySmoke(int index, QMsg& message)
{
	return CLH2_ParseEffectSmokeCommon(index, message, "models/grysmk1.spr", H2DRF_TRANSLUCENT);
}

static bool CLH2_ParseEffectRedSmoke(int index, QMsg& message)
{
	return CLH2_ParseEffectSmokeCommon(index, message, "models/redsmk1.spr", H2DRF_TRANSLUCENT);
}

static bool CLH2_ParseEffectTeleportSmoke1(int index, QMsg& message)
{
	if (!CLH2_ParseEffectSmokeCommon(index, message, "models/telesmk1.spr", H2DRF_TRANSLUCENT))
	{
		return false;
	}

	if (GGameType & GAME_HexenWorld)
	{
		S_StartSound(cl_common->h2_Effects[index].Smoke.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_ravenfire, 1, 1);

		if ((cl_common->h2_Effects[index].Smoke.entity_index2 = CLH2_NewEffectEntity()) != -1)
		{
			effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Smoke.entity_index2];
			VectorCopy(cl_common->h2_Effects[index].Smoke.origin, ent->state.origin);
			ent->model = R_RegisterModel("models/telesmk1.spr");
			ent->state.drawflags = H2DRF_TRANSLUCENT;
		}
	}
	return true;
}

static bool CLH2_ParseEffectTeleportSmoke2(int index, QMsg& message)
{
	return CLH2_ParseEffectSmokeCommon(index, message, "models/telesmk2.spr", H2DRF_TRANSLUCENT);
}

static bool CLH2_ParseEffectGhost(int index, QMsg& message)
{
	if (!CLH2_ParseEffectSmokeCommon(index, message, "models/ghost.spr", H2DRF_TRANSLUCENT | H2MLS_ABSLIGHT))
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Smoke.entity_index];
	ent->state.abslight = .5;
	if (GGameType & GAME_HexenWorld)
	{
		Log::write("Bad effect type %d\n", (int)cl_common->h2_Effects[index].type);
		return false;
	}
	return true;
}

static bool CLH2_ParseEffectRedCloud(int index, QMsg& message)
{
	return CLH2_ParseEffectSmokeCommon(index, message, "models/rcloud.spr", 0);
}

static bool CLH2_ParseEffectAcidMuzzleFlash(int index, QMsg& message)
{
	if (!CLH2_ParseEffectSmokeCommon(index, message, "models/muzzle1.spr", H2DRF_TRANSLUCENT | H2MLS_ABSLIGHT))
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Smoke.entity_index];
	ent->state.abslight = 0.2;
	return true;
}

static bool CLH2_ParseEffectFlameStream(int index, QMsg& message)
{
	if (!CLH2_ParseEffectSmokeCommon(index, message, "models/flamestr.spr", H2DRF_TRANSLUCENT | H2MLS_ABSLIGHT))
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Smoke.entity_index];
	ent->state.abslight = 1;
	ent->state.frame = cl_common->h2_Effects[index].Smoke.frame;
	return true;
}

static bool CLH2_ParseEffectFlameWall(int index, QMsg& message)
{
	return CLH2_ParseEffectSmokeCommon(index, message, "models/firewal1.spr", 0);
}

static bool CLH2_ParseEffectFlameWall2(int index, QMsg& message)
{
	return CLH2_ParseEffectSmokeCommon(index, message, "models/firewal2.spr", H2DRF_TRANSLUCENT);
}

static const char* CLH2_ChooseOnFireModel()
{
	int rdm = rand() & 3;
	if (rdm < 1)
	{
		return "models/firewal1.spr";
	}
	else if (rdm < 2)
	{
		return "models/firewal2.spr";
	}
	else
	{
		return "models/firewal3.spr";
	}
}

static bool CLH2_ParseEffectOnFire(int index, QMsg& message)
{
	if (!CLH2_ParseEffectSmokeCommon(index, message, CLH2_ChooseOnFireModel(), H2DRF_TRANSLUCENT))
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Smoke.entity_index];
	ent->state.abslight = 1;
	ent->state.frame = cl_common->h2_Effects[index].Smoke.frame;
	return true;
}

static bool CLHW_ParseEffectRipple(int index, QMsg& message)
{
	if (!CLH2_ParseEffectSmokeCommon(index, message, "models/ripple.spr", H2DRF_TRANSLUCENT))
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Smoke.entity_index];
	ent->state.angles[0] = 90;

	if (cl_common->h2_Effects[index].Smoke.framelength == 2)
	{
		CLH2_SplashParticleEffect(cl_common->h2_Effects[index].Smoke.origin, 200, 406 + rand() % 8, pt_h2slowgrav, 40);
		S_StartSound(cl_common->h2_Effects[index].Smoke.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_splash, 1, 1);
	}
	else if (cl_common->h2_Effects[index].Smoke.framelength == 1)
	{
		CLH2_SplashParticleEffect(cl_common->h2_Effects[index].Smoke.origin, 100, 406 + rand() % 8, pt_h2slowgrav, 20);
	}
	else
	{
		S_StartSound(cl_common->h2_Effects[index].Smoke.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_ripple, 1, 1);
	}
	cl_common->h2_Effects[index].Smoke.framelength = 0.05;
	return true;
}

static bool CLH2_ParseEffectExplosionCommon(int index, QMsg& message, const char* modelName)
{
	message.ReadPos(cl_common->h2_Effects[index].Smoke.origin);

	cl_common->h2_Effects[index].Smoke.entity_index = CLH2_NewEffectEntity();
	if (cl_common->h2_Effects[index].Smoke.entity_index == -1)
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Smoke.entity_index];
	VectorCopy(cl_common->h2_Effects[index].Smoke.origin, ent->state.origin);
	ent->model = R_RegisterModel(modelName);
	return true;
}

static bool CLH2_ParseEffectSmallWhiteFlash(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/sm_white.spr");
}

static bool CLH2_ParseEffectYellowRedFlash(int index, QMsg& message)
{
	if (!CLH2_ParseEffectExplosionCommon(index, message, "models/yr_flsh.spr"))
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Smoke.entity_index];
	ent->state.drawflags = H2DRF_TRANSLUCENT;
	return true;
}

static bool CLH2_ParseEffectBlueSpark(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/bspark.spr");
}

static bool CLH2_ParseEffectYellowSpark(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/spark.spr");
}

static bool CLH2_ParseEffectSmallCircleExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/fcircle.spr");
}

static bool CLH2_ParseEffectBigCircleExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/xplod29.spr");
}

static bool CLH2_ParseEffectSmallExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/sm_expld.spr");
}

static bool CLHW_ParseEffectSmallExplosionWithSound(int index, QMsg& message)
{
	if (!CLH2_ParseEffectSmallExplosion(index, message))
	{
		return false;
	}
	S_StartSound(cl_common->h2_Effects[index].Smoke.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_explode, 1, 1);
	return true;
}

static bool CLH2_ParseEffectBigExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/bg_expld.spr");
}

static bool CLHW_ParseEffectBigExplosionWithSound(int index, QMsg& message)
{
	if (!CLH2_ParseEffectBigExplosion(index, message))
	{
		return false;
	}
	S_StartSound(cl_common->h2_Effects[index].Smoke.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_explode, 1, 1);
	return true;
}

static bool CLH2_ParseEffectFloorExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/fl_expld.spr");
}

static bool CLH2_ParseEffectFloorExplosion2(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/flrexpl2.spr");
}

static bool CLH2_ParseEffectFloorExplosion3(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/biggy.spr");
}

static bool CLH2_ParseEffectBlueExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/xpspblue.spr");
}

static bool CLH2_ParseEffectRedSpark(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/rspark.spr");
}

static bool CLH2_ParseEffectGreenSpark(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/gspark.spr");
}

static bool CLH2_ParseEffectIceHit(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/icehit.spr");
}

static bool CLH2_ParseEffectMedusaHit(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/medhit.spr");
}

static bool CLH2_ParseEffectMezzoReflect(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/mezzoref.spr");
}

static bool CLH2_ParseEffectXBowExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/xbowexpl.spr");
}

static bool CLH2_ParseEffectNewExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/gen_expl.spr");
}

static bool CLH2_ParseEffectMagicMissileExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/mm_expld.spr");
}

static bool CLHW_ParseEffectMagicMissileExplosionWithSound(int index, QMsg& message)
{
	if (!CLH2_ParseEffectMagicMissileExplosion(index, message))
	{
		return false;
	}
	S_StartSound(cl_common->h2_Effects[index].Smoke.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_explode, 1, 1);
	return true;
}

static bool CLH2_ParseEffectBoneExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/bonexpld.spr");
}

static bool CLH2_ParseEffectBldrnExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/xplsn_1.spr");
}

static bool CLH2_ParseEffectLShock(int index, QMsg& message)
{
	if (!CLH2_ParseEffectExplosionCommon(index, message, "models/vorpshok.mdl"))
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Smoke.entity_index];
	ent->state.drawflags = H2MLS_TORCH;
	ent->state.angles[2] = 90;
	ent->state.scale = 255;
	return true;
}

static bool CLH2_ParseEffectAcidHit(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/axplsn_2.spr");
}

static bool CLH2_ParseEffectAcidSplat(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/axplsn_1.spr");
}

static bool CLH2_ParseEffectAcidExplosion(int index, QMsg& message)
{
	if (!CLH2_ParseEffectExplosionCommon(index, message, "models/axplsn_5.spr"))
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Smoke.entity_index];
	ent->state.drawflags = H2MLS_ABSLIGHT;
	ent->state.abslight = 1;
	return true;
}

static bool CLH2_ParseEffectLBallExplosion(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/Bluexp3.spr");
}

static bool CLH2_ParseEffectFBoom(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/fboom.spr");
}

static bool CLH2_ParseEffectBomb(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/pow.spr");
}

static bool CLH2_ParseEffectFirewallSall(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/firewal1.spr");
}

static bool CLH2_ParseEffectFirewallMedium(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/firewal5.spr");
}

static bool CLH2_ParseEffectFirewallLarge(int index, QMsg& message)
{
	return CLH2_ParseEffectExplosionCommon(index, message, "models/firewal4.spr");
}

static bool CLH2_ParseEffectFlashCommon(int index, QMsg& message, const char* modelName)
{
	message.ReadPos(cl_common->h2_Effects[index].Flash.origin);
	cl_common->h2_Effects[index].Flash.reverse = 0;
	if ((cl_common->h2_Effects[index].Flash.entity_index = CLH2_NewEffectEntity()) == -1)
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Flash.entity_index];
	VectorCopy(cl_common->h2_Effects[index].Flash.origin, ent->state.origin);
	ent->model = R_RegisterModel(modelName);
	ent->state.drawflags = H2DRF_TRANSLUCENT;
	return true;
}

static bool CLH2_ParseEffectWhiteFlash(int index, QMsg& message)
{
	return CLH2_ParseEffectFlashCommon(index, message, "models/gryspt.spr");
}

static bool CLH2_ParseEffectBlueFlash(int index, QMsg& message)
{
	return CLH2_ParseEffectFlashCommon(index, message, "models/bluflash.spr");
}

static bool CLH2_ParseEffectSmallBlueFlash(int index, QMsg& message)
{
	return CLH2_ParseEffectFlashCommon(index, message, "models/sm_blue.spr");
}

static bool CLHW_ParseEffectSplitFlash(int index, QMsg& message)
{
	if (!CLH2_ParseEffectSmallBlueFlash(index, message))
	{
		return false;
	}
	S_StartSound(cl_common->h2_Effects[index].Flash.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_ravensplit, 1, 1);
	return true;
}

static bool CLH2_ParseEffectRedFlash(int index, QMsg& message)
{
	return CLH2_ParseEffectFlashCommon(index, message, "models/redspt.spr");
}

static void CLH2_ParseEffectRiderDeath(int index, QMsg& message)
{
	message.ReadPos(cl_common->h2_Effects[index].RD.origin);
}

static void CLH2_ParseEffectGravityWell(int index, QMsg& message)
{
	message.ReadPos(cl_common->h2_Effects[index].GravityWell.origin);
	cl_common->h2_Effects[index].GravityWell.color = message.ReadShort();
	cl_common->h2_Effects[index].GravityWell.lifetime = message.ReadFloat();
}

static void CLH2_ParseEffectTeleporterPuffs(int index, QMsg& message)
{
	message.ReadPos(cl_common->h2_Effects[index].Teleporter.origin);

	cl_common->h2_Effects[index].Teleporter.framelength = .05;
	int dir = 0;
	for (int i = 0; i < 8; i++)
	{		
		if ((cl_common->h2_Effects[index].Teleporter.entity_index[i] = CLH2_NewEffectEntity()) == -1)
		{
			continue;
		}
		effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Teleporter.entity_index[i]];
		VectorCopy(cl_common->h2_Effects[index].Teleporter.origin, ent->state.origin);

		float angleval = dir * M_PI * 2 / 360;

		float sinval = sin(angleval);
		float cosval = cos(angleval);

		cl_common->h2_Effects[index].Teleporter.velocity[i][0] = 10 * cosval;
		cl_common->h2_Effects[index].Teleporter.velocity[i][1] = 10 * sinval;
		cl_common->h2_Effects[index].Teleporter.velocity[i][2] = 0;
		dir += 45;

		ent->model = R_RegisterModel("models/telesmk2.spr");
		ent->state.drawflags = H2DRF_TRANSLUCENT;
	}
}

static void CLH2_ParseEffectTeleporterBody(int index, QMsg& message)
{
	message.ReadPos(cl_common->h2_Effects[index].Teleporter.origin);
	cl_common->h2_Effects[index].Teleporter.velocity[0][0] = message.ReadFloat();
	cl_common->h2_Effects[index].Teleporter.velocity[0][1] = message.ReadFloat();
	cl_common->h2_Effects[index].Teleporter.velocity[0][2] = message.ReadFloat();
	float skinnum = message.ReadFloat();
	
	cl_common->h2_Effects[index].Teleporter.framelength = .05;
	if ((cl_common->h2_Effects[index].Teleporter.entity_index[0] = CLH2_NewEffectEntity()) == -1)
	{
		return;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Teleporter.entity_index[0]];
	VectorCopy(cl_common->h2_Effects[index].Teleporter.origin, ent->state.origin);

	ent->model = R_RegisterModel("models/teleport.mdl");
	ent->state.drawflags = H2SCALE_TYPE_XYONLY | H2DRF_TRANSLUCENT;
	ent->state.scale = 100;
	ent->state.skinnum = skinnum;
}

static bool CLH2_ParseEffectMissileCommon(int index, QMsg& message, const char* modelName)
{
	message.ReadPos(cl_common->h2_Effects[index].Missile.origin);
	cl_common->h2_Effects[index].Missile.velocity[0] = message.ReadFloat();
	cl_common->h2_Effects[index].Missile.velocity[1] = message.ReadFloat();
	cl_common->h2_Effects[index].Missile.velocity[2] = message.ReadFloat();
	cl_common->h2_Effects[index].Missile.angle[0] = message.ReadFloat();
	cl_common->h2_Effects[index].Missile.angle[1] = message.ReadFloat();
	cl_common->h2_Effects[index].Missile.angle[2] = message.ReadFloat();
	cl_common->h2_Effects[index].Missile.avelocity[0] = message.ReadFloat();
	cl_common->h2_Effects[index].Missile.avelocity[1] = message.ReadFloat();
	cl_common->h2_Effects[index].Missile.avelocity[2] = message.ReadFloat();

	if ((cl_common->h2_Effects[index].Missile.entity_index = CLH2_NewEffectEntity()) == -1)
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Missile.entity_index];
	VectorCopy(cl_common->h2_Effects[index].Missile.origin, ent->state.origin);
	if (GGameType & GAME_HexenWorld)
	{
		VectorCopy(cl_common->h2_Effects[index].Missile.angle, ent->state.angles);
	}
	ent->model = R_RegisterModel(modelName);
	return true;
}

static bool CLHW_ParseEffectMissileCommonNoAngles(int index, QMsg& message, const char* modelName)
{
	message.ReadPos(cl_common->h2_Effects[index].Missile.origin);
	cl_common->h2_Effects[index].Missile.velocity[0] = message.ReadFloat();
	cl_common->h2_Effects[index].Missile.velocity[1] = message.ReadFloat();
	cl_common->h2_Effects[index].Missile.velocity[2] = message.ReadFloat();

	VecToAnglesBuggy(cl_common->h2_Effects[index].Missile.velocity, cl_common->h2_Effects[index].Missile.angle);
	if ((cl_common->h2_Effects[index].Missile.entity_index = CLH2_NewEffectEntity()) == -1)
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Missile.entity_index];
	VectorCopy(cl_common->h2_Effects[index].Missile.origin, ent->state.origin);
	VectorCopy(cl_common->h2_Effects[index].Missile.angle, ent->state.angles);
	ent->model = R_RegisterModel(modelName);
	return true;
}

static bool CLH2_ParseEffectBoneShard(int index, QMsg& message)
{
	return CLH2_ParseEffectMissileCommon(index, message, "models/boneshot.mdl");
}

static bool CLHW_ParseEffectBoneShard(int index, QMsg& message)
{
	if (!CLHW_ParseEffectMissileCommonNoAngles(index, message, "models/boneshot.mdl"))
	{
		return false;
	}
	cl_common->h2_Effects[index].Missile.avelocity[0] = (rand() % 1554) - 777;
	S_StartSound(cl_common->h2_Effects[index].Missile.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_bone, 1, 1);
	return true;
}

static bool CLH2_ParseEffectBoneShrapnel(int index, QMsg& message)
{
	return CLH2_ParseEffectMissileCommon(index, message, "models/boneshrd.mdl");
}

static void CLH2_ParseEffectChunk(int index, QMsg& message)
{
	message.ReadPos(cl_common->h2_Effects[index].Chunk.origin);
	cl_common->h2_Effects[index].Chunk.type = message.ReadByte();
	cl_common->h2_Effects[index].Chunk.srcVel[0] = message.ReadCoord();
	cl_common->h2_Effects[index].Chunk.srcVel[1] = message.ReadCoord();
	cl_common->h2_Effects[index].Chunk.srcVel[2] = message.ReadCoord();
	cl_common->h2_Effects[index].Chunk.numChunks = message.ReadByte();

	CLH2_InitChunkEffect(cl_common->h2_Effects[index]);
}

static bool CLHW_ParseEffectBoneBall(int index, QMsg& message)
{
	if (!CLH2_ParseEffectMissileCommon(index, message, "models/bonelump.mdl"))
	{
		return false;
	}
	S_StartSound(cl_common->h2_Effects[index].Missile.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_bonefpow, 1, 1);
	return true;
}

static bool CLHW_ParseEffectRavenStaff(int index, QMsg& message)
{
	if (!CLHW_ParseEffectMissileCommonNoAngles(index, message, "models/vindsht1.mdl"))
	{
		return false;
	}
	cl_common->h2_Effects[index].Missile.avelocity[2] = 1000;
	return true;
}

static bool CLHW_ParseEffectRavenPower(int index, QMsg& message)
{
	if (!CLHW_ParseEffectMissileCommonNoAngles(index, message, "models/ravproj.mdl"))
	{
		return false;
	}
	S_StartSound(cl_common->h2_Effects[index].Missile.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_ravengo, 1, 1);
	return true;
}

static void CLHW_ParseEffectDeathBubbles(int index, QMsg& message)
{
	cl_common->h2_Effects[index].Bubble.owner = message.ReadShort();
	cl_common->h2_Effects[index].Bubble.offset[0] = message.ReadByte();
	cl_common->h2_Effects[index].Bubble.offset[1] = message.ReadByte();
	cl_common->h2_Effects[index].Bubble.offset[2] = message.ReadByte();
	cl_common->h2_Effects[index].Bubble.count = message.ReadByte();//num of bubbles
	cl_common->h2_Effects[index].Bubble.time_amount = 0;
}

static void CLHW_ParseEffectXBowCommon(int index, QMsg& message, vec3_t origin)
{
	message.ReadPos(origin);
	cl_common->h2_Effects[index].Xbow.angle[0] = message.ReadAngle();
	cl_common->h2_Effects[index].Xbow.angle[1] = message.ReadAngle();
	cl_common->h2_Effects[index].Xbow.angle[2] = 0;
}

static void CLHW_ParseEffectXBowThunderbolt(int index, QMsg& message, int i, vec3_t forward2)
{
	message.ReadPos(cl_common->h2_Effects[index].Xbow.origin[i]);
	vec3_t vtemp;
	vtemp[0] = message.ReadAngle();
	vtemp[1] = message.ReadAngle();
	vtemp[2] = 0;
	vec3_t right2, up2;
	AngleVectors(vtemp, forward2, right2, up2);
}

static void CLHW_InitXBowEffectEntity(int index, int i, const char* modelName)
{
	cl_common->h2_Effects[index].Xbow.ent[i] = CLH2_NewEffectEntity();
	if (cl_common->h2_Effects[index].Xbow.ent[i] == -1)
	{
		return;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Xbow.ent[i]];
	VectorCopy(cl_common->h2_Effects[index].Xbow.origin[i], ent->state.origin);
	VecToAnglesBuggy(cl_common->h2_Effects[index].Xbow.vel[i], ent->state.angles);
	ent->model = R_RegisterModel(modelName);
}

static void CLHW_ParseEffectXBowShoot(int index, QMsg& message)
{
	vec3_t origin;
	CLHW_ParseEffectXBowCommon(index, message, origin);
	cl_common->h2_Effects[index].Xbow.bolts = message.ReadByte();
	cl_common->h2_Effects[index].Xbow.randseed = message.ReadByte();
	cl_common->h2_Effects[index].Xbow.turnedbolts = message.ReadByte();
	cl_common->h2_Effects[index].Xbow.activebolts= message.ReadByte();

	setseed(cl_common->h2_Effects[index].Xbow.randseed);

	vec3_t forward, right, up;
	AngleVectors(cl_common->h2_Effects[index].Xbow.angle, forward, right, up);

	VectorNormalize(forward);
	VectorCopy(forward, cl_common->h2_Effects[index].Xbow.velocity);

	if (cl_common->h2_Effects[index].Xbow.bolts == 3)
	{
		S_StartSound(origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_xbowshoot, 1, 1);
	}
	else if (cl_common->h2_Effects[index].Xbow.bolts == 5)
	{
		S_StartSound(origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_xbowfshoot, 1, 1);
	}

	for (int i = 0; i < cl_common->h2_Effects[index].Xbow.bolts; i++)
	{
		cl_common->h2_Effects[index].Xbow.gonetime[i] = 1 + seedrand() * 2;
		cl_common->h2_Effects[index].Xbow.state[i] = 0;

		if ((1 << i) & cl_common->h2_Effects[index].Xbow.turnedbolts)
		{
			vec3_t forward2;
			CLHW_ParseEffectXBowThunderbolt(index, message, i, forward2);
			VectorScale(forward2, 800 + seedrand() * 500, cl_common->h2_Effects[index].Xbow.vel[i]);
		}
		else
		{
			VectorScale(forward, 800 + seedrand() * 500, cl_common->h2_Effects[index].Xbow.vel[i]);

			vec3_t vtemp;
			VectorScale(right, i * 100 - (cl_common->h2_Effects[index].Xbow.bolts- 1) * 50, vtemp);

			//this should only be done for deathmatch:
			VectorScale(vtemp, 0.333, vtemp);

			VectorAdd(cl_common->h2_Effects[index].Xbow.vel[i], vtemp, cl_common->h2_Effects[index].Xbow.vel[i]);

			//start me off a bit out
			VectorScale(vtemp, 0.05, cl_common->h2_Effects[index].Xbow.origin[i]);
			VectorAdd(origin, cl_common->h2_Effects[index].Xbow.origin[i], cl_common->h2_Effects[index].Xbow.origin[i]);
		}
		if (cl_common->h2_Effects[index].Xbow.bolts == 5)
		{
			CLHW_InitXBowEffectEntity(index, i, "models/flaming.mdl");
		}
		else
		{
			CLHW_InitXBowEffectEntity(index, i, "models/arrow.mdl");
		}
	}
}

static void CLHW_ParseEffectSheepinator(int index, QMsg& message)
{
	vec3_t origin;
	CLHW_ParseEffectXBowCommon(index, message, origin);
	cl_common->h2_Effects[index].Xbow.turnedbolts = message.ReadByte();
	cl_common->h2_Effects[index].Xbow.activebolts= message.ReadByte();
	cl_common->h2_Effects[index].Xbow.bolts = 5;
	cl_common->h2_Effects[index].Xbow.randseed = 0;

	vec3_t forward, right, up;
	AngleVectors(cl_common->h2_Effects[index].Xbow.angle, forward, right, up);

	VectorNormalize(forward);
	VectorCopy(forward, cl_common->h2_Effects[index].Xbow.velocity);

	for (int i = 0; i < cl_common->h2_Effects[index].Xbow.bolts; i++)
	{
		cl_common->h2_Effects[index].Xbow.gonetime[i] = 0;
		cl_common->h2_Effects[index].Xbow.state[i] = 0;

		if ((1 << i) & cl_common->h2_Effects[index].Xbow.turnedbolts)
		{
			vec3_t forward2;
			CLHW_ParseEffectXBowThunderbolt(index, message, i, forward2);
			VectorScale(forward2, 700, cl_common->h2_Effects[index].Xbow.vel[i]);
		}
		else
		{
			VectorCopy(origin, cl_common->h2_Effects[index].Xbow.origin[i]);
			VectorScale(forward, 700, cl_common->h2_Effects[index].Xbow.vel[i]);
			vec3_t vtemp;
			VectorScale(right, i * 75 - 150, vtemp);
			VectorAdd(cl_common->h2_Effects[index].Xbow.vel[i], vtemp, cl_common->h2_Effects[index].Xbow.vel[i]);
		}
		CLHW_InitXBowEffectEntity(index, i, "models/polymrph.spr");
	}
}

static bool CLHW_ParseEffectDrilla(int index, QMsg& message)
{
	message.ReadPos(cl_common->h2_Effects[index].Missile.origin);
	cl_common->h2_Effects[index].Missile.angle[0] = message.ReadAngle();
	cl_common->h2_Effects[index].Missile.angle[1] = message.ReadAngle();
	cl_common->h2_Effects[index].Missile.angle[2] = 0;
	cl_common->h2_Effects[index].Missile.speed = message.ReadShort();

	S_StartSound(cl_common->h2_Effects[index].Missile.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_drillashoot, 1, 1);

	vec3_t right, up;
	AngleVectors(cl_common->h2_Effects[index].Missile.angle, cl_common->h2_Effects[index].Missile.velocity, right, up);

	VectorScale(cl_common->h2_Effects[index].Missile.velocity, cl_common->h2_Effects[index].Missile.speed, cl_common->h2_Effects[index].Missile.velocity);

	cl_common->h2_Effects[index].Missile.entity_index = CLH2_NewEffectEntity();
	if (cl_common->h2_Effects[index].Missile.entity_index == -1)
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Missile.entity_index];
	VectorCopy(cl_common->h2_Effects[index].Missile.origin, ent->state.origin);
	VectorCopy(cl_common->h2_Effects[index].Missile.angle, ent->state.angles);
	ent->model = R_RegisterModel("models/scrbstp1.mdl");
	return true;
}

static bool CLHW_ParseEffectScarabChain(int index, QMsg& message)
{
	message.ReadPos(cl_common->h2_Effects[index].Chain.origin);
	cl_common->h2_Effects[index].Chain.owner = message.ReadShort();
	cl_common->h2_Effects[index].Chain.tag = message.ReadByte();

	cl_common->h2_Effects[index].Chain.material = cl_common->h2_Effects[index].Chain.owner >> 12;
	cl_common->h2_Effects[index].Chain.owner &= 0x0fff;
	cl_common->h2_Effects[index].Chain.height = 16;

	cl_common->h2_Effects[index].Chain.sound_time = cl_common->serverTime * 0.001;

	cl_common->h2_Effects[index].Chain.state = 0;//state 0: move slowly toward owner

	S_StartSound(cl_common->h2_Effects[index].Chain.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_scarabwhoosh, 1, 1);

	cl_common->h2_Effects[index].Chain.ent1 = CLH2_NewEffectEntity();
	if (cl_common->h2_Effects[index].Chain.ent1 == -1)
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Chain.ent1];
	VectorCopy(cl_common->h2_Effects[index].Chain.origin, ent->state.origin);
	ent->model = R_RegisterModel("models/scrbpbdy.mdl");
	return true;
}

static bool CLHW_ParseEffectTripMineCommon(int index, QMsg& message)
{
	message.ReadPos(cl_common->h2_Effects[index].Chain.origin);
	cl_common->h2_Effects[index].Chain.velocity[0] = message.ReadFloat();
	cl_common->h2_Effects[index].Chain.velocity[1] = message.ReadFloat();
	cl_common->h2_Effects[index].Chain.velocity[2] = message.ReadFloat();

	cl_common->h2_Effects[index].Chain.ent1 = CLH2_NewEffectEntity();
	if (cl_common->h2_Effects[index].Chain.ent1 == -1)
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Chain.ent1];
	ent->model = R_RegisterModel("models/twspike.mdl");
	return true;
}

static bool CLHW_ParseEffectTripMine(int index, QMsg& message)
{
	if (!CLHW_ParseEffectTripMineCommon(index, message))
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Chain.ent1];
	VectorCopy(cl_common->h2_Effects[index].Chain.origin, ent->state.origin);
	return true;
}

static bool CLHW_ParseEffectTripMineStill(int index, QMsg& message)
{
	if (!CLHW_ParseEffectTripMineCommon(index, message))
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Chain.ent1];
	VectorCopy(cl_common->h2_Effects[index].Chain.velocity, ent->state.origin);
	return true;
}

static bool CLHW_ParseEffectStarCommon(int index, QMsg& message)
{
	cl_common->h2_Effects[index].Star.origin[0] = message.ReadCoord();
	cl_common->h2_Effects[index].Star.origin[1] = message.ReadCoord();
	cl_common->h2_Effects[index].Star.origin[2] = message.ReadCoord();

	cl_common->h2_Effects[index].Star.velocity[0] = message.ReadFloat();
	cl_common->h2_Effects[index].Star.velocity[1] = message.ReadFloat();
	cl_common->h2_Effects[index].Star.velocity[2] = message.ReadFloat();

	VecToAnglesBuggy(cl_common->h2_Effects[index].Star.velocity, cl_common->h2_Effects[index].Star.angle);
	cl_common->h2_Effects[index].Missile.avelocity[2] = 300 + rand() % 300;

	cl_common->h2_Effects[index].Star.scaleDir = 1;
	cl_common->h2_Effects[index].Star.scale = 0.3;

	cl_common->h2_Effects[index].Star.entity_index = CLH2_NewEffectEntity();
	if (cl_common->h2_Effects[index].Star.entity_index == -1)
	{
		return false;
	}
	effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Star.entity_index];
	VectorCopy(cl_common->h2_Effects[index].Star.origin, ent->state.origin);
	VectorCopy(cl_common->h2_Effects[index].Star.angle, ent->state.angles);
	ent->model = R_RegisterModel("models/ball.mdl");
	return true;
}

static bool CLHW_ParseEffectMissileStar(int index, QMsg& message)
{
	bool ImmediateFree = !CLHW_ParseEffectStarCommon(index, message);

	cl_common->h2_Effects[index].Star.ent1 = CLH2_NewEffectEntity();
	if (cl_common->h2_Effects[index].Star.ent1 != -1)
	{
		effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Star.ent1];
		VectorCopy(cl_common->h2_Effects[index].Star.origin, ent->state.origin);
		ent->state.drawflags |= H2MLS_ABSLIGHT;
		ent->state.abslight = 0.5;
		ent->state.angles[2] = 90;
		ent->model = R_RegisterModel("models/star.mdl");
		ent->state.scale = 0.3;
		S_StartSound(ent->state.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_mmfire, 1, 1);
	}
	cl_common->h2_Effects[index].Star.ent2 = CLH2_NewEffectEntity();
	if (cl_common->h2_Effects[index].Star.ent2 != -1)
	{
		effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Star.ent2];
		VectorCopy(cl_common->h2_Effects[index].Star.origin, ent->state.origin);
		ent->model = R_RegisterModel("models/star.mdl");
		ent->state.drawflags |= H2MLS_ABSLIGHT;
		ent->state.abslight = 0.5;
		ent->state.scale = 0.3;
	}
	return !ImmediateFree;
}

static bool CLHW_ParseEffectEidolonStar(int index, QMsg& message)
{
	bool ImmediateFree = !CLHW_ParseEffectStarCommon(index, message);

	if ((cl_common->h2_Effects[index].Star.ent1 = CLH2_NewEffectEntity()) != -1)
	{
		effect_entity_t* ent = &EffectEntities[cl_common->h2_Effects[index].Star.ent1];
		VectorCopy(cl_common->h2_Effects[index].Star.origin, ent->state.origin);
		ent->state.drawflags |= H2MLS_ABSLIGHT;
		ent->state.abslight = 0.5;
		ent->state.angles[2] = 90;
		ent->model = R_RegisterModel("models/glowball.mdl");
		S_StartSound(ent->state.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_eidolon, 1, 1);
	}
	return !ImmediateFree;
}

static bool CLH2_ParseEffectType(int index, QMsg& message)
{
	bool ImmediateFree = false;
	switch (cl_common->h2_Effects[index].type)
	{
	case H2CE_RAIN:
		CLH2_ParseEffectRain(index, message);
		break;
	case H2CE_SNOW:
		CLH2_ParseEffectSnow(index, message);
		break;
	case H2CE_FOUNTAIN:
		CLH2_ParseEffectFountain(index, message);
		break;
	case H2CE_QUAKE:
		CLH2_ParseEffectQuake(index, message);
		break;
	case H2CE_WHITE_SMOKE:
	case H2CE_SLOW_WHITE_SMOKE:
		ImmediateFree = CLH2_ParseEffectWhiteSmoke(index, message);
		break;
	case H2CE_GREEN_SMOKE:
		ImmediateFree = !CLH2_ParseEffectGreenSmoke(index, message);
		break;
	case H2CE_GREY_SMOKE:
		ImmediateFree = !CLH2_ParseEffectGraySmoke(index, message);
		break;
	case H2CE_RED_SMOKE:
		ImmediateFree = !CLH2_ParseEffectRedSmoke(index, message);
		break;
	case H2CE_TELESMK1:
		ImmediateFree = !CLH2_ParseEffectTeleportSmoke1(index, message);
		break;
	case H2CE_TELESMK2:
		ImmediateFree = !CLH2_ParseEffectTeleportSmoke2(index, message);
		break;
	case H2CE_GHOST:
		ImmediateFree = !CLH2_ParseEffectGhost(index, message);
		break;
	case H2CE_REDCLOUD:
		ImmediateFree = !CLH2_ParseEffectRedCloud(index, message);
		break;
	case H2CE_ACID_MUZZFL:
		ImmediateFree = !CLH2_ParseEffectAcidMuzzleFlash(index, message);
		break;
	case H2CE_FLAMESTREAM:
		ImmediateFree = !CLH2_ParseEffectFlameStream(index, message);
		break;
	case H2CE_FLAMEWALL:
		ImmediateFree = !CLH2_ParseEffectFlameWall(index, message);
		break;
	case H2CE_FLAMEWALL2:
		ImmediateFree = !CLH2_ParseEffectFlameWall2(index, message);
		break;
	case H2CE_ONFIRE:
		ImmediateFree = !CLH2_ParseEffectOnFire(index, message);
		break;
	case H2CE_SM_WHITE_FLASH:
		ImmediateFree = !CLH2_ParseEffectSmallWhiteFlash(index, message);
		break;
	case H2CE_YELLOWRED_FLASH:
		ImmediateFree = !CLH2_ParseEffectYellowRedFlash(index, message);
		break;
	case H2CE_BLUESPARK:
		ImmediateFree = !CLH2_ParseEffectBlueSpark(index, message);
		break;
	case H2CE_YELLOWSPARK:
	case H2CE_BRN_BOUNCE:
		ImmediateFree = !CLH2_ParseEffectYellowSpark(index, message);
		break;
	case H2CE_SM_CIRCLE_EXP:
		ImmediateFree = !CLH2_ParseEffectSmallCircleExplosion(index, message);
		break;
	case H2CE_BG_CIRCLE_EXP:
		ImmediateFree = !CLH2_ParseEffectBigCircleExplosion(index, message);
		break;
	case H2CE_SM_EXPLOSION:
		ImmediateFree = !CLH2_ParseEffectSmallExplosion(index, message);
		break;
	case H2CE_LG_EXPLOSION:
		ImmediateFree = !CLH2_ParseEffectBigExplosion(index, message);
		break;
	case H2CE_FLOOR_EXPLOSION:
		ImmediateFree = !CLH2_ParseEffectFloorExplosion(index, message);
		break;
	case H2CE_FLOOR_EXPLOSION3:
		ImmediateFree = !CLH2_ParseEffectFloorExplosion3(index, message);
		break;
	case H2CE_BLUE_EXPLOSION:
		ImmediateFree = !CLH2_ParseEffectBlueExplosion(index, message);
		break;
	case H2CE_REDSPARK:
		ImmediateFree = !CLH2_ParseEffectRedSpark(index, message);
		break;
	case H2CE_GREENSPARK:
		ImmediateFree = !CLH2_ParseEffectGreenSpark(index, message);
		break;
	case H2CE_ICEHIT:
		ImmediateFree = !CLH2_ParseEffectIceHit(index, message);
		break;
	case H2CE_MEDUSA_HIT:
		ImmediateFree = !CLH2_ParseEffectMedusaHit(index, message);
		break;
	case H2CE_MEZZO_REFLECT:
		ImmediateFree = !CLH2_ParseEffectMezzoReflect(index, message);
		break;
	case H2CE_FLOOR_EXPLOSION2:
		ImmediateFree = !CLH2_ParseEffectFloorExplosion2(index, message);
		break;
	case H2CE_XBOW_EXPLOSION:
		ImmediateFree = !CLH2_ParseEffectXBowExplosion(index, message);
		break;
	case H2CE_NEW_EXPLOSION:
		ImmediateFree = !CLH2_ParseEffectNewExplosion(index, message);
		break;
	case H2CE_MAGIC_MISSILE_EXPLOSION:
		ImmediateFree = !CLH2_ParseEffectMagicMissileExplosion(index, message);
		break;
	case H2CE_BONE_EXPLOSION:
		ImmediateFree = !CLH2_ParseEffectBoneExplosion(index, message);
		break;
	case H2CE_BLDRN_EXPL:
		ImmediateFree = !CLH2_ParseEffectBldrnExplosion(index, message);
		break;
	case H2CE_LSHOCK:
		ImmediateFree = !CLH2_ParseEffectLShock(index, message);
		break;
	case H2CE_ACID_HIT:
		ImmediateFree = !CLH2_ParseEffectAcidHit(index, message);
		break;
	case H2CE_ACID_SPLAT:
		ImmediateFree = !CLH2_ParseEffectAcidSplat(index, message);
		break;
	case H2CE_ACID_EXPL:
		ImmediateFree = !CLH2_ParseEffectAcidExplosion(index, message);
		break;
	case H2CE_LBALL_EXPL:
		ImmediateFree = !CLH2_ParseEffectLBallExplosion(index, message);
		break;
	case H2CE_FBOOM:
		ImmediateFree = !CLH2_ParseEffectFBoom(index, message);
		break;
	case H2CE_BOMB:
		ImmediateFree = !CLH2_ParseEffectBomb(index, message);
		break;
	case H2CE_FIREWALL_SMALL:
		ImmediateFree = !CLH2_ParseEffectFirewallSall(index, message);
		break;
	case H2CE_FIREWALL_MEDIUM:
		ImmediateFree = !CLH2_ParseEffectFirewallMedium(index, message);
		break;
	case H2CE_FIREWALL_LARGE:
		ImmediateFree = !CLH2_ParseEffectFirewallLarge(index, message);
		break;
	case H2CE_WHITE_FLASH:
		ImmediateFree = !CLH2_ParseEffectWhiteFlash(index, message);
		break;
	case H2CE_BLUE_FLASH:
		ImmediateFree = !CLH2_ParseEffectBlueFlash(index, message);
		break;
	case H2CE_SM_BLUE_FLASH:
		ImmediateFree = !CLH2_ParseEffectSmallBlueFlash(index, message);
		break;
	case H2CE_RED_FLASH:
		ImmediateFree = !CLH2_ParseEffectRedFlash(index, message);
		break;
	case H2CE_RIDER_DEATH:
		CLH2_ParseEffectRiderDeath(index, message);
		break;
	case H2CE_GRAVITYWELL:
		CLH2_ParseEffectGravityWell(index, message);
		break;
	case H2CE_TELEPORTERPUFFS:
		CLH2_ParseEffectTeleporterPuffs(index, message);
		break;
	case H2CE_TELEPORTERBODY:
		CLH2_ParseEffectTeleporterBody(index, message);
		break;
	case H2CE_BONESHARD:
		ImmediateFree = !CLH2_ParseEffectBoneShard(index, message);
		break;
	case H2CE_BONESHRAPNEL:
		ImmediateFree = !CLH2_ParseEffectBoneShrapnel(index, message);
		break;
	case H2CE_CHUNK:
		CLH2_ParseEffectChunk(index, message);
		break;
	default:
		throw Exception("CL_ParseEffect: bad type");
	}
	return ImmediateFree;
}

static bool CLHW_ParseEffectType(int index, QMsg& message)
{
	bool ImmediateFree = false;
	switch (cl_common->h2_Effects[index].type)
	{
	case HWCE_RAIN:
		CLH2_ParseEffectRain(index, message);
		break;
	case HWCE_FOUNTAIN:
		CLH2_ParseEffectFountain(index, message);
		break;
	case HWCE_QUAKE:
		CLH2_ParseEffectQuake(index, message);
		break;
	case HWCE_WHITE_SMOKE:
	case HWCE_SLOW_WHITE_SMOKE:
		ImmediateFree = !CLH2_ParseEffectWhiteSmoke(index, message);
		break;
	case HWCE_GREEN_SMOKE:
		ImmediateFree = !CLH2_ParseEffectGreenSmoke(index, message);
		break;
	case HWCE_GREY_SMOKE:
		ImmediateFree = !CLH2_ParseEffectGraySmoke(index, message);
		break;
	case HWCE_RED_SMOKE:
		ImmediateFree = !CLH2_ParseEffectRedSmoke(index, message);
		break;
	case HWCE_TELESMK1:
		ImmediateFree = !CLH2_ParseEffectTeleportSmoke1(index, message);
		break;
	case HWCE_TELESMK2:
		ImmediateFree = !CLH2_ParseEffectTeleportSmoke2(index, message);
		break;
	case HWCE_GHOST:
		ImmediateFree = !CLH2_ParseEffectGhost(index, message);
		break;
	case HWCE_REDCLOUD:
		ImmediateFree = !CLH2_ParseEffectRedCloud(index, message);
		break;
	case HWCE_ACID_MUZZFL:
		ImmediateFree = !CLH2_ParseEffectAcidMuzzleFlash(index, message);
		break;
	case HWCE_FLAMESTREAM:
		ImmediateFree = !CLH2_ParseEffectFlameStream(index, message);
		break;
	case HWCE_FLAMEWALL:
		ImmediateFree = !CLH2_ParseEffectFlameWall(index, message);
		break;
	case HWCE_FLAMEWALL2:
		ImmediateFree = !CLH2_ParseEffectFlameWall2(index, message);
		break;
	case HWCE_ONFIRE:
		ImmediateFree = !CLH2_ParseEffectOnFire(index, message);
		break;
	case HWCE_RIPPLE:
		ImmediateFree = !CLHW_ParseEffectRipple(index, message);
		break;
	case HWCE_SM_WHITE_FLASH:
		ImmediateFree = !CLH2_ParseEffectSmallWhiteFlash(index, message);
		break;
	case HWCE_YELLOWRED_FLASH:
		ImmediateFree = !CLH2_ParseEffectYellowRedFlash(index, message);
		break;
	case HWCE_BLUESPARK:
		ImmediateFree = !CLH2_ParseEffectBlueSpark(index, message);
		break;
	case HWCE_YELLOWSPARK:
	case HWCE_BRN_BOUNCE:
		ImmediateFree = !CLH2_ParseEffectYellowSpark(index, message);
		break;
	case HWCE_SM_CIRCLE_EXP:
		ImmediateFree = !CLH2_ParseEffectSmallCircleExplosion(index, message);
		break;
	case HWCE_BG_CIRCLE_EXP:
		ImmediateFree = !CLH2_ParseEffectBigCircleExplosion(index, message);
		break;
	case HWCE_SM_EXPLOSION:
		ImmediateFree = !CLH2_ParseEffectSmallExplosion(index, message);
		break;
	case HWCE_SM_EXPLOSION2:
		ImmediateFree = !CLHW_ParseEffectSmallExplosionWithSound(index, message);
		break;
	case HWCE_BG_EXPLOSION:
		ImmediateFree = !CLHW_ParseEffectBigExplosionWithSound(index, message);
		break;
	case HWCE_FLOOR_EXPLOSION:
		ImmediateFree = !CLH2_ParseEffectFloorExplosion(index, message);
		break;
	case HWCE_BLUE_EXPLOSION:
		ImmediateFree = !CLH2_ParseEffectBlueExplosion(index, message);
		break;
	case HWCE_REDSPARK:
		ImmediateFree = !CLH2_ParseEffectRedSpark(index, message);
		break;
	case HWCE_GREENSPARK:
		ImmediateFree = !CLH2_ParseEffectGreenSpark(index, message);
		break;
	case HWCE_ICEHIT:
		ImmediateFree = !CLH2_ParseEffectIceHit(index, message);
		break;
	case HWCE_MEDUSA_HIT:
		ImmediateFree = !CLH2_ParseEffectMedusaHit(index, message);
		break;
	case HWCE_MEZZO_REFLECT:
		ImmediateFree = !CLH2_ParseEffectMezzoReflect(index, message);
		break;
	case HWCE_FLOOR_EXPLOSION2:
		ImmediateFree = !CLH2_ParseEffectFloorExplosion2(index, message);
		break;
	case HWCE_XBOW_EXPLOSION:
		ImmediateFree = !CLH2_ParseEffectXBowExplosion(index, message);
		break;
	case HWCE_NEW_EXPLOSION:
		ImmediateFree = !CLH2_ParseEffectNewExplosion(index, message);
		break;
	case HWCE_MAGIC_MISSILE_EXPLOSION:
		ImmediateFree = !CLHW_ParseEffectMagicMissileExplosionWithSound(index, message);
		break;
	case HWCE_BONE_EXPLOSION:
		ImmediateFree = !CLH2_ParseEffectBoneExplosion(index, message);
		break;
	case HWCE_BLDRN_EXPL:
		ImmediateFree = !CLH2_ParseEffectBldrnExplosion(index, message);
		break;
	case HWCE_LSHOCK:
		ImmediateFree = !CLH2_ParseEffectLShock(index, message);
		break;
	case HWCE_ACID_HIT:
		ImmediateFree = !CLH2_ParseEffectAcidHit(index, message);
		break;
	case HWCE_ACID_SPLAT:
		ImmediateFree = !CLH2_ParseEffectAcidSplat(index, message);
		break;
	case HWCE_ACID_EXPL:
		ImmediateFree = !CLH2_ParseEffectAcidExplosion(index, message);
		break;
	case HWCE_LBALL_EXPL:
		ImmediateFree = !CLH2_ParseEffectLBallExplosion(index, message);
		break;
	case HWCE_FBOOM:
		ImmediateFree = !CLH2_ParseEffectFBoom(index, message);
		break;
	case HWCE_BOMB:
		ImmediateFree = !CLH2_ParseEffectBomb(index, message);
		break;
	case HWCE_FIREWALL_SMALL:
		ImmediateFree = !CLH2_ParseEffectFirewallSall(index, message);
		break;
	case HWCE_FIREWALL_MEDIUM:
		ImmediateFree = !CLH2_ParseEffectFirewallMedium(index, message);
		break;
	case HWCE_FIREWALL_LARGE:
		ImmediateFree = !CLH2_ParseEffectFirewallLarge(index, message);
		break;
	case HWCE_WHITE_FLASH:
		ImmediateFree = !CLH2_ParseEffectWhiteFlash(index, message);
		break;
	case HWCE_BLUE_FLASH:
		ImmediateFree = !CLH2_ParseEffectBlueFlash(index, message);
		break;
	case HWCE_SM_BLUE_FLASH:
		ImmediateFree = !CLH2_ParseEffectSmallBlueFlash(index, message);
		break;
	case HWCE_HWSPLITFLASH:
		ImmediateFree = !CLHW_ParseEffectSplitFlash(index, message);
		break;
	case HWCE_RED_FLASH:
		ImmediateFree = !CLH2_ParseEffectRedFlash(index, message);
		break;
	case HWCE_RIDER_DEATH:
		CLH2_ParseEffectRiderDeath(index, message);
		break;
	case HWCE_TELEPORTERPUFFS:
		CLH2_ParseEffectTeleporterPuffs(index, message);
		break;
	case HWCE_TELEPORTERBODY:
		CLH2_ParseEffectTeleporterBody(index, message);
		break;
	case HWCE_BONESHRAPNEL:
		ImmediateFree = !CLH2_ParseEffectBoneShrapnel(index, message);
		break;
	case HWCE_HWBONEBALL:
		ImmediateFree = !CLHW_ParseEffectBoneBall(index, message);
		break;
	case HWCE_BONESHARD:
		ImmediateFree = !CLHW_ParseEffectBoneShard(index, message);
		break;
	case HWCE_HWRAVENSTAFF:
		ImmediateFree = !CLHW_ParseEffectRavenStaff(index, message);
		break;
	case HWCE_HWRAVENPOWER:
		ImmediateFree = !CLHW_ParseEffectRavenPower(index, message);
		break;
	case HWCE_DEATHBUBBLES:
		CLHW_ParseEffectDeathBubbles(index, message);
		break;
	case HWCE_HWXBOWSHOOT:
		CLHW_ParseEffectXBowShoot(index, message);
		break;
	case HWCE_HWSHEEPINATOR:
		CLHW_ParseEffectSheepinator(index, message);
		break;
	case HWCE_HWDRILLA:
		ImmediateFree = !CLHW_ParseEffectDrilla(index, message);
		break;
	case HWCE_SCARABCHAIN:
		ImmediateFree = !CLHW_ParseEffectScarabChain(index, message);
		break;
	case HWCE_TRIPMINE:
		ImmediateFree = !CLHW_ParseEffectTripMine(index, message);
		break;
	case HWCE_TRIPMINESTILL:
		ImmediateFree = !CLHW_ParseEffectTripMineStill(index, message);
		break;
	case HWCE_HWMISSILESTAR:
		ImmediateFree = !CLHW_ParseEffectMissileStar(index, message);
		break;
	case HWCE_HWEIDOLONSTAR:
		ImmediateFree = !CLHW_ParseEffectEidolonStar(index, message);
		break;
	default:
		throw Exception("CL_ParseEffect: bad type");
	}
	return ImmediateFree;
}

void CLH2_ParseEffect(QMsg& message)
{
	int index = message.ReadByte();
	if (cl_common->h2_Effects[index].type)
	{
		CLH2_FreeEffect(index);
	}

	Com_Memset(&cl_common->h2_Effects[index], 0, sizeof(h2EffectT));

	cl_common->h2_Effects[index].type = message.ReadByte();

	bool ImmediateFree;
	if (!(GGameType & GAME_HexenWorld))
	{
		ImmediateFree = CLH2_ParseEffectType(index, message);
	}
	else
	{
		ImmediateFree = CLHW_ParseEffectType(index, message);
	}

	if (ImmediateFree)
	{
		cl_common->h2_Effects[index].type = HWCE_NONE;
	}
}