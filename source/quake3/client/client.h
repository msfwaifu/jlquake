/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// client.h -- primary header for client

#include "../game/q_shared.h"
#include "../../client/client.h"
#include "../qcommon/qcommon.h"
#include "../ui/ui_public.h"
#include "keys.h"
#include "../cgame/cg_public.h"
#include "../game/bg_public.h"

#define	RETRANSMIT_TIMEOUT	3000	// time between connection packet retransmits

extern int g_console_field_width;

/*
==================================================================

the clientStatic_t structure is never wiped, and is used even when
no client connection is active at all

==================================================================
*/

typedef struct {
	netadr_t	adr;
	int			start;
	int			time;
	char		info[MAX_INFO_STRING_Q3];
} ping_t;

struct clientStatic_t : clientStaticCommon_t
{
};

extern	clientStatic_t		cls;

//=============================================================================

extern	vm_t			*cgvm;	// interface to cgame dll or vm
extern	vm_t			*uivm;	// interface to ui dll or vm


//
// cvars
//
extern	Cvar	*cl_nodelta;
extern	Cvar	*cl_debugMove;
extern	Cvar	*cl_noprint;
extern	Cvar	*cl_timegraph;
extern	Cvar	*cl_maxpackets;
extern	Cvar	*cl_packetdup;
extern	Cvar	*cl_shownet;
extern	Cvar	*cl_showSend;
extern	Cvar	*cl_timeNudge;
extern	Cvar	*cl_showTimeDelta;
extern	Cvar	*cl_freezeDemo;

extern	Cvar	*cl_yawspeed;
extern	Cvar	*cl_pitchspeed;
extern	Cvar	*cl_run;
extern	Cvar	*cl_anglespeedkey;

extern	Cvar	*cl_sensitivity;
extern	Cvar	*cl_freelook;

extern	Cvar	*cl_mouseAccel;
extern	Cvar	*cl_showMouseRate;

extern	Cvar	*m_pitch;
extern	Cvar	*m_yaw;
extern	Cvar	*m_forward;
extern	Cvar	*m_side;
extern	Cvar	*m_filter;

extern	Cvar	*cl_timedemo;

extern	Cvar	*cl_activeAction;

extern	Cvar	*cl_allowDownload;
extern	Cvar	*cl_conXOffset;

//=================================================

//
// cl_main
//

void CL_Init (void);
void CL_FlushMemory(void);
void CL_ShutdownAll(void);
void CL_AddReliableCommand( const char *cmd );

void CL_StartHunkUsers( void );

void CL_Disconnect_f (void);
void CL_GetChallengePacket (void);
void CL_Vid_Restart_f( void );
void CL_Snd_Restart_f (void);
void CL_StartDemoLoop( void );
void CL_NextDemo( void );
void CL_ReadDemoMessage( void );

void CL_InitDownloads(void);
void CL_NextDownload(void);

void CL_GetPing( int n, char *buf, int buflen, int *pingtime );
void CL_GetPingInfo( int n, char *buf, int buflen );
void CL_ClearPing( int n );
int CL_GetPingQueueCount( void );

void CL_ShutdownRef( void );
void CL_InitRef( void );
qboolean CL_CDKeyValidate( const char *key, const char *checksum );
int CL_ServerStatus( char *serverAddress, char *serverStatusString, int maxLen );


//
// cl_input
//
typedef struct {
	int			down[2];		// key nums holding it down
	unsigned	downtime;		// msec timestamp
	unsigned	msec;			// msec down this frame if both a down and up happened
	qboolean	active;			// current state
	qboolean	wasPressed;		// set when down, not cleared when up
} kbutton_t;

extern	kbutton_t	in_mlook, in_klook;
extern 	kbutton_t 	in_strafe;
extern 	kbutton_t 	in_speed;

void CL_InitInput (void);
void CL_SendCmd (void);
void CL_ClearState (void);
void CL_ReadPackets (void);

void CL_WritePacket( void );
void IN_CenterView (void);

void CL_VerifyCode( void );

float CL_KeyState (kbutton_t *key);
const char *Key_KeynumToString (int keynum);

//
// cl_parse.c
//
extern int cl_connectedToPureServer;

void CL_SystemInfoChanged( void );
void CL_ParseServerMessage( QMsg *msg );

//====================================================================

void	CL_ServerInfoPacket( netadr_t from, QMsg *msg );
void	CL_LocalServers_f( void );
void	CL_GlobalServers_f( void );
void	CL_FavoriteServers_f( void );
void	CL_Ping_f( void );
qboolean CL_UpdateVisiblePings_f( int source );


//
// console
//
void Con_DrawCharacter (int cx, int line, int num);

void Con_CheckResize (void);
void Con_Init (void);
void Con_Clear_f (void);
void Con_ToggleConsole_f (void);
void Con_DrawNotify (void);
void Con_RunConsole (void);
void Con_DrawConsole (void);
void Con_PageUp( void );
void Con_PageDown( void );
void Con_Top( void );
void Con_Bottom( void );
void Con_Close( void );


//
// cl_scrn.c
//
void	SCR_Init (void);
void	SCR_UpdateScreen (void);

void	SCR_DebugGraph (float value, int color);

//
// cl_cin.c
//

void CL_PlayCinematic_f( void );
void SCR_DrawCinematic (void);
void SCR_RunCinematic (void);
void SCR_StopCinematic (void);
e_status CIN_StopCinematic(int handle);
void CIN_DrawCinematic (int handle);
void CIN_SetExtents (int handle, int x, int y, int w, int h);
void CIN_CloseAllVideos(void);

//
// cl_cgame.c
//
void CL_InitCGame( void );
void CL_ShutdownCGame( void );
qboolean CL_GameCommand( void );
void CL_CGameRendering( stereoFrame_t stereo );
void CL_SetCGameTime( void );
void CL_FirstSnapshot( void );
void CL_ShaderStateChanged(void);

//
// cl_ui.c
//
void CL_InitUI( void );
void CL_ShutdownUI( void );
int Key_GetCatcher( void );
void Key_SetCatcher( int catcher );
void LAN_LoadCachedServers();
void LAN_SaveServersToCache();


//
// cl_net_chan.c
//
void CL_Netchan_Transmit( netchan_t *chan, QMsg* msg);	//int length, const byte *data );
void CL_Netchan_TransmitNextFragment( netchan_t *chan );
qboolean CL_Netchan_Process( netchan_t *chan, QMsg *msg );
