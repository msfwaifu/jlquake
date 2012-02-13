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

#define MAX_STATIC_ENTITIES_Q1	128			// torches, etc

extern Cvar* cl_doubleeyes;

extern q1entity_state_t clq1_baselines[MAX_EDICTS_Q1];
extern q1entity_t clq1_entities[MAX_EDICTS_Q1];
extern q1entity_t clq1_static_entities[MAX_STATIC_ENTITIES_Q1];

extern image_t* clq1_playertextures[BIGGEST_MAX_CLIENTS_Q1];

extern int clq1_spikeindex;
extern int clq1_playerindex;

extern Cvar* clqw_baseskin;
extern Cvar* clqw_noskins;

void CLQ1_SignonReply();
bool CLQW_CheckOrDownloadFile(const char* filename);

q1entity_t* CLQ1_EntityNum(int number);
void CLQ1_ParseSpawnBaseline(QMsg& message);
void CLQ1_ParseSpawnStatic(QMsg& message);
void CLQ1_ParseUpdate(QMsg& message, int bits);
void CLQW_ParsePacketEntities(QMsg& message);
void CLQW_ParseDeltaPacketEntities(QMsg& message);
void CLQW_ParsePlayerinfo(QMsg& message);
void CLQ1_SetRefEntAxis(refEntity_t* ent, vec3_t ent_angles);
void CLQ1_TranslatePlayerSkin(int playernum);
void CLQ1_LinkStaticEntities();
void CLQ1_RelinkEntities();
void CLQW_HandlePlayerSkin(refEntity_t* Ent, int PlayerNum);
void CLQW_LinkPacketEntities();

void CLQ1_InitTEnts();
void CLQ1_ClearTEnts();
void CLQ1_ParseTEnt(QMsg& message);
void CLQW_ParseTEnt(QMsg& message);
void CLQ1_UpdateTEnts();

void CLQ1_ClearProjectiles();
void CLQW_ParseNails(QMsg& message);
void CLQ1_LinkProjectiles();

void CLQW_SkinFind(q1player_info_t* sc);
byte* CLQW_SkinCache(qw_skin_t* skin);
void CLQW_SkinNextDownload();
void CLQW_SkinSkins_f();
void CLQW_SkinAllSkins_f();
