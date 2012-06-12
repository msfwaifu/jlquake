/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// server.h

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../../server/et/g_public.h"
#include "../game/bg_public.h"
#include "../../common/file_formats/md3.h"
#include "../qcommon/qfiles.h"
#include "../../server/server.h"
#include "../../server/tech3/local.h"
#include "../../server/et/local.h"

//=============================================================================

#define AUTHORIZE_TIMEOUT   5000


#define MAX_MASTERS                         8				// max recipients for heartbeat packets

#define SERVER_PERFORMANCECOUNTER_FRAMES    600

//=============================================================================

#define MAX_MASTER_SERVERS  5

extern Cvar* sv_fps;
extern Cvar* sv_timeout;
extern Cvar* sv_zombietime;
extern Cvar* sv_rconPassword;
extern Cvar* sv_privatePassword;
extern Cvar* sv_allowDownload;
extern Cvar* sv_friendlyFire;			// NERVE - SMF
extern Cvar* sv_maxlives;				// NERVE - SMF
extern Cvar* sv_maxclients;
extern Cvar* sv_needpass;

extern Cvar* sv_privateClients;
extern Cvar* sv_hostname;
extern Cvar* sv_master[MAX_MASTER_SERVERS];
extern Cvar* sv_reconnectlimit;
extern Cvar* sv_tempbanmessage;
extern Cvar* sv_showloss;
extern Cvar* sv_padPackets;
extern Cvar* sv_killserver;
extern Cvar* sv_mapname;
extern Cvar* sv_mapChecksum;
extern Cvar* sv_serverid;
extern Cvar* sv_maxRate;
extern Cvar* sv_minPing;
extern Cvar* sv_maxPing;
//extern	Cvar	*sv_gametype;
extern Cvar* sv_pure;
extern Cvar* sv_floodProtect;
extern Cvar* sv_allowAnonymous;
extern Cvar* sv_lanForceRate;
extern Cvar* sv_onlyVisibleClients;

extern Cvar* sv_showAverageBPS;				// NERVE - SMF - net debugging

extern Cvar* g_gameType;

// Rafael gameskill
//extern	Cvar	*sv_gameskill;
// done

extern Cvar* sv_reloading;

// TTimo - autodl
extern Cvar* sv_dl_maxRate;

// TTimo
extern Cvar* sv_wwwDownload;// general flag to enable/disable www download redirects
extern Cvar* sv_wwwBaseURL;	// the base URL of all the files
// tell clients to perform their downloads while disconnected from the server
// this gets you a better throughput, but you loose the ability to control the download usage
extern Cvar* sv_wwwDlDisconnected;
extern Cvar* sv_wwwFallbackURL;

//bani
extern Cvar* sv_cheats;
extern Cvar* sv_packetloss;
extern Cvar* sv_packetdelay;

//fretn
extern Cvar* sv_fullmsg;

//===========================================================

//
// sv_main.c
//
void SV_FinalCommand(const char* cmd, qboolean disconnect);		// ydnar: added disconnect flag so map changes can use this function as well
void QDECL SV_SendServerCommand(client_t* cl, const char* fmt, ...);


void SV_AddOperatorCommands(void);
void SV_RemoveOperatorCommands(void);


void SV_MasterHeartbeat(const char* hbname);
void SV_MasterShutdown(void);

void SV_MasterGameCompleteStatus();		// NERVE - SMF
//bani - bugtraq 12534
qboolean SV_VerifyChallenge(char* challenge);


//
// sv_init.c
//
void SV_SetConfigstringNoUpdate(int index, const char* val);
void SV_SetConfigstring(int index, const char* val);
void SV_UpdateConfigStrings(void);
void SV_GetConfigstring(int index, char* buffer, int bufferSize);

void SV_SetUserinfo(int index, const char* val);
void SV_GetUserinfo(int index, char* buffer, int bufferSize);

void SV_CreateBaseline(void);

void SV_ChangeMaxClients(void);
void SV_SpawnServer(char* server, qboolean killBots);



//
// sv_client.c
//
void SV_GetChallenge(netadr_t from);

void SV_DirectConnect(netadr_t from);

void SV_AuthorizeIpPacket(netadr_t from);

void SV_ExecuteClientMessage(client_t* cl, QMsg* msg);
void SV_UserinfoChanged(client_t* cl);

void SV_ClientEnterWorld(client_t* client, etusercmd_t* cmd);
void SV_FreeClientNetChan(client_t* client);
void SV_DropClient(client_t* drop, const char* reason);

void SV_ExecuteClientCommand(client_t* cl, const char* s, qboolean clientOK, qboolean premaprestart);
void SV_ClientThink(client_t* cl, etusercmd_t* cmd);

void SV_WriteDownloadToClient(client_t* cl, QMsg* msg);

//
// sv_ccmds.c
//
void SV_Heartbeat_f(void);

qboolean SV_TempBanIsBanned(netadr_t address);
void SV_TempBanNetAddress(netadr_t address, int length);

//
// sv_snapshot.c
//
void SV_AddServerCommand(client_t* client, const char* cmd);
void SV_UpdateServerCommandsToClient(client_t* client, QMsg* msg);
void SV_WriteFrameToClient(client_t* client, QMsg* msg);
void SV_SendMessageToClient(QMsg* msg, client_t* client);
void SV_SendClientMessages(void);
void SV_SendClientSnapshot(client_t* client);
//bani
void SV_SendClientIdle(client_t* client);

//
// sv_game.c
//
void        SV_InitGameProgs(void);
void        SV_ShutdownGameProgs(void);
void        SV_RestartGameProgs(void);
qboolean SV_GetTag(int clientNum, int tagFileNumber, char* tagname, orientation_t* _or);
int SV_LoadTag(const char* mod_name);
qboolean    SV_GameIsSinglePlayer(void);
qboolean    SV_GameIsCoop(void);
void        SV_GameBinaryMessageReceived(int cno, const char* buf, int buflen, int commandTime);

//
// sv_bot.c
//
void        SV_BotFrame(int time);
int         SV_BotAllocateClient(int clientNum);
void        SV_BotFreeClient(int clientNum);

void        SV_BotInitCvars(void);
int         SV_BotLibSetup(void);
int         SV_BotLibShutdown(void);
int         SV_BotGetSnapshotEntity(int client, int ent);
int         SV_BotGetConsoleMessage(int client, char* buf, int size);

//============================================================
//
// high level object sorting to reduce interaction tests
//

//
// sv_net_chan.c
//
void SV_Netchan_Transmit(client_t* client, QMsg* msg);
void SV_Netchan_TransmitNextFragment(client_t* client);
qboolean SV_Netchan_Process(client_t* client, QMsg* msg);

//bani - cl->downloadnotify
#define DLNOTIFY_REDIRECT   0x00000001	// "Redirecting client ..."
#define DLNOTIFY_BEGIN      0x00000002	// "clientDownload: 4 : beginning ..."
#define DLNOTIFY_ALL        (DLNOTIFY_REDIRECT | DLNOTIFY_BEGIN)
