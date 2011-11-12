
//**************************************************************************
//**
//** cl_effect.c
//**
//** Client side effects.
//**
//** $Header: /HexenWorld/Client/cl_effect.c 89    5/25/98 1:29p Mgummelt $
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "quakedef.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------
extern void CreateStream(int type, int ent, int flags, int tag, float duration, int skin, vec3_t source, vec3_t dest);
extern void CLTENT_XbowImpact(vec3_t pos, vec3_t vel, int chType, int damage, int arrowType);//so xbow effect can use tents
extern void CLTENT_SpawnDeathBubble(vec3_t pos);
h2entity_state_t *FindState(int EntNum);
void setseed(unsigned int seed);
float seedrand(void);

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
// CL_ParseEffect
//
//==========================================================================

// All changes need to be in SV_SendEffect(), SV_ParseEffect(),
// CL_ParseEffect()
void CL_ParseEffect(void)
{
	int index,i;
	qboolean ImmediateFree;
	effect_entity_t* ent;
	int dir;
	float	angleval, sinval, cosval;
	float skinnum;
	vec3_t forward, right, up, vtemp;
	vec3_t forward2, right2, up2;
	vec3_t origin;

	ImmediateFree = false;

	index = net_message.ReadByte();
	if (cl.h2_Effects[index].type)
		CLH2_FreeEffect(index);

	Com_Memset(&cl.h2_Effects[index],0,sizeof(struct h2EffectT));

	cl.h2_Effects[index].type = net_message.ReadByte();

	switch(cl.h2_Effects[index].type)
	{
		case HWCE_RAIN:
			CLH2_ParseEffectRain(index, net_message);
			break;

		case HWCE_FOUNTAIN:
			CLH2_ParseEffectFountain(index, net_message);
			break;

		case HWCE_QUAKE:
			CLH2_ParseEffectQuake(index, net_message);
			break;

		case HWCE_WHITE_SMOKE:
		case HWCE_SLOW_WHITE_SMOKE:
			ImmediateFree = !CLH2_ParseEffectWhiteSmoke(index, net_message);
			break;
		case HWCE_GREEN_SMOKE:
			ImmediateFree = !CLH2_ParseEffectGreenSmoke(index, net_message);
			break;
		case HWCE_GREY_SMOKE:
			ImmediateFree = !CLH2_ParseEffectGraySmoke(index, net_message);
			break;
		case HWCE_RED_SMOKE:
			ImmediateFree = !CLH2_ParseEffectRedSmoke(index, net_message);
			break;
		case HWCE_TELESMK1:
			ImmediateFree = !CLH2_ParseEffectTeleportSmoke1(index, net_message);
			break;
		case HWCE_TELESMK2:
			ImmediateFree = !CLH2_ParseEffectTeleportSmoke2(index, net_message);
			break;
		case HWCE_GHOST:
			ImmediateFree = !CLH2_ParseEffectGhost(index, net_message);
			break;
		case HWCE_REDCLOUD:
			ImmediateFree = !CLH2_ParseEffectRedCloud(index, net_message);
			break;
		case HWCE_ACID_MUZZFL:
			ImmediateFree = !CLH2_ParseEffectAcidMuzzleFlash(index, net_message);
			break;
		case HWCE_FLAMESTREAM:
			ImmediateFree = !CLH2_ParseEffectFlameStream(index, net_message);
			break;
		case HWCE_FLAMEWALL:
			ImmediateFree = !CLH2_ParseEffectFlameWall(index, net_message);
			break;
		case HWCE_FLAMEWALL2:
			ImmediateFree = !CLH2_ParseEffectFlameWall2(index, net_message);
			break;
		case HWCE_ONFIRE:
			ImmediateFree = !CLH2_ParseEffectOnFire(index, net_message);
			break;
		case HWCE_RIPPLE:
			ImmediateFree = !CLHW_ParseEffectRipple(index, net_message);
			break;

		case HWCE_SM_WHITE_FLASH:
			ImmediateFree = !CLH2_ParseEffectSmallWhiteFlash(index, net_message);
			break;
		case HWCE_YELLOWRED_FLASH:
			ImmediateFree = !CLH2_ParseEffectYellowRedFlash(index, net_message);
			break;
		case HWCE_BLUESPARK:
			ImmediateFree = !CLH2_ParseEffectBlueSpark(index, net_message);
			break;
		case HWCE_YELLOWSPARK:
		case HWCE_BRN_BOUNCE:
			ImmediateFree = !CLH2_ParseEffectYellowSpark(index, net_message);
			break;
		case HWCE_SM_CIRCLE_EXP:
			ImmediateFree = !CLH2_ParseEffectSmallCircleExplosion(index, net_message);
			break;
		case HWCE_BG_CIRCLE_EXP:
			ImmediateFree = !CLH2_ParseEffectBigCircleExplosion(index, net_message);
			break;
		case HWCE_SM_EXPLOSION:
			ImmediateFree = !CLH2_ParseEffectSmallExplosion(index, net_message);
			break;
		case HWCE_SM_EXPLOSION2:
			ImmediateFree = !CLH2_ParseEffectSmallExplosionWithSound(index, net_message);
			break;
		case HWCE_BG_EXPLOSION:
			ImmediateFree = !CLH2_ParseEffectBigExplosionWithSound(index, net_message);
			break;
		case HWCE_FLOOR_EXPLOSION:
			ImmediateFree = !CLH2_ParseEffectFloorExplosion(index, net_message);
			break;
		case HWCE_BLUE_EXPLOSION:
			ImmediateFree = !CLH2_ParseEffectBlueExplosion(index, net_message);
			break;
		case HWCE_REDSPARK:
			ImmediateFree = !CLH2_ParseEffectRedSpark(index, net_message);
			break;
		case HWCE_GREENSPARK:
			ImmediateFree = !CLH2_ParseEffectGreenSpark(index, net_message);
			break;
		case HWCE_ICEHIT:
			ImmediateFree = !CLH2_ParseEffectIceHit(index, net_message);
			break;
		case HWCE_MEDUSA_HIT:
			ImmediateFree = !CLH2_ParseEffectMedusaHit(index, net_message);
			break;
		case HWCE_MEZZO_REFLECT:
			ImmediateFree = !CLH2_ParseEffectMezzoReflect(index, net_message);
			break;
		case HWCE_FLOOR_EXPLOSION2:
			ImmediateFree = !CLH2_ParseEffectFloorExplosion2(index, net_message);
			break;
		case HWCE_XBOW_EXPLOSION:
			ImmediateFree = !CLH2_ParseEffectXBowExplosion(index, net_message);
			break;
		case HWCE_NEW_EXPLOSION:
			ImmediateFree = !CLH2_ParseEffectNewExplosion(index, net_message);
			break;
		case HWCE_MAGIC_MISSILE_EXPLOSION:
			ImmediateFree = !CLH2_ParseEffectMagicMissileExplosionWithSound(index, net_message);
			break;
		case HWCE_BONE_EXPLOSION:
			ImmediateFree = !CLH2_ParseEffectBoneExplosion(index, net_message);
			break;
		case HWCE_BLDRN_EXPL:
			ImmediateFree = !CLH2_ParseEffectBldrnExplosion(index, net_message);
			break;
		case HWCE_LSHOCK:
			ImmediateFree = !CLH2_ParseEffectLShock(index, net_message);
			break;
		case HWCE_ACID_HIT:
			ImmediateFree = !CLH2_ParseEffectAcidHit(index, net_message);
			break;
		case HWCE_ACID_SPLAT:
			ImmediateFree = !CLH2_ParseEffectAcidSplat(index, net_message);
			break;
		case HWCE_ACID_EXPL:
			ImmediateFree = !CLH2_ParseEffectAcidExplosion(index, net_message);
			break;
		case HWCE_LBALL_EXPL:
			ImmediateFree = !CLH2_ParseEffectLBallExplosion(index, net_message);
			break;
		case HWCE_FBOOM:
			ImmediateFree = !CLH2_ParseEffectFBoom(index, net_message);
			break;
		case HWCE_BOMB:
			ImmediateFree = !CLH2_ParseEffectBomb(index, net_message);
			break;
		case HWCE_FIREWALL_SMALL:
			ImmediateFree = !CLH2_ParseEffectFirewallSall(index, net_message);
			break;
		case HWCE_FIREWALL_MEDIUM:
			ImmediateFree = !CLH2_ParseEffectFirewallMedium(index, net_message);
			break;
		case HWCE_FIREWALL_LARGE:
			ImmediateFree = !CLH2_ParseEffectFirewallLarge(index, net_message);
			break;

		case HWCE_WHITE_FLASH:
		case HWCE_BLUE_FLASH:
		case HWCE_SM_BLUE_FLASH:
		case HWCE_HWSPLITFLASH:
		case HWCE_RED_FLASH:
			cl.h2_Effects[index].Flash.origin[0] = net_message.ReadCoord ();
			cl.h2_Effects[index].Flash.origin[1] = net_message.ReadCoord ();
			cl.h2_Effects[index].Flash.origin[2] = net_message.ReadCoord ();
			cl.h2_Effects[index].Flash.reverse = 0;
			if ((cl.h2_Effects[index].Flash.entity_index = CLH2_NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.h2_Effects[index].Flash.entity_index];
				VectorCopy(cl.h2_Effects[index].Flash.origin, ent->state.origin);

				if (cl.h2_Effects[index].type == HWCE_WHITE_FLASH)
					ent->model = R_RegisterModel("models/gryspt.spr");
				else if (cl.h2_Effects[index].type == HWCE_BLUE_FLASH)
					ent->model = R_RegisterModel("models/bluflash.spr");
				else if (cl.h2_Effects[index].type == HWCE_SM_BLUE_FLASH)
					ent->model = R_RegisterModel("models/sm_blue.spr");
				else if (cl.h2_Effects[index].type == HWCE_RED_FLASH)
					ent->model = R_RegisterModel("models/redspt.spr");
				else if (cl.h2_Effects[index].type == HWCE_HWSPLITFLASH)
				{
					ent->model = R_RegisterModel("models/sm_blue.spr");
					S_StartSound(cl.h2_Effects[index].Flash.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_ravensplit, 1, 1);
				}
				ent->state.drawflags = H2DRF_TRANSLUCENT;

			}
			else
			{
				ImmediateFree = true;
			}
			break;

		case HWCE_RIDER_DEATH:
			cl.h2_Effects[index].RD.origin[0] = net_message.ReadCoord ();
			cl.h2_Effects[index].RD.origin[1] = net_message.ReadCoord ();
			cl.h2_Effects[index].RD.origin[2] = net_message.ReadCoord ();
			break;

		case HWCE_TELEPORTERPUFFS:
			cl.h2_Effects[index].Teleporter.origin[0] = net_message.ReadCoord ();
			cl.h2_Effects[index].Teleporter.origin[1] = net_message.ReadCoord ();
			cl.h2_Effects[index].Teleporter.origin[2] = net_message.ReadCoord ();
				
			cl.h2_Effects[index].Teleporter.framelength = .05;
			dir = 0;
			for (i=0;i<8;++i)
			{		
				if ((cl.h2_Effects[index].Teleporter.entity_index[i] = CLH2_NewEffectEntity()) != -1)
				{
					ent = &EffectEntities[cl.h2_Effects[index].Teleporter.entity_index[i]];
					VectorCopy(cl.h2_Effects[index].Teleporter.origin, ent->state.origin);

					angleval = dir * M_PI*2 / 360;

					sinval = sin(angleval);
					cosval = cos(angleval);

					cl.h2_Effects[index].Teleporter.velocity[i][0] = 10*cosval;
					cl.h2_Effects[index].Teleporter.velocity[i][1] = 10*sinval;
					cl.h2_Effects[index].Teleporter.velocity[i][2] = 0;
					dir += 45;

					ent->model = R_RegisterModel("models/telesmk2.spr");
					ent->state.drawflags = H2DRF_TRANSLUCENT;
				}
			}
			break;

		case HWCE_TELEPORTERBODY:
			cl.h2_Effects[index].Teleporter.origin[0] = net_message.ReadCoord ();
			cl.h2_Effects[index].Teleporter.origin[1] = net_message.ReadCoord ();
			cl.h2_Effects[index].Teleporter.origin[2] = net_message.ReadCoord ();

			cl.h2_Effects[index].Teleporter.velocity[0][0] = net_message.ReadFloat ();
			cl.h2_Effects[index].Teleporter.velocity[0][1] = net_message.ReadFloat ();
			cl.h2_Effects[index].Teleporter.velocity[0][2] = net_message.ReadFloat ();

			skinnum = net_message.ReadFloat ();
			
			cl.h2_Effects[index].Teleporter.framelength = .05;
			dir = 0;
			if ((cl.h2_Effects[index].Teleporter.entity_index[0] = CLH2_NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.h2_Effects[index].Teleporter.entity_index[0]];
				VectorCopy(cl.h2_Effects[index].Teleporter.origin, ent->state.origin);

				ent->model = R_RegisterModel("models/teleport.mdl");
				ent->state.drawflags = H2SCALE_TYPE_XYONLY | H2DRF_TRANSLUCENT;
				ent->state.scale = 100;
				ent->state.skinnum = skinnum;
			}
			break;

		case HWCE_BONESHRAPNEL:
		case HWCE_HWBONEBALL:
			cl.h2_Effects[index].Missile.origin[0] = net_message.ReadCoord ();
			cl.h2_Effects[index].Missile.origin[1] = net_message.ReadCoord ();
			cl.h2_Effects[index].Missile.origin[2] = net_message.ReadCoord ();

			cl.h2_Effects[index].Missile.velocity[0] = net_message.ReadFloat ();
			cl.h2_Effects[index].Missile.velocity[1] = net_message.ReadFloat ();
			cl.h2_Effects[index].Missile.velocity[2] = net_message.ReadFloat ();

			cl.h2_Effects[index].Missile.angle[0] = net_message.ReadFloat ();
			cl.h2_Effects[index].Missile.angle[1] = net_message.ReadFloat ();
			cl.h2_Effects[index].Missile.angle[2] = net_message.ReadFloat ();

			cl.h2_Effects[index].Missile.avelocity[0] = net_message.ReadFloat ();
			cl.h2_Effects[index].Missile.avelocity[1] = net_message.ReadFloat ();
			cl.h2_Effects[index].Missile.avelocity[2] = net_message.ReadFloat ();

			if ((cl.h2_Effects[index].Missile.entity_index = CLH2_NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.h2_Effects[index].Missile.entity_index];
				VectorCopy(cl.h2_Effects[index].Missile.origin, ent->state.origin);
				VectorCopy(cl.h2_Effects[index].Missile.angle, ent->state.angles);
				if (cl.h2_Effects[index].type == HWCE_BONESHRAPNEL)
					ent->model = R_RegisterModel("models/boneshrd.mdl");
				else if (cl.h2_Effects[index].type == HWCE_HWBONEBALL)
				{
					ent->model = R_RegisterModel("models/bonelump.mdl");
					S_StartSound(cl.h2_Effects[index].Missile.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_bonefpow, 1, 1);
				}
			}
			else
				ImmediateFree = true;

			break;
		case HWCE_BONESHARD:
		case HWCE_HWRAVENSTAFF:
		case HWCE_HWRAVENPOWER:
			cl.h2_Effects[index].Missile.origin[0] = net_message.ReadCoord ();
			cl.h2_Effects[index].Missile.origin[1] = net_message.ReadCoord ();
			cl.h2_Effects[index].Missile.origin[2] = net_message.ReadCoord ();

			cl.h2_Effects[index].Missile.velocity[0] = net_message.ReadFloat ();
			cl.h2_Effects[index].Missile.velocity[1] = net_message.ReadFloat ();
			cl.h2_Effects[index].Missile.velocity[2] = net_message.ReadFloat ();
			VecToAnglesBuggy(cl.h2_Effects[index].Missile.velocity, cl.h2_Effects[index].Missile.angle);

			if ((cl.h2_Effects[index].Missile.entity_index = CLH2_NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.h2_Effects[index].Missile.entity_index];
				VectorCopy(cl.h2_Effects[index].Missile.origin, ent->state.origin);
				VectorCopy(cl.h2_Effects[index].Missile.angle, ent->state.angles);
				if (cl.h2_Effects[index].type == HWCE_HWRAVENSTAFF)
				{
					cl.h2_Effects[index].Missile.avelocity[2] = 1000;
					ent->model = R_RegisterModel("models/vindsht1.mdl");
				}
				else if (cl.h2_Effects[index].type == HWCE_BONESHARD)
				{
					cl.h2_Effects[index].Missile.avelocity[0] = (rand() % 1554) - 777;
					ent->model = R_RegisterModel("models/boneshot.mdl");
					S_StartSound(cl.h2_Effects[index].Missile.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_bone, 1, 1);
				}
				else if (cl.h2_Effects[index].type == HWCE_HWRAVENPOWER)
				{
					ent->model = R_RegisterModel("models/ravproj.mdl");
					S_StartSound(cl.h2_Effects[index].Missile.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_ravengo, 1, 1);
				}
			}
			else
				ImmediateFree = true;
			break;
		case HWCE_DEATHBUBBLES:
			cl.h2_Effects[index].Bubble.owner = net_message.ReadShort();
			cl.h2_Effects[index].Bubble.offset[0] = net_message.ReadByte();
			cl.h2_Effects[index].Bubble.offset[1] = net_message.ReadByte();
			cl.h2_Effects[index].Bubble.offset[2] = net_message.ReadByte();
			cl.h2_Effects[index].Bubble.count = net_message.ReadByte();//num of bubbles
			cl.h2_Effects[index].Bubble.time_amount = 0;
			break;
		case HWCE_HWXBOWSHOOT:
			origin[0] = net_message.ReadCoord ();
			origin[1] = net_message.ReadCoord ();
			origin[2] = net_message.ReadCoord ();

			cl.h2_Effects[index].Xbow.angle[0] = net_message.ReadAngle ();
			cl.h2_Effects[index].Xbow.angle[1] = net_message.ReadAngle ();
			cl.h2_Effects[index].Xbow.angle[2] = 0;//net_message.ReadFloat ();

			cl.h2_Effects[index].Xbow.bolts = net_message.ReadByte();
			cl.h2_Effects[index].Xbow.randseed = net_message.ReadByte();

			cl.h2_Effects[index].Xbow.turnedbolts = net_message.ReadByte();

			cl.h2_Effects[index].Xbow.activebolts= net_message.ReadByte();

			setseed(cl.h2_Effects[index].Xbow.randseed);

			AngleVectors (cl.h2_Effects[index].Xbow.angle, forward, right, up);

			VectorNormalize(forward);
			VectorCopy(forward, cl.h2_Effects[index].Xbow.velocity);
//			VectorScale(forward, 1000, cl.h2_Effects[index].Xbow.velocity);

			if (cl.h2_Effects[index].Xbow.bolts == 3)
			{
				S_StartSound(origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_xbowshoot, 1, 1);
			}
			else if (cl.h2_Effects[index].Xbow.bolts == 5)
			{
				S_StartSound(origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_xbowfshoot, 1, 1);
			}

			for (i=0;i<cl.h2_Effects[index].Xbow.bolts;i++)
			{
				cl.h2_Effects[index].Xbow.gonetime[i] = 1 + seedrand()*2;
				cl.h2_Effects[index].Xbow.state[i] = 0;

				if ((1<<i)&	cl.h2_Effects[index].Xbow.turnedbolts)
				{
					cl.h2_Effects[index].Xbow.origin[i][0]=net_message.ReadCoord();
					cl.h2_Effects[index].Xbow.origin[i][1]=net_message.ReadCoord();
					cl.h2_Effects[index].Xbow.origin[i][2]=net_message.ReadCoord();
					vtemp[0]=net_message.ReadAngle();
					vtemp[1]=net_message.ReadAngle();
					vtemp[2]=0;
					AngleVectors (vtemp, forward2, right2, up2);
					VectorScale(forward2, 800 + seedrand()*500, cl.h2_Effects[index].Xbow.vel[i]);
				}
				else
				{
					VectorScale(forward, 800 + seedrand()*500, cl.h2_Effects[index].Xbow.vel[i]);

					VectorScale(right,i*100-(cl.h2_Effects[index].Xbow.bolts-1)*50,vtemp);

					//this should only be done for deathmatch:
					VectorScale(vtemp,0.333,vtemp);

					VectorAdd(cl.h2_Effects[index].Xbow.vel[i],vtemp,cl.h2_Effects[index].Xbow.vel[i]);

					//start me off a bit out
					VectorScale(vtemp,0.05,cl.h2_Effects[index].Xbow.origin[i]);
					VectorAdd(origin,cl.h2_Effects[index].Xbow.origin[i],cl.h2_Effects[index].Xbow.origin[i]);
				}

				if ((cl.h2_Effects[index].Xbow.ent[i] = CLH2_NewEffectEntity()) != -1)
				{
					ent = &EffectEntities[cl.h2_Effects[index].Xbow.ent[i]];
					VectorCopy(cl.h2_Effects[index].Xbow.origin[i], ent->state.origin);
					VecToAnglesBuggy(cl.h2_Effects[index].Xbow.vel[i],ent->state.angles);
					if (cl.h2_Effects[index].Xbow.bolts == 5)
						ent->model = R_RegisterModel("models/flaming.mdl");
					else
						ent->model = R_RegisterModel("models/arrow.mdl");
				}
			}

			break;
		case HWCE_HWSHEEPINATOR:
			origin[0] = net_message.ReadCoord ();
			origin[1] = net_message.ReadCoord ();
			origin[2] = net_message.ReadCoord ();

			cl.h2_Effects[index].Xbow.angle[0] = net_message.ReadAngle ();
			cl.h2_Effects[index].Xbow.angle[1] = net_message.ReadAngle ();
			cl.h2_Effects[index].Xbow.angle[2] = 0;//net_message.ReadFloat ();

			cl.h2_Effects[index].Xbow.turnedbolts = net_message.ReadByte();

			cl.h2_Effects[index].Xbow.activebolts= net_message.ReadByte();

			cl.h2_Effects[index].Xbow.bolts = 5;
			cl.h2_Effects[index].Xbow.randseed = 0;

			AngleVectors (cl.h2_Effects[index].Xbow.angle, forward, right, up);

			VectorNormalize(forward);
			VectorCopy(forward, cl.h2_Effects[index].Xbow.velocity);

//			S_StartSound(origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_xbowshoot, 1, 1);

			for (i=0;i<cl.h2_Effects[index].Xbow.bolts;i++)
			{
				cl.h2_Effects[index].Xbow.gonetime[i] = 0;
				cl.h2_Effects[index].Xbow.state[i] = 0;


				if ((1<<i)&	cl.h2_Effects[index].Xbow.turnedbolts)
				{
					cl.h2_Effects[index].Xbow.origin[i][0]=net_message.ReadCoord();
					cl.h2_Effects[index].Xbow.origin[i][1]=net_message.ReadCoord();
					cl.h2_Effects[index].Xbow.origin[i][2]=net_message.ReadCoord();
					vtemp[0]=net_message.ReadAngle();
					vtemp[1]=net_message.ReadAngle();
					vtemp[2]=0;
					AngleVectors (vtemp, forward2, right2, up2);
					VectorScale(forward2, 700, cl.h2_Effects[index].Xbow.vel[i]);
				}
				else
				{
					VectorCopy(origin,cl.h2_Effects[index].Xbow.origin[i]);
					VectorScale(forward, 700, cl.h2_Effects[index].Xbow.vel[i]);
					VectorScale(right,i*75-150,vtemp);
					VectorAdd(cl.h2_Effects[index].Xbow.vel[i],vtemp,cl.h2_Effects[index].Xbow.vel[i]);
				}

				if ((cl.h2_Effects[index].Xbow.ent[i] = CLH2_NewEffectEntity()) != -1)
				{
					ent = &EffectEntities[cl.h2_Effects[index].Xbow.ent[i]];
					VectorCopy(cl.h2_Effects[index].Xbow.origin[i], ent->state.origin);
					VecToAnglesBuggy(cl.h2_Effects[index].Xbow.vel[i],ent->state.angles);
					ent->model = R_RegisterModel("models/polymrph.spr");
				}
			}

			break;
		case HWCE_HWDRILLA:
			cl.h2_Effects[index].Missile.origin[0] = net_message.ReadCoord ();
			cl.h2_Effects[index].Missile.origin[1] = net_message.ReadCoord ();
			cl.h2_Effects[index].Missile.origin[2] = net_message.ReadCoord ();

			cl.h2_Effects[index].Missile.angle[0] = net_message.ReadAngle ();
			cl.h2_Effects[index].Missile.angle[1] = net_message.ReadAngle ();
			cl.h2_Effects[index].Missile.angle[2] = 0;

			cl.h2_Effects[index].Missile.speed = net_message.ReadShort();

			S_StartSound(cl.h2_Effects[index].Missile.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_drillashoot, 1, 1);

			AngleVectors(cl.h2_Effects[index].Missile.angle,cl.h2_Effects[index].Missile.velocity,right,up);

			VectorScale(cl.h2_Effects[index].Missile.velocity,cl.h2_Effects[index].Missile.speed,cl.h2_Effects[index].Missile.velocity);

			if ((cl.h2_Effects[index].Missile.entity_index = CLH2_NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.h2_Effects[index].Missile.entity_index];
				VectorCopy(cl.h2_Effects[index].Missile.origin, ent->state.origin);
				VectorCopy(cl.h2_Effects[index].Missile.angle, ent->state.angles);
				ent->model = R_RegisterModel("models/scrbstp1.mdl");
			}
			else
				ImmediateFree = true;
			break;
		case HWCE_SCARABCHAIN:
			cl.h2_Effects[index].Chain.origin[0] = net_message.ReadCoord ();
			cl.h2_Effects[index].Chain.origin[1] = net_message.ReadCoord ();
			cl.h2_Effects[index].Chain.origin[2] = net_message.ReadCoord ();

			cl.h2_Effects[index].Chain.owner = net_message.ReadShort ();
			cl.h2_Effects[index].Chain.tag = net_message.ReadByte();
			cl.h2_Effects[index].Chain.material = cl.h2_Effects[index].Chain.owner>>12;
			cl.h2_Effects[index].Chain.owner &= 0x0fff;
			cl.h2_Effects[index].Chain.height = 16;//net_message.ReadByte ();

			cl.h2_Effects[index].Chain.sound_time = cl.serverTimeFloat;

			cl.h2_Effects[index].Chain.state = 0;//state 0: move slowly toward owner

			S_StartSound(cl.h2_Effects[index].Chain.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_scarabwhoosh, 1, 1);

			if ((cl.h2_Effects[index].Chain.ent1 = CLH2_NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.h2_Effects[index].Chain.ent1];
				VectorCopy(cl.h2_Effects[index].Chain.origin, ent->state.origin);
				ent->model = R_RegisterModel("models/scrbpbdy.mdl");
			}
			else
				ImmediateFree = true;
			break;
		case HWCE_TRIPMINE:
			cl.h2_Effects[index].Chain.origin[0] = net_message.ReadCoord ();
			cl.h2_Effects[index].Chain.origin[1] = net_message.ReadCoord ();
			cl.h2_Effects[index].Chain.origin[2] = net_message.ReadCoord ();

			cl.h2_Effects[index].Chain.velocity[0] = net_message.ReadFloat ();
			cl.h2_Effects[index].Chain.velocity[1] = net_message.ReadFloat ();
			cl.h2_Effects[index].Chain.velocity[2] = net_message.ReadFloat ();

			if ((cl.h2_Effects[index].Chain.ent1 = CLH2_NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.h2_Effects[index].Chain.ent1];
				VectorCopy(cl.h2_Effects[index].Chain.origin, ent->state.origin);
				ent->model = R_RegisterModel("models/twspike.mdl");
			}
			else
				ImmediateFree = true;
			break;
		case HWCE_TRIPMINESTILL:
//			Con_DPrintf("Allocating chain effect...\n");
			cl.h2_Effects[index].Chain.origin[0] = net_message.ReadCoord ();
			cl.h2_Effects[index].Chain.origin[1] = net_message.ReadCoord ();
			cl.h2_Effects[index].Chain.origin[2] = net_message.ReadCoord ();

			cl.h2_Effects[index].Chain.velocity[0] = net_message.ReadFloat ();
			cl.h2_Effects[index].Chain.velocity[1] = net_message.ReadFloat ();
			cl.h2_Effects[index].Chain.velocity[2] = net_message.ReadFloat ();

			if ((cl.h2_Effects[index].Chain.ent1 = CLH2_NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.h2_Effects[index].Chain.ent1];
				VectorCopy(cl.h2_Effects[index].Chain.velocity, ent->state.origin);
				ent->model = R_RegisterModel("models/twspike.mdl");
			}
			else
			{
//				Con_DPrintf("ERROR: Couldn't allocate chain effect!\n");
				ImmediateFree = true;
			}
			break;
		case HWCE_HWMISSILESTAR:
		case HWCE_HWEIDOLONSTAR:
			cl.h2_Effects[index].Star.origin[0] = net_message.ReadCoord ();
			cl.h2_Effects[index].Star.origin[1] = net_message.ReadCoord ();
			cl.h2_Effects[index].Star.origin[2] = net_message.ReadCoord ();

			cl.h2_Effects[index].Star.velocity[0] = net_message.ReadFloat ();
			cl.h2_Effects[index].Star.velocity[1] = net_message.ReadFloat ();
			cl.h2_Effects[index].Star.velocity[2] = net_message.ReadFloat ();
			VecToAnglesBuggy(cl.h2_Effects[index].Star.velocity, cl.h2_Effects[index].Star.angle);
			cl.h2_Effects[index].Missile.avelocity[2] = 300 + rand()%300;

			if ((cl.h2_Effects[index].Star.entity_index = CLH2_NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.h2_Effects[index].Star.entity_index];
				VectorCopy(cl.h2_Effects[index].Star.origin, ent->state.origin);
				VectorCopy(cl.h2_Effects[index].Star.angle, ent->state.angles);
				ent->model = R_RegisterModel("models/ball.mdl");
			}
			else
				ImmediateFree = true;

			cl.h2_Effects[index].Star.scaleDir = 1;
			cl.h2_Effects[index].Star.scale = 0.3;
			
			if ((cl.h2_Effects[index].Star.ent1 = CLH2_NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.h2_Effects[index].Star.ent1];
				VectorCopy(cl.h2_Effects[index].Star.origin, ent->state.origin);
				ent->state.drawflags |= H2MLS_ABSLIGHT;
				ent->state.abslight = 0.5;
				ent->state.angles[2] = 90;
				if(cl.h2_Effects[index].type == HWCE_HWMISSILESTAR)
				{
					ent->model = R_RegisterModel("models/star.mdl");
					ent->state.scale = 0.3;
					S_StartSound(ent->state.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_mmfire, 1, 1);

				}
				else
				{
					ent->model = R_RegisterModel("models/glowball.mdl");
					S_StartSound(ent->state.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_eidolon, 1, 1);
				}
			}
			if(cl.h2_Effects[index].type == HWCE_HWMISSILESTAR)
			{
				if ((cl.h2_Effects[index].Star.ent2 = CLH2_NewEffectEntity()) != -1)
				{
					ent = &EffectEntities[cl.h2_Effects[index].Star.ent2];
					VectorCopy(cl.h2_Effects[index].Star.origin, ent->state.origin);
					ent->model = R_RegisterModel("models/star.mdl");
					ent->state.drawflags |= H2MLS_ABSLIGHT;
					ent->state.abslight = 0.5;
					ent->state.scale = 0.3;
				}
			}
			break;
		default:
			Sys_Error ("CL_ParseEffect: bad type");
	}

	if (ImmediateFree)
	{
		cl.h2_Effects[index].type = HWCE_NONE;
	}
}

// these are in cl_tent.c
void CreateRavenDeath(vec3_t pos);
void CreateExplosionWithSound(vec3_t pos);

void CL_EndEffect(void)
{
	int index;
	effect_entity_t* ent;

	index = net_message.ReadByte();

	switch(cl.h2_Effects[index].type )
	{
	case HWCE_HWRAVENPOWER:
		if(cl.h2_Effects[index].Missile.entity_index > -1)
		{
			ent = &EffectEntities[cl.h2_Effects[index].Missile.entity_index];
			CreateRavenDeath(ent->state.origin);
		}
		break;
	case HWCE_HWRAVENSTAFF:
		if(cl.h2_Effects[index].Missile.entity_index > -1)
		{
			ent = &EffectEntities[cl.h2_Effects[index].Missile.entity_index];
			CreateExplosionWithSound(ent->state.origin);
		}
		break;
	}
	CLH2_FreeEffect(index);
}

void XbowImpactPuff(vec3_t origin, int material)//hopefully can use this with xbow & chain both
{
	int		part_color;

	switch(material)
	{
	case H2XBOW_IMPACT_REDFLESH:
		part_color = 256 + 8 * 16 + rand()%9;				//Blood red
		break;
	case H2XBOW_IMPACT_STONE:
		part_color = 256 + 20 + rand()%8;			// Gray
		break;
	case H2XBOW_IMPACT_METAL:
		part_color = 256 + (8 * 15);			// Sparks
		break;
	case H2XBOW_IMPACT_WOOD:
		part_color = 256 + (5 * 16) + rand()%8;			// Wood chunks
		break;
	case H2XBOW_IMPACT_ICE:
		part_color = 406+rand()%8;				// Ice particles
		break;
	case H2XBOW_IMPACT_GREENFLESH:
		part_color = 256 + 183 + rand()%8;		// Spider's have green blood
		break;
	default:
		part_color = 256 + (3 * 16) + 4;		// Dust Brown
		break;
	}

	CLH2_RunParticleEffect4 (origin, 24, part_color, pt_h2fastgrav, 20);
}

void CL_ReviseEffect(void)	// be sure to read, in the switch statement, everything
							// in this message, even if the effect is not the right kind or invalid,
							// or else client is sure to crash.	
{
	int index,type,revisionCode;
	int curEnt,material,takedamage;
	effect_entity_t* ent;
	vec3_t	forward,right,up,pos;
	float	dist,speed;
	h2entity_state_t	*es;


	index = net_message.ReadByte ();
	type = net_message.ReadByte ();

	if (cl.h2_Effects[index].type==type)
		switch(type)
		{
		case HWCE_SCARABCHAIN://attach to new guy or retract if new guy is world
			curEnt = net_message.ReadShort();
			if (cl.h2_Effects[index].type==type)
			{
				cl.h2_Effects[index].Chain.material = curEnt>>12;
				curEnt &= 0x0fff;

				if (curEnt)
				{
					cl.h2_Effects[index].Chain.state = 1;
					cl.h2_Effects[index].Chain.owner = curEnt;
					es = FindState(cl.h2_Effects[index].Chain.owner);
					if (es)
					{
						ent = &EffectEntities[cl.h2_Effects[index].Chain.ent1];
						XbowImpactPuff(ent->state.origin, cl.h2_Effects[index].Chain.material);
					}
				}
				else
					cl.h2_Effects[index].Chain.state = 2;
			}
			break;
		case HWCE_HWXBOWSHOOT:
			revisionCode = net_message.ReadByte();
			//this is one packed byte!
			//highest bit: for impact revision, indicates whether damage is done
			//				for redirect revision, indicates whether new origin was sent
			//next 3 high bits: for all revisions, indicates which bolt is to be revised
			//highest 3 of the low 4 bits: for impact revision, indicates the material that was hit
			//lowest bit: indicates whether revision is of impact or redirect variety


			curEnt = (revisionCode>>4)&7;
			if (revisionCode & 1)//impact effect: 
			{
				cl.h2_Effects[index].Xbow.activebolts &= ~(1<<curEnt);
				dist = net_message.ReadCoord();
				if (cl.h2_Effects[index].Xbow.ent[curEnt]!= -1)
				{
					ent = &EffectEntities[cl.h2_Effects[index].Xbow.ent[curEnt]];

					//make sure bolt is in correct position
					VectorCopy(cl.h2_Effects[index].Xbow.vel[curEnt],forward);
					VectorNormalize(forward);
					VectorScale(forward,dist,forward);
					VectorAdd(cl.h2_Effects[index].Xbow.origin[curEnt],forward,ent->state.origin);

					material = (revisionCode & 14);
					takedamage = (revisionCode & 128);

					if (takedamage)
					{
						cl.h2_Effects[index].Xbow.gonetime[curEnt] = cl.serverTimeFloat;
					}
					else
					{
						cl.h2_Effects[index].Xbow.gonetime[curEnt] += cl.serverTimeFloat;
					}
					
					VectorCopy(cl.h2_Effects[index].Xbow.vel[curEnt],forward);
					VectorNormalize(forward);
					VectorScale(forward,8,forward);

					// do a particle effect here, with the color depending on chType
					XbowImpactPuff(ent->state.origin,material);

					// impact sound:
					switch (material)
					{
					case H2XBOW_IMPACT_GREENFLESH:
					case H2XBOW_IMPACT_REDFLESH:
					case H2XBOW_IMPACT_MUMMY:
						S_StartSound(ent->state.origin, CLH2_TempSoundChannel(), 0, clh2_fxsfx_arr2flsh, 1, 1);
						break;
					case H2XBOW_IMPACT_WOOD:
						S_StartSound(ent->state.origin, CLH2_TempSoundChannel(), 0, clh2_fxsfx_arr2wood, 1, 1);
						break;
					default:
						S_StartSound(ent->state.origin, CLH2_TempSoundChannel(), 0, clh2_fxsfx_met2stn, 1, 1);
						break;
					}

					CLTENT_XbowImpact(ent->state.origin, forward, material, takedamage, cl.h2_Effects[index].Xbow.bolts);
				}
			}
			else
			{
				if (cl.h2_Effects[index].Xbow.ent[curEnt]!=-1)
				{
					ent = &EffectEntities[cl.h2_Effects[index].Xbow.ent[curEnt]];
					ent->state.angles[0] = net_message.ReadAngle();
					if (ent->state.angles[0] < 0)
						ent->state.angles[0] += 360;
					ent->state.angles[0]*=-1;
					ent->state.angles[1] = net_message.ReadAngle();
					if (ent->state.angles[1] < 0)
						ent->state.angles[1] += 360;
					ent->state.angles[2] = 0;

					if (revisionCode &128)//new origin
					{
						cl.h2_Effects[index].Xbow.origin[curEnt][0]=net_message.ReadCoord();
						cl.h2_Effects[index].Xbow.origin[curEnt][1]=net_message.ReadCoord();
						cl.h2_Effects[index].Xbow.origin[curEnt][2]=net_message.ReadCoord();
					}

					AngleVectors(ent->state.angles,forward,right,up);
					speed = VectorLength(cl.h2_Effects[index].Xbow.vel[curEnt]);
					VectorScale(forward,speed,cl.h2_Effects[index].Xbow.vel[curEnt]);
					VectorCopy(cl.h2_Effects[index].Xbow.origin[curEnt],ent->state.origin);
				}
				else
				{
					pos[0] = net_message.ReadAngle();
					if (pos[0] < 0)
						pos[0] += 360;
					pos[0]*=-1;
					pos[1] = net_message.ReadAngle();
					if (pos[1] < 0)
						pos[1] += 360;
					pos[2] = 0;

					if (revisionCode &128)//new origin
					{
						cl.h2_Effects[index].Xbow.origin[curEnt][0]=net_message.ReadCoord();
						cl.h2_Effects[index].Xbow.origin[curEnt][1]=net_message.ReadCoord();
						cl.h2_Effects[index].Xbow.origin[curEnt][2]=net_message.ReadCoord();
					}

					AngleVectors(pos,forward,right,up);
					speed = VectorLength(cl.h2_Effects[index].Xbow.vel[curEnt]);
					VectorScale(forward,speed,cl.h2_Effects[index].Xbow.vel[curEnt]);
				}
			}
			break;

		case HWCE_HWSHEEPINATOR:
			revisionCode = net_message.ReadByte();
			curEnt = (revisionCode>>4)&7;
			if (revisionCode & 1)//impact
			{
				dist = net_message.ReadCoord();
				cl.h2_Effects[index].Xbow.activebolts &= ~(1<<curEnt);
				if (cl.h2_Effects[index].Xbow.ent[curEnt] != -1)
				{
					ent = &EffectEntities[cl.h2_Effects[index].Xbow.ent[curEnt]];

					//make sure bolt is in correct position
					VectorCopy(cl.h2_Effects[index].Xbow.vel[curEnt],forward);
					VectorNormalize(forward);
					VectorScale(forward,dist,forward);
					VectorAdd(cl.h2_Effects[index].Xbow.origin[curEnt],forward,ent->state.origin);
					CLH2_ColouredParticleExplosion(ent->state.origin,(rand()%16)+144/*(144,159)*/,20,30);
				}
			}
			else//direction change
			{
				if (cl.h2_Effects[index].Xbow.ent[curEnt] != -1)
				{
					ent = &EffectEntities[cl.h2_Effects[index].Xbow.ent[curEnt]];
					ent->state.angles[0] = net_message.ReadAngle();
					if (ent->state.angles[0] < 0)
						ent->state.angles[0] += 360;
					ent->state.angles[0]*=-1;
					ent->state.angles[1] = net_message.ReadAngle();
					if (ent->state.angles[1] < 0)
						ent->state.angles[1] += 360;
					ent->state.angles[2] = 0;

					if (revisionCode &128)//new origin
					{
						cl.h2_Effects[index].Xbow.origin[curEnt][0]=net_message.ReadCoord();
						cl.h2_Effects[index].Xbow.origin[curEnt][1]=net_message.ReadCoord();
						cl.h2_Effects[index].Xbow.origin[curEnt][2]=net_message.ReadCoord();
					}

					AngleVectors(ent->state.angles,forward,right,up);
					speed = VectorLength(cl.h2_Effects[index].Xbow.vel[curEnt]);
					VectorScale(forward,speed,cl.h2_Effects[index].Xbow.vel[curEnt]);
					VectorCopy(cl.h2_Effects[index].Xbow.origin[curEnt],ent->state.origin);
				}
				else
				{
					pos[0] = net_message.ReadAngle();
					if (pos[0] < 0)
						pos[0] += 360;
					pos[0]*=-1;
					pos[1] = net_message.ReadAngle();
					if (pos[1] < 0)
						pos[1] += 360;
					pos[2] = 0;

					if (revisionCode &128)//new origin
					{
						cl.h2_Effects[index].Xbow.origin[curEnt][0]=net_message.ReadCoord();
						cl.h2_Effects[index].Xbow.origin[curEnt][1]=net_message.ReadCoord();
						cl.h2_Effects[index].Xbow.origin[curEnt][2]=net_message.ReadCoord();
					}

					AngleVectors(pos,forward,right,up);
					speed = VectorLength(cl.h2_Effects[index].Xbow.vel[curEnt]);
					VectorScale(forward,speed,cl.h2_Effects[index].Xbow.vel[curEnt]);
				}
			}
			break;


		case HWCE_HWDRILLA:
			revisionCode = net_message.ReadByte();
			if (revisionCode == 0)//impact
			{
				pos[0] = net_message.ReadCoord();
				pos[1] = net_message.ReadCoord();
				pos[2] = net_message.ReadCoord();
				material = net_message.ReadByte();

				//throw lil bits of victim at entry
				XbowImpactPuff(pos,material);

				if ((material == H2XBOW_IMPACT_GREENFLESH) || (material == H2XBOW_IMPACT_GREENFLESH))
				{//meaty sound and some chunks too
					S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_fxsfx_drillameat, 1, 1);
					
					//todo: the chunks
				}

				//lil bits at exit
				VectorCopy(cl.h2_Effects[index].Missile.velocity,forward);
				VectorNormalize(forward);
				VectorScale(forward,36,forward);
				VectorAdd(forward,pos,pos);
				XbowImpactPuff(pos,material);
			}
			else//turn
			{
				if (cl.h2_Effects[index].Missile.entity_index!=-1)
				{
					ent = &EffectEntities[cl.h2_Effects[index].Missile.entity_index];
					ent->state.angles[0] = net_message.ReadAngle();
					if (ent->state.angles[0] < 0)
						ent->state.angles[0] += 360;
					ent->state.angles[0]*=-1;
					ent->state.angles[1] = net_message.ReadAngle();
					if (ent->state.angles[1] < 0)
						ent->state.angles[1] += 360;
					ent->state.angles[2] = 0;

					cl.h2_Effects[index].Missile.origin[0]=net_message.ReadCoord();
					cl.h2_Effects[index].Missile.origin[1]=net_message.ReadCoord();
					cl.h2_Effects[index].Missile.origin[2]=net_message.ReadCoord();

					AngleVectors(ent->state.angles,forward,right,up);
					speed = VectorLength(cl.h2_Effects[index].Missile.velocity);
					VectorScale(forward,speed,cl.h2_Effects[index].Missile.velocity);
					VectorCopy(cl.h2_Effects[index].Missile.origin,ent->state.origin);
				}
				else
				{
					pos[0] = net_message.ReadAngle();
					if (pos[0] < 0)
						pos[0] += 360;
					pos[0]*=-1;
					pos[1] = net_message.ReadAngle();
					if (pos[1] < 0)
						pos[1] += 360;
					pos[2] = 0;

					cl.h2_Effects[index].Missile.origin[0]=net_message.ReadCoord();
					cl.h2_Effects[index].Missile.origin[1]=net_message.ReadCoord();
					cl.h2_Effects[index].Missile.origin[2]=net_message.ReadCoord();

					AngleVectors(pos,forward,right,up);
					speed = VectorLength(cl.h2_Effects[index].Missile.velocity);
					VectorScale(forward,speed,cl.h2_Effects[index].Missile.velocity);
				}
			}
			break;
		}
	else
	{
//		Con_DPrintf("Received Unrecognized Effect Update!\n");
		switch(type)
		{
		case HWCE_SCARABCHAIN://attach to new guy or retract if new guy is world
			curEnt = net_message.ReadShort();
			break;
		case HWCE_HWXBOWSHOOT:
			revisionCode = net_message.ReadByte();

			curEnt = (revisionCode>>4)&7;
			if (revisionCode & 1)//impact effect: 
			{
				net_message.ReadCoord();
			}
			else
			{
				net_message.ReadAngle();
				net_message.ReadAngle();
				if (revisionCode &128)//new origin
				{
					net_message.ReadCoord();
					net_message.ReadCoord();
					net_message.ReadCoord();

					// create a clc message to retrieve effect information
//					cls.netchan.message.WriteByte(clc_get_effect);
//					cls.netchan.message.WriteByte(index);
				}
			}
			break;
		case HWCE_HWSHEEPINATOR:
			revisionCode = net_message.ReadByte();
			curEnt = (revisionCode>>4)&7;
			if (revisionCode & 1)//impact
			{
				net_message.ReadCoord();
			}
			else//direction change
			{
				net_message.ReadAngle();
				net_message.ReadAngle();
				if (revisionCode &128)//new origin
				{
					net_message.ReadCoord();
					net_message.ReadCoord();
					net_message.ReadCoord();

					// create a clc message to retrieve effect information
//					cls.netchan.message.WriteByte(clc_get_effect);
//					cls.netchan.message.WriteByte(index);
				}
			}
			break;
		case HWCE_HWDRILLA:
			revisionCode = net_message.ReadByte();
			if (revisionCode == 0)//impact
			{
				net_message.ReadCoord();
				net_message.ReadCoord();
				net_message.ReadCoord();
				net_message.ReadByte();
			}
			else//turn
			{
				net_message.ReadAngle();
				net_message.ReadAngle();

				net_message.ReadCoord();
				net_message.ReadCoord();
				net_message.ReadCoord();

				// create a clc message to retrieve effect information
//				cls.netchan.message.WriteByte(clc_get_effect);
//				cls.netchan.message.WriteByte(index);

			}
			break;
		}
	}
}

void UpdateMissilePath(vec3_t oldorg, vec3_t neworg, vec3_t newvel, float time)
{
	vec3_t endpos;	//the position it should be at currently
	float delta;

	delta = cl.serverTimeFloat - time;
	
	VectorMA(neworg, delta, newvel, endpos); 
	VectorCopy(neworg, oldorg);	//set orig, maybe vel too
}


void CL_TurnEffect(void)
{
	int index;
	effect_entity_t* ent;
	vec3_t pos, vel;
	float time;

	index = net_message.ReadByte ();
	time = net_message.ReadFloat ();
	pos[0] = net_message.ReadCoord();
	pos[1] = net_message.ReadCoord();
	pos[2] = net_message.ReadCoord();
	vel[0] = net_message.ReadCoord();
	vel[1] = net_message.ReadCoord();
	vel[2] = net_message.ReadCoord();
	switch(cl.h2_Effects[index].type)
	{
	case HWCE_HWRAVENSTAFF:
	case HWCE_HWRAVENPOWER:
	case HWCE_BONESHARD:
	case HWCE_BONESHRAPNEL:
	case HWCE_HWBONEBALL:
		if(cl.h2_Effects[index].Missile.entity_index > -1)
		{
			ent = &EffectEntities[cl.h2_Effects[index].Missile.entity_index];
			UpdateMissilePath(ent->state.origin, pos, vel, time);
			VectorCopy(vel, cl.h2_Effects[index].Missile.velocity);
			VecToAnglesBuggy(cl.h2_Effects[index].Missile.velocity, cl.h2_Effects[index].Missile.angle);
		}
		break;
	case HWCE_HWMISSILESTAR:
	case HWCE_HWEIDOLONSTAR:
		if(cl.h2_Effects[index].Star.entity_index > -1)
		{
			ent = &EffectEntities[cl.h2_Effects[index].Star.entity_index];
			UpdateMissilePath(ent->state.origin, pos, vel, time);
			VectorCopy(vel, cl.h2_Effects[index].Star.velocity);
		}
		break;
	case 0:
		// create a clc message to retrieve effect information
//		cls.netchan.message.WriteByte(clc_get_effect);
//		cls.netchan.message.WriteByte(index);
//		Con_Printf("CL_TurnEffect: null effect %d\n", index);
		break;
	default:
		Con_Printf ("CL_TurnEffect: bad type %d\n", cl.h2_Effects[index].type);
		break;
	}

}

void CL_LinkEntity(effect_entity_t* ent)
{
	//debug: if visedicts getting messed up, it should appear here.
//	if (ent->_model < 10)
//	{
//		ent->_model = 3;
//	}

	refEntity_t rent;
	Com_Memset(&rent, 0, sizeof(rent));
	rent.reType = RT_MODEL;
	VectorCopy(ent->state.origin, rent.origin);
	rent.hModel = ent->model;
	rent.frame = ent->state.frame;
	rent.skinNum = ent->state.skinnum;
	CL_SetRefEntAxis(&rent, ent->state.angles, vec3_origin, ent->state.scale, 0, ent->state.abslight, ent->state.drawflags);
	R_HandleCustomSkin(&rent, -1);
	R_AddRefEntityToScene(&rent);
}

void CL_UpdateEffects(void)
{
	int			index,cur_frame;
	vec3_t		mymin,mymax;
	float		frametime;
//	edict_t		test;
//	trace_t		trace;
	vec3_t		org,org2,old_origin;
	int			x_dir,y_dir;
	effect_entity_t	*ent, *ent2;
	float		smoketime;
	int			i;
	h2entity_state_t	*es;

	frametime = host_frametime;
	if (!frametime) return;
//	Con_Printf("Here at %f\n",cl.time);

	for(index=0;index<MAX_EFFECTS_H2;index++)
	{
		if (!cl.h2_Effects[index].type) 
			continue;

		switch(cl.h2_Effects[index].type)
		{
			case HWCE_RAIN:
				org[0] = cl.h2_Effects[index].Rain.min_org[0];
				org[1] = cl.h2_Effects[index].Rain.min_org[1];
				org[2] = cl.h2_Effects[index].Rain.max_org[2];

				org2[0] = cl.h2_Effects[index].Rain.e_size[0];
				org2[1] = cl.h2_Effects[index].Rain.e_size[1];
				org2[2] = cl.h2_Effects[index].Rain.e_size[2];

				x_dir = cl.h2_Effects[index].Rain.dir[0];
				y_dir = cl.h2_Effects[index].Rain.dir[1];
				
				cl.h2_Effects[index].Rain.next_time += frametime;
				if (cl.h2_Effects[index].Rain.next_time >= cl.h2_Effects[index].Rain.wait)
				{		
					CLH2_RainEffect(org,org2,x_dir,y_dir,cl.h2_Effects[index].Rain.color,
						cl.h2_Effects[index].Rain.count);
					cl.h2_Effects[index].Rain.next_time = 0;
				}
				break;

			case HWCE_FOUNTAIN:
				mymin[0] = (-3 * cl.h2_Effects[index].Fountain.vright[0] * cl.h2_Effects[index].Fountain.movedir[0]) +
						   (-3 * cl.h2_Effects[index].Fountain.vforward[0] * cl.h2_Effects[index].Fountain.movedir[1]) +
						   (2 * cl.h2_Effects[index].Fountain.vup[0] * cl.h2_Effects[index].Fountain.movedir[2]);
				mymin[1] = (-3 * cl.h2_Effects[index].Fountain.vright[1] * cl.h2_Effects[index].Fountain.movedir[0]) +
						   (-3 * cl.h2_Effects[index].Fountain.vforward[1] * cl.h2_Effects[index].Fountain.movedir[1]) +
						   (2 * cl.h2_Effects[index].Fountain.vup[1] * cl.h2_Effects[index].Fountain.movedir[2]);
				mymin[2] = (-3 * cl.h2_Effects[index].Fountain.vright[2] * cl.h2_Effects[index].Fountain.movedir[0]) +
						   (-3 * cl.h2_Effects[index].Fountain.vforward[2] * cl.h2_Effects[index].Fountain.movedir[1]) +
						   (2 * cl.h2_Effects[index].Fountain.vup[2] * cl.h2_Effects[index].Fountain.movedir[2]);
				mymin[0] *= 15;
				mymin[1] *= 15;
				mymin[2] *= 15;

				mymax[0] = (3 * cl.h2_Effects[index].Fountain.vright[0] * cl.h2_Effects[index].Fountain.movedir[0]) +
						   (3 * cl.h2_Effects[index].Fountain.vforward[0] * cl.h2_Effects[index].Fountain.movedir[1]) +
						   (10 * cl.h2_Effects[index].Fountain.vup[0] * cl.h2_Effects[index].Fountain.movedir[2]);
				mymax[1] = (3 * cl.h2_Effects[index].Fountain.vright[1] * cl.h2_Effects[index].Fountain.movedir[0]) +
						   (3 * cl.h2_Effects[index].Fountain.vforward[1] * cl.h2_Effects[index].Fountain.movedir[1]) +
						   (10 * cl.h2_Effects[index].Fountain.vup[1] * cl.h2_Effects[index].Fountain.movedir[2]);
				mymax[2] = (3 * cl.h2_Effects[index].Fountain.vright[2] * cl.h2_Effects[index].Fountain.movedir[0]) +
						   (3 * cl.h2_Effects[index].Fountain.vforward[2] * cl.h2_Effects[index].Fountain.movedir[1]) +
						   (10 * cl.h2_Effects[index].Fountain.vup[2] * cl.h2_Effects[index].Fountain.movedir[2]);
				mymax[0] *= 15;
				mymax[1] *= 15;
				mymax[2] *= 15;

				CLH2_RunParticleEffect2 (cl.h2_Effects[index].Fountain.pos,mymin,mymax,
					                  cl.h2_Effects[index].Fountain.color,pt_h2fastgrav,cl.h2_Effects[index].Fountain.cnt);

/*				Com_Memset(&test,0,sizeof(test));
				trace = SV_Move (cl.h2_Effects[index].Fountain.pos, mymin, mymax, mymin, false, &test);
				Con_Printf("Fraction is %f\n",trace.fraction);*/
				break;

			case HWCE_QUAKE:
				CLH2_RunQuakeEffect (cl.h2_Effects[index].Quake.origin,cl.h2_Effects[index].Quake.radius);
				break;

			case HWCE_RIPPLE:
				cl.h2_Effects[index].Smoke.time_amount += frametime;
				ent = &EffectEntities[cl.h2_Effects[index].Smoke.entity_index];

				smoketime = cl.h2_Effects[index].Smoke.framelength;
				if (!smoketime)
					smoketime = HX_FRAME_TIME*2;

				while(cl.h2_Effects[index].Smoke.time_amount >= smoketime&&ent->state.scale<250)
				{
					ent->state.frame++;
					ent->state.angles[1]+=1;
					cl.h2_Effects[index].Smoke.time_amount -= smoketime;
				}

				if (ent->state.frame >= 10)
				{
					CLH2_FreeEffect(index);
				}
				else
					CL_LinkEntity(ent);
				break;


			case HWCE_WHITE_SMOKE:
			case HWCE_GREEN_SMOKE:
			case HWCE_GREY_SMOKE:
			case HWCE_RED_SMOKE:
			case HWCE_SLOW_WHITE_SMOKE:
			case HWCE_TELESMK1:
			case HWCE_TELESMK2:
			case HWCE_GHOST:
			case HWCE_REDCLOUD:
			case HWCE_FLAMESTREAM:
			case HWCE_ACID_MUZZFL:
			case HWCE_FLAMEWALL:
			case HWCE_FLAMEWALL2:
			case HWCE_ONFIRE:
				cl.h2_Effects[index].Smoke.time_amount += frametime;
				ent = &EffectEntities[cl.h2_Effects[index].Smoke.entity_index];

				smoketime = cl.h2_Effects[index].Smoke.framelength;
				if (!smoketime)
					smoketime = HX_FRAME_TIME;

				ent->state.origin[0] += (frametime/smoketime) * cl.h2_Effects[index].Smoke.velocity[0];
				ent->state.origin[1] += (frametime/smoketime) * cl.h2_Effects[index].Smoke.velocity[1];
				ent->state.origin[2] += (frametime/smoketime) * cl.h2_Effects[index].Smoke.velocity[2];
	
				i=0;
				while(cl.h2_Effects[index].Smoke.time_amount >= smoketime)
				{
					ent->state.frame++;
					i++;
					cl.h2_Effects[index].Smoke.time_amount -= smoketime;
				}

				if (ent->state.frame >= R_ModelNumFrames(ent->model))
				{
					CLH2_FreeEffect(index);
				}
				else
					CL_LinkEntity(ent);

				
				if(cl.h2_Effects[index].type == HWCE_TELESMK1)
				{
					ent = &EffectEntities[cl.h2_Effects[index].Smoke.entity_index2];

					ent->state.origin[0] -= (frametime/smoketime) * cl.h2_Effects[index].Smoke.velocity[0];
					ent->state.origin[1] -= (frametime/smoketime) * cl.h2_Effects[index].Smoke.velocity[1];
					ent->state.origin[2] -= (frametime/smoketime) * cl.h2_Effects[index].Smoke.velocity[2];

					ent->state.frame += i;
					if (ent->state.frame < R_ModelNumFrames(ent->model))
					{
						CL_LinkEntity(ent);
					}
				}
				break;

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
			case HWCE_ACID_HIT:
			case HWCE_ACID_SPLAT:
			case HWCE_ACID_EXPL:
			case HWCE_LBALL_EXPL:
			case HWCE_FBOOM:
			case HWCE_BOMB:
			case HWCE_FIREWALL_SMALL:
			case HWCE_FIREWALL_MEDIUM:
			case HWCE_FIREWALL_LARGE:

				cl.h2_Effects[index].Smoke.time_amount += frametime;
				ent = &EffectEntities[cl.h2_Effects[index].Smoke.entity_index];

				if (cl.h2_Effects[index].type != HWCE_BG_CIRCLE_EXP)
				{
					while(cl.h2_Effects[index].Smoke.time_amount >= HX_FRAME_TIME)
					{
						ent->state.frame++;
						cl.h2_Effects[index].Smoke.time_amount -= HX_FRAME_TIME;
					}
				}
				else
				{
					while(cl.h2_Effects[index].Smoke.time_amount >= HX_FRAME_TIME * 2)
					{
						ent->state.frame++;
						cl.h2_Effects[index].Smoke.time_amount -= HX_FRAME_TIME * 2;
					}
				}


				if (ent->state.frame >= R_ModelNumFrames(ent->model))
				{
					CLH2_FreeEffect(index);
				}
				else
					CL_LinkEntity(ent);

				break;

			// Go forward then backward through animation then remove
			case HWCE_WHITE_FLASH:
			case HWCE_BLUE_FLASH:
			case HWCE_SM_BLUE_FLASH:
			case HWCE_HWSPLITFLASH:
			case HWCE_RED_FLASH:
				cl.h2_Effects[index].Flash.time_amount += frametime;
				ent = &EffectEntities[cl.h2_Effects[index].Flash.entity_index];

				while(cl.h2_Effects[index].Flash.time_amount >= HX_FRAME_TIME)
				{
					if (!cl.h2_Effects[index].Flash.reverse)
					{
						if (ent->state.frame >= R_ModelNumFrames(ent->model) - 1)  // Ran through forward animation
						{
							cl.h2_Effects[index].Flash.reverse = 1;
							ent->state.frame--;
						}
						else
							ent->state.frame++;

					}	
					else
						ent->state.frame--;

					cl.h2_Effects[index].Flash.time_amount -= HX_FRAME_TIME;
				}

				if ((ent->state.frame <= 0) && (cl.h2_Effects[index].Flash.reverse))
				{
					CLH2_FreeEffect(index);
				}
				else
					CL_LinkEntity(ent);
				break;

			case HWCE_RIDER_DEATH:
				cl.h2_Effects[index].RD.time_amount += frametime;
				if (cl.h2_Effects[index].RD.time_amount >= 1)
				{
					cl.h2_Effects[index].RD.stage++;
					cl.h2_Effects[index].RD.time_amount -= 1;
				}

				VectorCopy(cl.h2_Effects[index].RD.origin,org);
				org[0] += sin(cl.h2_Effects[index].RD.time_amount * 2 * M_PI) * 30;
				org[1] += cos(cl.h2_Effects[index].RD.time_amount * 2 * M_PI) * 30;

				if (cl.h2_Effects[index].RD.stage <= 6)
				{
					CLH2_RiderParticles(cl.h2_Effects[index].RD.stage+1,org);
				}
				else
				{
					// To set the rider's origin point for the particles
					CLH2_RiderParticles(0,org);
					if (cl.h2_Effects[index].RD.stage == 7) 
					{
						cl.cshifts[CSHIFT_BONUS].destcolor[0] = 255;
						cl.cshifts[CSHIFT_BONUS].destcolor[1] = 255;
						cl.cshifts[CSHIFT_BONUS].destcolor[2] = 255;
						cl.cshifts[CSHIFT_BONUS].percent = 256;
					}
					else if (cl.h2_Effects[index].RD.stage > 13) 
					{
//						cl.h2_Effects[index].RD.stage = 0;
						CLH2_FreeEffect(index);
					}
				}
				break;

			case HWCE_TELEPORTERPUFFS:
				cl.h2_Effects[index].Teleporter.time_amount += frametime;
				smoketime = cl.h2_Effects[index].Teleporter.framelength;

				ent = &EffectEntities[cl.h2_Effects[index].Teleporter.entity_index[0]];
				while(cl.h2_Effects[index].Teleporter.time_amount >= HX_FRAME_TIME)
				{
					ent->state.frame++;
					cl.h2_Effects[index].Teleporter.time_amount -= HX_FRAME_TIME;
				}
				cur_frame = ent->state.frame;

				if (cur_frame >= R_ModelNumFrames(ent->model))
				{
					CLH2_FreeEffect(index);
					break;
				}

				for (i=0;i<8;++i)
				{
					ent = &EffectEntities[cl.h2_Effects[index].Teleporter.entity_index[i]];

					ent->state.origin[0] += (frametime/smoketime) * cl.h2_Effects[index].Teleporter.velocity[i][0];
					ent->state.origin[1] += (frametime/smoketime) * cl.h2_Effects[index].Teleporter.velocity[i][1];
					ent->state.origin[2] += (frametime/smoketime) * cl.h2_Effects[index].Teleporter.velocity[i][2];
					ent->state.frame = cur_frame;

					CL_LinkEntity(ent);
				}
				break;
			case HWCE_TELEPORTERBODY:
				cl.h2_Effects[index].Teleporter.time_amount += frametime;
				smoketime = cl.h2_Effects[index].Teleporter.framelength;

				ent = &EffectEntities[cl.h2_Effects[index].Teleporter.entity_index[0]];
				while(cl.h2_Effects[index].Teleporter.time_amount >= HX_FRAME_TIME)
				{
					ent->state.scale -= 15;
					cl.h2_Effects[index].Teleporter.time_amount -= HX_FRAME_TIME;
				}

				ent = &EffectEntities[cl.h2_Effects[index].Teleporter.entity_index[0]];
				ent->state.angles[1] += 45;

				if (ent->state.scale <= 10)
				{
					CLH2_FreeEffect(index);
				}
				else
				{
					CL_LinkEntity(ent);
				}
				break;

			case HWCE_HWDRILLA:
				cl.h2_Effects[index].Missile.time_amount += frametime;
				ent = &EffectEntities[cl.h2_Effects[index].Missile.entity_index];

				if ((int)(cl.serverTimeFloat) != (int)(cl.serverTimeFloat - host_frametime))
				{
					S_StartSound(ent->state.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_drillaspin, 1, 1);
				}

				ent->state.angles[0] += frametime * cl.h2_Effects[index].Missile.avelocity[0];
				ent->state.angles[1] += frametime * cl.h2_Effects[index].Missile.avelocity[1];
				ent->state.angles[2] += frametime * cl.h2_Effects[index].Missile.avelocity[2];

				VectorCopy(ent->state.origin,old_origin);

				ent->state.origin[0] += frametime * cl.h2_Effects[index].Missile.velocity[0];
				ent->state.origin[1] += frametime * cl.h2_Effects[index].Missile.velocity[1];
				ent->state.origin[2] += frametime * cl.h2_Effects[index].Missile.velocity[2];

				CLH2_TrailParticles (old_origin, ent->state.origin, rt_setstaff);

				CL_LinkEntity(ent);
				break;
			case HWCE_HWXBOWSHOOT:
				cl.h2_Effects[index].Xbow.time_amount += frametime;
				for (i=0;i<cl.h2_Effects[index].Xbow.bolts;i++)
				{
					if (cl.h2_Effects[index].Xbow.ent[i] != -1)//only update valid effect ents
					{
						if (cl.h2_Effects[index].Xbow.activebolts & (1<<i))//bolt in air, simply update position
						{
							ent = &EffectEntities[cl.h2_Effects[index].Xbow.ent[i]];

							ent->state.origin[0] += frametime * cl.h2_Effects[index].Xbow.vel[i][0];
							ent->state.origin[1] += frametime * cl.h2_Effects[index].Xbow.vel[i][1];
							ent->state.origin[2] += frametime * cl.h2_Effects[index].Xbow.vel[i][2];

							CL_LinkEntity(ent);
						}
						else if (cl.h2_Effects[index].Xbow.bolts == 5)//fiery bolts don't just go away
						{
							if (cl.h2_Effects[index].Xbow.state[i] == 0)//waiting to explode state
							{
								if (cl.h2_Effects[index].Xbow.gonetime[i] > cl.serverTimeFloat)//fiery bolts stick around for a while
								{
									ent = &EffectEntities[cl.h2_Effects[index].Xbow.ent[i]];
									CL_LinkEntity(ent);
								}
								else//when time's up on fiery guys, they explode
								{
									//set state to exploding
									cl.h2_Effects[index].Xbow.state[i] = 1;

									ent = &EffectEntities[cl.h2_Effects[index].Xbow.ent[i]];

									//move bolt back a little to make explosion look better
									VectorNormalize(cl.h2_Effects[index].Xbow.vel[i]);
									VectorScale(cl.h2_Effects[index].Xbow.vel[i],-8,cl.h2_Effects[index].Xbow.vel[i]);
									VectorAdd(ent->state.origin,cl.h2_Effects[index].Xbow.vel[i],ent->state.origin);

									//turn bolt entity into an explosion
									ent->model = R_RegisterModel("models/xbowexpl.spr");
									ent->state.frame = 0;

									//set frame change counter
									cl.h2_Effects[index].Xbow.gonetime[i] = cl.serverTimeFloat + HX_FRAME_TIME * 2;

									//play explosion sound
									S_StartSound(ent->state.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_explode, 1, 1);

									CL_LinkEntity(ent);
								}
							}
							else if (cl.h2_Effects[index].Xbow.state[i] == 1)//fiery bolt exploding state
							{
								ent = &EffectEntities[cl.h2_Effects[index].Xbow.ent[i]];

								//increment frame if it's time
								while(cl.h2_Effects[index].Xbow.gonetime[i] <= cl.serverTimeFloat)
								{
									ent->state.frame++;
									cl.h2_Effects[index].Xbow.gonetime[i] += HX_FRAME_TIME * 0.75;
								}


								if (ent->state.frame >= R_ModelNumFrames(ent->model))
								{
									cl.h2_Effects[index].Xbow.state[i] = 2;//if anim is over, set me to inactive state
								}
								else
									CL_LinkEntity(ent);
							}
						}
					}
				}
				break;
			case HWCE_HWSHEEPINATOR:
				cl.h2_Effects[index].Xbow.time_amount += frametime;
				for (i=0;i<cl.h2_Effects[index].Xbow.bolts;i++)
				{
					if (cl.h2_Effects[index].Xbow.ent[i] != -1)//only update valid effect ents
					{
						if (cl.h2_Effects[index].Xbow.activebolts & (1<<i))//bolt in air, simply update position
						{
							ent = &EffectEntities[cl.h2_Effects[index].Xbow.ent[i]];

							ent->state.origin[0] += frametime * cl.h2_Effects[index].Xbow.vel[i][0];
							ent->state.origin[1] += frametime * cl.h2_Effects[index].Xbow.vel[i][1];
							ent->state.origin[2] += frametime * cl.h2_Effects[index].Xbow.vel[i][2];

							CLH2_RunParticleEffect4(ent->state.origin,7,(rand()%15)+144,pt_h2explode2,(rand()%5)+1);

							CL_LinkEntity(ent);
						}
					}
				}
				break;
			case HWCE_DEATHBUBBLES:
				cl.h2_Effects[index].Bubble.time_amount += frametime;
				if (cl.h2_Effects[index].Bubble.time_amount > 0.1)//10 bubbles a sec
				{
					cl.h2_Effects[index].Bubble.time_amount = 0;
					cl.h2_Effects[index].Bubble.count--;
					es = FindState(cl.h2_Effects[index].Bubble.owner);
					if (es)
					{
						VectorCopy(es->origin,org);
						VectorAdd(org,cl.h2_Effects[index].Bubble.offset,org);

						if (CM_PointContentsQ1(org, 0) != BSP29CONTENTS_WATER) 
						{
							//not in water anymore
							CLH2_FreeEffect(index);
							break;
						}
						else
						{
							CLTENT_SpawnDeathBubble(org);
						}
					}
				}
				if (cl.h2_Effects[index].Bubble.count <= 0)
					CLH2_FreeEffect(index);
				break;
			case HWCE_SCARABCHAIN:
				cl.h2_Effects[index].Chain.time_amount += frametime;
				ent = &EffectEntities[cl.h2_Effects[index].Chain.ent1];

				switch (cl.h2_Effects[index].Chain.state)
				{
				case 0://zooming in toward owner
					es = FindState(cl.h2_Effects[index].Chain.owner);
					if (cl.h2_Effects[index].Chain.sound_time <= cl.serverTimeFloat)
					{
						cl.h2_Effects[index].Chain.sound_time = cl.serverTimeFloat + 0.5;
						S_StartSound(ent->state.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_scarabhome, 1, 1);
					}
					if (es)
					{
						VectorCopy(es->origin,org);
						org[2]+=cl.h2_Effects[index].Chain.height;
						VectorSubtract(org,ent->state.origin,org);
						if (fabs(VectorNormalize(org))<500*frametime)
						{
							S_StartSound(ent->state.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_scarabgrab, 1, 1);
							cl.h2_Effects[index].Chain.state = 1;
							VectorCopy(es->origin, ent->state.origin);
							ent->state.origin[2] += cl.h2_Effects[index].Chain.height;
							XbowImpactPuff(ent->state.origin, cl.h2_Effects[index].Chain.material);
						}
						else
						{
							VectorScale(org,500*frametime,org);
							VectorAdd(ent->state.origin,org,ent->state.origin);
						}
					}
					break;
				case 1://attached--snap to owner's pos
					es = FindState(cl.h2_Effects[index].Chain.owner);
					if (es)
					{
						VectorCopy(es->origin, ent->state.origin);
						ent->state.origin[2] += cl.h2_Effects[index].Chain.height;
					}
					break;
				case 2://unattaching, server needs to set this state
					VectorCopy(ent->state.origin,org);
					VectorSubtract(cl.h2_Effects[index].Chain.origin,org,org);
					if (fabs(VectorNormalize(org))>350*frametime)//closer than 30 is too close?
					{
						VectorScale(org,350*frametime,org);
						VectorAdd(ent->state.origin,org,ent->state.origin);
					}
					else//done--flash & git outa here (change type to redflash)
					{
						S_StartSound(ent->state.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_scarabbyebye, 1, 1);
						cl.h2_Effects[index].Flash.entity_index = cl.h2_Effects[index].Chain.ent1;
						cl.h2_Effects[index].type = HWCE_RED_FLASH;
						VectorCopy(ent->state.origin,cl.h2_Effects[index].Flash.origin);
						cl.h2_Effects[index].Flash.reverse = 0;
						ent->model = R_RegisterModel("models/redspt.spr");
						ent->state.frame = 0;
						ent->state.drawflags = H2DRF_TRANSLUCENT;
					}
					break;
				}

				CL_LinkEntity(ent);

				//damndamndamn--add stream stuff here!
				VectorCopy(cl.h2_Effects[index].Chain.origin, org);
				VectorCopy(ent->state.origin, org2);
				CreateStream(TE_STREAM_CHAIN, cl.h2_Effects[index].Chain.ent1, 1, cl.h2_Effects[index].Chain.tag, 0.1, 0, org, org2);

				break;
			case HWCE_TRIPMINESTILL:
				cl.h2_Effects[index].Chain.time_amount += frametime;
				ent = &EffectEntities[cl.h2_Effects[index].Chain.ent1];

//				if (cl.h2_Effects[index].Chain.ent1 < 0)//fixme: remove this!!!
//					Con_DPrintf("OHSHITOHSHIT--bad chain ent\n");

				CL_LinkEntity(ent);
//				Con_DPrintf("Chain Ent at: %d %d %d\n",(int)cl.h2_Effects[index].Chain.origin[0],(int)cl.h2_Effects[index].Chain.origin[1],(int)cl.h2_Effects[index].Chain.origin[2]);

				//damndamndamn--add stream stuff here!
				VectorCopy(cl.h2_Effects[index].Chain.origin, org);
				VectorCopy(ent->state.origin, org2);
				CreateStream(TE_STREAM_CHAIN, cl.h2_Effects[index].Chain.ent1, 1, 1, 0.1, 0, org, org2);

				break;
			case HWCE_TRIPMINE:
				cl.h2_Effects[index].Chain.time_amount += frametime;
				ent = &EffectEntities[cl.h2_Effects[index].Chain.ent1];

				ent->state.origin[0] += frametime * cl.h2_Effects[index].Chain.velocity[0];
				ent->state.origin[1] += frametime * cl.h2_Effects[index].Chain.velocity[1];
				ent->state.origin[2] += frametime * cl.h2_Effects[index].Chain.velocity[2];

				CL_LinkEntity(ent);

				//damndamndamn--add stream stuff here!
				VectorCopy(cl.h2_Effects[index].Chain.origin, org);
				VectorCopy(ent->state.origin, org2);
				CreateStream(TE_STREAM_CHAIN, cl.h2_Effects[index].Chain.ent1, 1, 1, 0.1, 0, org, org2);

				break;
			case HWCE_BONESHARD:
			case HWCE_BONESHRAPNEL:
			case HWCE_HWBONEBALL:
			case HWCE_HWRAVENSTAFF:
			case HWCE_HWRAVENPOWER:
				cl.h2_Effects[index].Missile.time_amount += frametime;
				ent = &EffectEntities[cl.h2_Effects[index].Missile.entity_index];

//		ent->angles[0] = cl.h2_Effects[index].Missile.angle[0];
//		ent->angles[1] = cl.h2_Effects[index].Missile.angle[1];
//		ent->angles[2] = cl.h2_Effects[index].Missile.angle[2];

				ent->state.angles[0] += frametime * cl.h2_Effects[index].Missile.avelocity[0];
				ent->state.angles[1] += frametime * cl.h2_Effects[index].Missile.avelocity[1];
				ent->state.angles[2] += frametime * cl.h2_Effects[index].Missile.avelocity[2];

				ent->state.origin[0] += frametime * cl.h2_Effects[index].Missile.velocity[0];
				ent->state.origin[1] += frametime * cl.h2_Effects[index].Missile.velocity[1];
				ent->state.origin[2] += frametime * cl.h2_Effects[index].Missile.velocity[2];
				if(cl.h2_Effects[index].type == HWCE_HWRAVENPOWER)
				{
					while(cl.h2_Effects[index].Missile.time_amount >= HX_FRAME_TIME)
					{
						ent->state.frame++;
						cl.h2_Effects[index].Missile.time_amount -= HX_FRAME_TIME;
					}

					if (ent->state.frame > 7)
					{
						ent->state.frame = 0;
						break;
					}
				}
				CL_LinkEntity(ent);
				if(cl.h2_Effects[index].type == HWCE_HWBONEBALL)
				{
					CLH2_RunParticleEffect4 (ent->state.origin, 10, 368 + rand() % 16, pt_h2slowgrav, 3);

				}
				break;
			case HWCE_HWMISSILESTAR:
			case HWCE_HWEIDOLONSTAR:
				// update scale
				if(cl.h2_Effects[index].Star.scaleDir)
				{
					cl.h2_Effects[index].Star.scale += 0.05;
					if(cl.h2_Effects[index].Star.scale >= 1)
					{
						cl.h2_Effects[index].Star.scaleDir = 0;
					}
				}
				else
				{
					cl.h2_Effects[index].Star.scale -= 0.05;
					if(cl.h2_Effects[index].Star.scale <= 0.01)
					{
						cl.h2_Effects[index].Star.scaleDir = 1;
					}
				}
				
				cl.h2_Effects[index].Star.time_amount += frametime;
				ent = &EffectEntities[cl.h2_Effects[index].Star.entity_index];

				ent->state.angles[0] += frametime * cl.h2_Effects[index].Star.avelocity[0];
				ent->state.angles[1] += frametime * cl.h2_Effects[index].Star.avelocity[1];
				ent->state.angles[2] += frametime * cl.h2_Effects[index].Star.avelocity[2];

				ent->state.origin[0] += frametime * cl.h2_Effects[index].Star.velocity[0];
				ent->state.origin[1] += frametime * cl.h2_Effects[index].Star.velocity[1];
				ent->state.origin[2] += frametime * cl.h2_Effects[index].Star.velocity[2];

				CL_LinkEntity(ent);
				
				if (cl.h2_Effects[index].Star.ent1 != -1)
				{
					ent2= &EffectEntities[cl.h2_Effects[index].Star.ent1];
					VectorCopy(ent->state.origin, ent2->state.origin);
					ent2->state.scale = cl.h2_Effects[index].Star.scale;
					ent2->state.angles[1] += frametime * 300;
					ent2->state.angles[2] += frametime * 400;
					CL_LinkEntity(ent2);
				}
				if(cl.h2_Effects[index].type == HWCE_HWMISSILESTAR)
				{
					if (cl.h2_Effects[index].Star.ent2 != -1)
					{
						ent2 = &EffectEntities[cl.h2_Effects[index].Star.ent2];
						VectorCopy(ent->state.origin, ent2->state.origin);
						ent2->state.scale = cl.h2_Effects[index].Star.scale;
						ent2->state.angles[1] += frametime * -300;
						ent2->state.angles[2] += frametime * -400;
						CL_LinkEntity(ent2);
					}
				}					
				if(rand() % 10 < 3)		
				{
					CLH2_RunParticleEffect4 (ent->state.origin, 7, 148 + rand() % 11, pt_h2grav, 10 + rand() % 10);
				}
				break;
		}
	}
}

// this creates multi effects from one packet
void CreateRavenExplosions(vec3_t pos);
void CL_ParseMultiEffect(void)
{
	int type, index, count;
	vec3_t	orig, vel;
	effect_entity_t* ent;

	type = net_message.ReadByte();
	switch(type)
	{
	case HWCE_HWRAVENPOWER:
		orig[0] = net_message.ReadCoord();
		orig[1] = net_message.ReadCoord();
		orig[2] = net_message.ReadCoord();
		vel[0] = net_message.ReadCoord();
		vel[1] = net_message.ReadCoord();
		vel[2] = net_message.ReadCoord();
		for(count=0;count<3;count++)
		{
			index = net_message.ReadByte();
			// create the effect
			cl.h2_Effects[index].type = type;
			VectorCopy(orig, cl.h2_Effects[index].Missile.origin);
			VectorCopy(vel, cl.h2_Effects[index].Missile.velocity);
			VecToAnglesBuggy(cl.h2_Effects[index].Missile.velocity, cl.h2_Effects[index].Missile.angle);
			if ((cl.h2_Effects[index].Missile.entity_index = CLH2_NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.h2_Effects[index].Missile.entity_index];
				VectorCopy(cl.h2_Effects[index].Missile.origin, ent->state.origin);
				VectorCopy(cl.h2_Effects[index].Missile.angle, ent->state.angles);
				ent->model = R_RegisterModel("models/ravproj.mdl");
				S_StartSound(cl.h2_Effects[index].Missile.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_ravengo, 1, 1);
			}
		}
		CreateRavenExplosions(orig);
		break;
	default:
		Sys_Error ("CL_ParseMultiEffect: bad type");

	}	
	
}
