// server.h

#define QW_SERVER

#define MAX_MASTERS 8				// max recipients for heartbeat packets

#define MAX_SIGNON_BUFFERS  8

typedef enum {
	ss_dead,			// no map loaded
	ss_loading,			// spawning level edicts
	ss_active			// actively running
} server_state_t;
// some qc commands are only valid before the server has finished
// initializing (precache commands, static sounds / objects, etc)

typedef struct
{
	qboolean active;					// false when server is going down
	server_state_t state;			// precache commands are only valid during load

	double time;

	int lastcheck;					// used by PF_checkclient
	double lastchecktime;			// for monster ai
	double next_PIV_time;			// Last Player In View time

	char name[64];					// map name
	char midi_name[128];			// midi file name
	byte cd_track;					// cd track number
	int current_skill;

	char startspot[64];
	char modelname[MAX_QPATH];				// maps/<name>.bsp, for model_precache[0]
	const char* model_precache[MAX_MODELS_H2];	// NULL terminated
	const char* sound_precache[MAX_SOUNDS_HW];	// NULL terminated
	const char* lightstyles[MAX_LIGHTSTYLES_H2];
	clipHandle_t models[MAX_MODELS_H2];

	struct h2EffectT Effects[MAX_EFFECTS_H2];

	int num_edicts;					// increases towards MAX_EDICTS_H2
	qhedict_t* edicts;					// can NOT be array indexed, because
	// qhedict_t is variable sized, but can
	// be used to reference the world ent

	// added to every client's unreliable buffer each frame, then cleared
	QMsg datagram;
	byte datagram_buf[MAX_DATAGRAM_HW];

	// added to every client's reliable buffer each frame, then cleared
	QMsg reliable_datagram;
	byte reliable_datagram_buf[MAX_MSGLEN_HW];

	// the multicast buffer is used to send a message to a set of clients
	QMsg multicast;
	byte multicast_buf[MAX_MSGLEN_HW];

	// the master buffer is used for building log packets
	QMsg master;
	byte master_buf[MAX_DATAGRAM_HW];

	// the signon buffer will be sent to each client as they connect
	// includes the entity baselines, the static entities, etc
	// large levels will have >MAX_DATAGRAM_HW sized signons, so
	// multiple signon messages are kept
	QMsg signon;
	int num_signon_buffers;
	int signon_buffer_size[MAX_SIGNON_BUFFERS];
	byte signon_buffers[MAX_SIGNON_BUFFERS][MAX_DATAGRAM_HW];
} server_t;


#define NUM_SPAWN_PARMS         16

typedef enum
{
	cs_free,		// can be reused for a new connection
	cs_zombie,		// client has been disconnected, but don't reuse
					// connection for a couple seconds
	cs_connected,	// has been assigned to a client_t, but not in game yet
	cs_spawned		// client is fully in game
} client_state_t;

typedef struct
{
	// received from client

	// reply
	double senttime;
	float ping_time;
	hwpacket_entities_t entities;
} client_frame_t;

typedef struct client_s
{
	client_state_t state;

	int spectator;						// non-interactive

	qboolean sendinfo;					// at end of frame, send info to all
										// this prevents malicious multiple broadcasts
	qboolean portals;					// They have portals mission pack installed
	int userid;										// identifying number
	char userinfo[HWMAX_INFO_STRING];					// infostring

	hwusercmd_t lastcmd;				// for filling in big drops and partial predictions
	double localtime;					// of last message
	int oldbuttons;

	float maxspeed;						// localized maxspeed
	float entgravity;					// localized ent gravity

	qhedict_t* edict;						// EDICT_NUM(clientnum+1)
	char name[32];						// for printing to other people
										// extracted from userinfo
	int playerclass;
	int siege_team;
	int next_playerclass;
	int messagelevel;					// for filtering printed messages

	// the datagram is written to after every frame, but only cleared
	// when it is sent out to the client.  overflow is tolerated.
	QMsg datagram;
	byte datagram_buf[MAX_DATAGRAM_HW];

	double connection_started;			// or time of disconnect for zombies
	qboolean send_message;				// set on frames a datagram arived on

// spawn parms are carried from level to level
	float spawn_parms[NUM_SPAWN_PARMS];

// client known data for deltas
	int old_frags;

	int stats[MAX_CL_STATS];

	client_frame_t frames[UPDATE_BACKUP_HW];	// updates can be deltad from here

	fileHandle_t download;				// file being downloaded
	int downloadsize;					// total bytes
	int downloadcount;					// bytes sent

	int spec_track;						// entnum of player tracking

	double whensaid[10];				// JACK: For floodprots
	int whensaidhead;					// Head value for floodprots
	double lockedtill;

//===== NETWORK ============
	int chokecount;
	int delta_sequence;					// -1 = no compression
	netchan_t netchan;

	h2client_entvars_t old_v;
	qboolean send_all_v;

	unsigned PIV, LastPIV;				// people in view
	qboolean skipsend;					// Skip sending this frame, guaranteed to send next frame
} client_t;

// a client can leave the server in one of four ways:
// dropping properly by quiting or disconnecting
// timing out if no valid messages are received for timeout.value seconds
// getting kicked off by the server operator
// a program error, like an overflowed reliable buffer

//=============================================================================


#define STATFRAMES  100
typedef struct
{
	double active;
	double idle;
	int count;
	int packets;

	double latched_active;
	double latched_idle;
	int latched_packets;
} svstats_t;


typedef struct
{
	int spawncount;					// number of servers spawned since start,
									// used to check late spawns
	client_t clients[HWMAX_CLIENTS];
	int serverflags;				// episode completion information
	qboolean changelevel_issued;	// cleared when at SV_SpawnServer

	double last_heartbeat;
	int heartbeat_sequence;
	svstats_t stats;

	char info[MAX_SERVERINFO_STRING];

	// log messages are used so that fraglog processes can get stats
	int logsequence;			// the message currently being filled
	double logtime;				// time of last swap
	QMsg log[2];
	byte log_buf[2][MAX_DATAGRAM_HW];
} server_static_t;

//=============================================================================


// edict->solid values
#define SOLID_NOT               0		// no interaction with other objects
#define SOLID_TRIGGER           1		// touch on edge, but not blocking
#define SOLID_BBOX              2		// touch on edge, block
#define SOLID_SLIDEBOX          3		// touch on edge, but not an onground
#define SOLID_BSP               4		// bsp clip, touch on edge, block
#define SOLID_PHASE             5		// won't slow down when hitting entities flagged as FL_MONSTER

// edict->deadflag values
#define DEAD_NO                 0
#define DEAD_DYING              1
#define DEAD_DEAD               2

#define DAMAGE_NO               0
#define DAMAGE_YES              1
#define DAMAGE_AIM              2

// edict->flags
#define FL_FLY                  1
#define FL_SWIM                 2
#define FL_CONVEYOR             4
#define FL_CLIENT               8
#define FL_INWATER              16
#define FL_MONSTER              32
#define FL_GODMODE              64
#define FL_NOTARGET             128
#define FL_ITEM                 256
#define FL_ONGROUND             512
#define FL_PARTIALGROUND        1024	// not all corners are valid
#define FL_WATERJUMP            2048	// player jumping out of water
#define FL_JUMPRELEASED         4096	// for jump debouncing
#define FL_FLASHLIGHT           8192
#define FL_ARCHIVE_OVERRIDE     1048576
#define FL_ARTIFACTUSED         16384
#define FL_MOVECHAIN_ANGLE      32768	// when in a move chain, will update the angle
#define FL_CLASS_DEPENDENT      2097152	// model will appear different to each player
#define FL_SPECIAL_ABILITY1     4194304	// has 1st special ability
#define FL_SPECIAL_ABILITY2     8388608	// has 2nd special ability

#define FL2_CROUCHED            4096

// Built-in Spawn Flags
#define SPAWNFLAG_NOT_PALADIN       256
#define SPAWNFLAG_NOT_CLERIC        512
#define SPAWNFLAG_NOT_NECROMANCER   1024
#define SPAWNFLAG_NOT_THEIF         2048
#define SPAWNFLAG_NOT_EASY          4096
#define SPAWNFLAG_NOT_MEDIUM        8192
#define SPAWNFLAG_NOT_HARD          16384
#define SPAWNFLAG_NOT_DEATHMATCH    32768
#define SPAWNFLAG_NOT_COOP          65536
#define SPAWNFLAG_NOT_SINGLE        131072

// server flags
#define SFL_EPISODE_1       1
#define SFL_EPISODE_2       2
#define SFL_EPISODE_3       4
#define SFL_EPISODE_4       8
#define SFL_NEW_UNIT        16
#define SFL_NEW_EPISODE     32
#define SFL_CROSS_TRIGGERS  65280

#define MULTICAST_ALL           0
#define MULTICAST_PHS           1
#define MULTICAST_PVS           2

#define MULTICAST_ALL_R         3
#define MULTICAST_PHS_R         4
#define MULTICAST_PVS_R         5

//============================================================================

extern Cvar* sv_mintic;
extern Cvar* sv_maxtic;
extern Cvar* sv_maxspeed;
extern Cvar* sv_highchars;

extern netadr_t master_adr[MAX_MASTERS];		// address of the master server

extern Cvar* spawn;
extern Cvar* teamplay;
extern Cvar* skill;
extern Cvar* deathmatch;
extern Cvar* coop;
extern Cvar* maxclients;
extern Cvar* randomclass;
extern Cvar* damageScale;
extern Cvar* meleeDamScale;
extern Cvar* shyRespawn;
extern Cvar* spartanPrint;
extern Cvar* manaScale;
extern Cvar* tomeMode;
extern Cvar* tomeRespawn;
extern Cvar* w2Respawn;
extern Cvar* altRespawn;
extern Cvar* fixedLevel;
extern Cvar* autoItems;
extern Cvar* dmMode;
extern Cvar* easyFourth;
extern Cvar* patternRunner;
extern Cvar* fraglimit;
extern Cvar* timelimit;
extern Cvar* noexit;

extern server_static_t svs;					// persistant server info
extern server_t sv;							// local server

extern client_t* host_client;

extern qhedict_t* sv_player;

extern char localmodels[MAX_MODELS_H2][5];			// inline model names for precache

extern char localinfo[MAX_LOCALINFO_STRING + 1];

extern int host_hunklevel;
extern fileHandle_t sv_logfile;
extern fileHandle_t sv_fraglogfile;

extern int sv_net_port;

//===========================================================

//
// sv_main.c
//
void SV_Shutdown(void);
void SV_Frame(float time);
void SV_FinalMessage(const char* message);
void SV_DropClient(client_t* drop);

int SV_CalcPing(client_t* cl);
void SV_FullClientUpdate(client_t* client, QMsg* buf);

int SV_ModelIndex(const char* name);

qboolean SV_CheckBottom(qhedict_t* ent);
qboolean SV_movestep(qhedict_t* ent, vec3_t move, qboolean relink, qboolean noenemy, qboolean set_trace);

void SV_WriteClientdataToMessage(client_t* client, QMsg* msg);

void SV_MoveToGoal(void);

void SV_SaveSpawnparms(void);

void SV_Physics_Client(qhedict_t* ent);

void SV_ExecuteUserCommand(char* s);
void SV_InitOperatorCommands(void);

void SV_SendServerinfo(client_t* client);
void SV_ExtractFromUserinfo(client_t* cl);


void Master_Heartbeat(void);
void Master_Packet(void);

//
// sv_init.c
//
void SV_SpawnServer(char* server, char* startspot);


//
// sv_phys.c
//
void SV_ProgStartFrame(void);
void SV_Physics(void);
void SV_CheckVelocity(qhedict_t* ent);
void SV_AddGravity(qhedict_t* ent, float scale);
qboolean SV_RunThink(qhedict_t* ent);
void SV_Physics_Toss(qhedict_t* ent);
void SV_RunNewmis(void);
void SV_Impact(qhedict_t* e1, qhedict_t* e2);

//
// sv_send.c
//
extern unsigned clients_multicast;

void SV_SendClientMessages(void);

void SV_Multicast(vec3_t origin, int to);
void SV_MulticastSpecific(unsigned clients, qboolean reliable);
void SV_StartSound(qhedict_t* entity, int channel, const char* sample, int volume,
	float attenuation);
void SV_StartParticle(vec3_t org, vec3_t dir, int color, int count);
void SV_StartParticle2(vec3_t org, vec3_t dmin, vec3_t dmax, int color, int effect, int count);
void SV_StartParticle3(vec3_t org, vec3_t box, int color, int effect, int count);
void SV_StartParticle4(vec3_t org, float radius, int color, int effect, int count);
void SV_StartRainEffect(vec3_t org, vec3_t e_size, int x_dir, int y_dir, int color, int count);
void SV_ClientPrintf(client_t* cl, int level, const char* fmt, ...);
void SV_BroadcastPrintf(int level, const char* fmt, ...);
void SV_BroadcastCommand(const char* fmt, ...);
void SV_SendMessagesToAll(void);
void SV_FindModelNumbers(void);

//
// sv_user.c
//
void SV_ExecuteClientMessage(client_t* cl);
void SV_UserInit(void);


//
// svonly.c
//
typedef enum {RD_NONE, RD_CLIENT, RD_PACKET} redirect_t;
void SV_BeginRedirect(redirect_t rd);
void SV_EndRedirect(void);

//
// sv_ccmds.c
//
void SV_Status_f(void);

//
// sv_ents.c
//
void SV_WriteEntitiesToClient(client_t* client, QMsg* msg);

void SV_UpdateSoundPos(qhedict_t* entity, int channel);
void SV_StopSound(qhedict_t* entity, int channel);
void SV_ParseEffect(QMsg* sb);
void SV_FlushSignon(void);
void SV_SetMoveVars(void);
void ED_ClearEdict(qhedict_t* e);
void SV_WriteInventory(client_t* host_client, qhedict_t* ent, QMsg* msg);
void SV_SendEffect(QMsg* sb, int index);
