
// defs common to client and server

#define GLQUAKE_VERSION 0.95
#define	VERSION		0.15
#define LINUX_VERSION 0.94


#ifdef SERVERONLY		// no asm in dedicated server
#undef id386
#endif

#if id386
#define UNALIGNED_OK	1	// set to 0 if unaligned accesses are not supported
#else
#define UNALIGNED_OK	0
#endif

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
#define CACHE_SIZE	32		// used to align key data structures

#define UNUSED(x)	(x = x)	// for pesky compiler / lint warnings

#define	MINIMUM_MEMORY	0x550000


#define	SOUND_CHANNELS		8


#define	ON_EPSILON		0.1			// point on plane side epsilon

//
// per-level limits
//
#define	MAX_LIGHTSTYLES_H2	64
#define	MAX_MODELS		512			// Sent over the net as a word
#define	MAX_SOUNDS		256			// so they cannot be blindly increased

#define	SAVEGAME_COMMENT_LENGTH	39

//
// stats are integers communicated to the client by the server
//
#define	MAX_CL_STATS		32
#define	STAT_HEALTH			0
//define	STAT_FRAGS			1
#define	STAT_WEAPON			2
#define	STAT_AMMO			3
#define	STAT_ARMOR			4
//define	STAT_WEAPONFRAME	5
#define	STAT_SHELLS			6
#define	STAT_NAILS			7
#define	STAT_ROCKETS		8
#define	STAT_CELLS			9
#define	STAT_ACTIVEWEAPON	10
#define	STAT_TOTALSECRETS	11
#define	STAT_TOTALMONSTERS	12
#define	STAT_SECRETS		13		// bumped on client side by svc_foundsecret
#define	STAT_MONSTERS		14		// bumped by svc_killedmonster
#define	STAT_ITEMS			15
//define	STAT_VIEWHEIGHT		16

#define	MAX_INVENTORY			15		// Max inventory array size


//
// item flags
//
#define	IT_SHOTGUN				1
#define	IT_SUPER_SHOTGUN		2
#define	IT_NAILGUN				4
#define	IT_SUPER_NAILGUN		8

#define	IT_GRENADE_LAUNCHER		16
#define	IT_ROCKET_LAUNCHER		32
#define	IT_LIGHTNING			64
#define	IT_SUPER_LIGHTNING		128

#define	IT_SHELLS				256
#define	IT_NAILS				512
#define	IT_ROCKETS				1024
#define	IT_CELLS				2048

#define	IT_AXE					4096

#define	IT_ARMOR1				8192
#define	IT_ARMOR2				16384
#define	IT_ARMOR3				32768

#define	IT_SUPERHEALTH			65536

#define	IT_KEY1					131072
#define	IT_KEY2					262144

#define	IT_INVISIBILITY			524288

#define	IT_INVULNERABILITY		1048576
#define	IT_SUIT					2097152
#define	IT_QUAD					4194304

#define	IT_SIGIL1				(1<<28)

#define	IT_SIGIL2				(1<<29)
#define	IT_SIGIL3				(1<<30)
#define	IT_SIGIL4				(1<<31)

#define ART_HASTE					1
#define ART_INVINCIBILITY			2
#define ART_TOMEOFPOWER				4
#define ART_INVISIBILITY			8
#define ARTFLAG_FROZEN				16
#define ARTFLAG_STONED				32
#define ARTFLAG_DIVINE_INTERVENTION 64
#define ARTFLAG_BOOTS				128

//
// print flags
//
#define	PRINT_LOW			0		// pickup messages
#define	PRINT_MEDIUM		1		// death messages
#define	PRINT_HIGH			2		// critical messages
#define	PRINT_CHAT			3		// chat messages
#define PRINT_SOUND			4		// says a sound

//Siege teams
#define ST_DEFENDER					1
#define ST_ATTACKER   				2

//Dm Modes
#define DM_CAPTURE_THE_TOKEN		1
#define DM_HUNTER					2
#define DM_SIEGE					3

// entity effects

#define EF_ONFIRE				0x00000001
#define	EF_MUZZLEFLASH 			0x00000002
#define	EF_BRIGHTLIGHT 			0x00000004
#define	EF_DIMLIGHT 			0x00000008
#define EF_DARKLIGHT			0x00000010
#define EF_DARKFIELD			0x00000020
#define EF_LIGHT				0x00000040
#define EF_NODRAW				0x00000080

#define	EF_BRIGHTFIELD			0x00000400
#define EF_POWERFLAMEBURN		0x00000800
#define EF_UPDATESOUND			0x00002000

#define EF_POISON_GAS			0x00200000
#define EF_ACIDBLOB				0x00400000
//#define EF_PURIFY2_EFFECT		0x00200000
//#define EF_AXE_EFFECT			0x00400000
//#define EF_SWORD_EFFECT		0x00800000
//#define EF_TORNADO_EFFECT		0x01000000
#define EF_ICESTORM_EFFECT		0x02000000
//#define EF_ICEBALL_EFFECT		0x04000000
//#define EF_METEOR_EFFECT		0x08000000
#define EF_HAMMER_EFFECTS		0x10000000
#define EF_BEETLE_EFFECTS		0x20000000
