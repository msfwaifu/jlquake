// sound.h -- client sound i/o functions

#ifndef __SOUND__
#define __SOUND__

#define DEFAULT_SOUND_PACKET_VOLUME 255
#define DEFAULT_SOUND_PACKET_ATTENUATION 1.0

void S_Init (void);
void S_Shutdown (void);
void S_StartSound (int entnum, int entchannel, sfxHandle_t sfx, vec3_t origin, float fvol,  float attenuation);
void S_StaticSound (sfxHandle_t sfx, vec3_t origin, float vol, float attenuation);
void S_StopSound (int entnum, int entchannel);
void S_UpdateSoundPos (int entnum, int entchannel, vec3_t origin);
void S_StopAllSounds(qboolean clear);
void S_ClearSoundBuffer (void);
void S_Update (vec3_t origin, vec3_t v_forward, vec3_t v_right, vec3_t v_up);
void S_ExtraUpdate (void);

void S_InitPaintChannels (void);

// ====================================================================
// User-setable variables
// ====================================================================

extern	QCvar* bgmvolume;
extern	QCvar* bgmtype;

void S_LocalSound (char *s);

void S_AmbientOff (void);
void S_AmbientOn (void);

#endif
