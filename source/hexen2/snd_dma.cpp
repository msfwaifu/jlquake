// snd_dma.c -- main control for any streaming sound output device

/*
 * $Header: /H2 Mission Pack/SND_DMA.C 9     4/01/98 6:43p Jmonroe $
 */

#include "quakedef.h"
#include "../../libs/client/snd_local.h"

void S_Play(void);
void S_PlayVol(void);
void S_SoundList(void);
void S_Update_();
void S_StopAllSounds(qboolean clear);
void S_StopAllSoundsC(void);

// =======================================================================
// Internal sound data & structures
// =======================================================================

static qboolean	snd_ambient = 1;

vec_t		sound_nominal_clip_dist=1000.0;

QCvar* bgmvolume;
QCvar* bgmtype;

QCvar* nosound;


// ====================================================================
// User-setable variables
// ====================================================================

void S_AmbientOff (void)
{
	snd_ambient = false;
}


void S_AmbientOn (void)
{
	snd_ambient = true;
}

/*
================
S_Init
================
*/
void S_Init (void)
{

	Con_Printf("\nSound Initialization\n");

	bgmvolume = Cvar_Get("bgmvolume", "1", CVAR_ARCHIVE);
	bgmtype = Cvar_Get("bgmtype", "cd", CVAR_ARCHIVE);   // cd or midi
	s_volume = Cvar_Get("volume", "0.7", CVAR_ARCHIVE);
	nosound = Cvar_Get("nosound", "0", 0);
	ambient_level = Cvar_Get("ambient_level", "0.3", 0);
	ambient_fade = Cvar_Get("ambient_fade", "100", 0);
	snd_noextraupdate = Cvar_Get("snd_noextraupdate", "0", 0);
	s_show = Cvar_Get("s_show", "0", 0);
	s_mixahead = Cvar_Get("s_mixahead", "0.1", CVAR_ARCHIVE);
	s_testsound = Cvar_Get ("s_testsound", "0", CVAR_CHEAT);

	if (COM_CheckParm("-nosound"))
		return;

	Cmd_AddCommand("play", S_Play);
	Cmd_AddCommand("playvol", S_PlayVol);
	Cmd_AddCommand("stopsound", S_StopAllSoundsC);
	Cmd_AddCommand("soundlist", S_SoundList);
	Cmd_AddCommand("soundinfo", S_SoundInfo_f);

	bool r = SNDDMA_Init();

	if (r)
	{
		s_soundStarted = 1;

		s_numSfx = 0;

		ambient_sfx[BSP29AMBIENT_WATER] = S_PrecacheSound ("ambience/water1.wav");
		ambient_sfx[BSP29AMBIENT_SKY] = S_PrecacheSound ("ambience/wind2.wav");

		S_StopAllSounds (true);

		S_SoundInfo_f();
	}
}


// =======================================================================
// Shutdown sound engine
// =======================================================================

void S_Shutdown(void)
{
	if (!s_soundStarted)
		return;

	s_soundStarted = 0;

	SNDDMA_Shutdown();
}


// =======================================================================
// Load a sound
// =======================================================================

/*
==================
S_FindName

==================
*/
sfx_t *S_FindName (char *name)
{
	int		i;
	sfx_t	*sfx;

	if (!name)
		Sys_Error ("S_FindName: NULL\n");

	if (QStr::Length(name) >= MAX_QPATH)
		Sys_Error ("Sound name too long: %s", name);

// see if already loaded
	for (i=0 ; i < s_numSfx; i++)
		if (!QStr::Cmp(s_knownSfx[i].Name, name))
		{
			return &s_knownSfx[i];
		}

	if (s_numSfx == MAX_SFX)
		Sys_Error ("S_FindName: out of sfx_t");
	
	sfx = &s_knownSfx[i];
	QStr::Cpy(sfx->Name, name);

	s_numSfx++;
	
	return sfx;
}


/*
==================
S_TouchSound

==================
*/
void S_TouchSound (char *name)
{
	sfx_t	*sfx;
	
	if (!s_soundStarted)
		return;

	sfx = S_FindName (name);
}

/*
==================
S_PrecacheSound

==================
*/
sfx_t *S_PrecacheSound (char *name)
{
	sfx_t	*sfx;

	if (!s_soundStarted || nosound->value)
		return NULL;

	sfx = S_FindName (name);
	
	// cache it in
	S_LoadSound (sfx);
	
	return sfx;
}


//=============================================================================

/*
=================
SND_PickChannel
=================
*/
channel_t *SND_PickChannel(int entnum, int entchannel)
{
    int ch_idx;
    int first_to_die;
    int life_left;

// Check for replacement sound, or find the best one to replace
    first_to_die = -1;
    life_left = 0x7fffffff;
    for (ch_idx=0; ch_idx < MAX_CHANNELS ; ch_idx++)
    {
		if (entchannel != 0		// channel 0 never overrides
		&& s_channels[ch_idx].entnum == entnum
		&& (s_channels[ch_idx].entchannel == entchannel || entchannel == -1) )
		{	// allways override sound from same entity
			first_to_die = ch_idx;
			break;
		}

		// don't let monster sounds override player sounds
		if (s_channels[ch_idx].entnum == cl.viewentity && entnum != cl.viewentity && s_channels[ch_idx].sfx)
			continue;

		if (!s_channels[ch_idx].sfx)
		{
			if (life_left > 0)
			{
				life_left = 0;
				first_to_die = ch_idx;
			}
			continue;
		}

		if (s_channels[ch_idx].startSample + s_channels[ch_idx].sfx->Length - s_paintedtime < life_left)
		{
			life_left = s_channels[ch_idx].startSample + s_channels[ch_idx].sfx->Length - s_paintedtime;
			first_to_die = ch_idx;
		}
   }

	if (first_to_die == -1)
		return NULL;

	if (s_channels[first_to_die].sfx)
		s_channels[first_to_die].sfx = NULL;

    return &s_channels[first_to_die];    
}       

/*
=================
SND_Spatialize
=================
*/
void SND_Spatialize(channel_t *ch)
{
    vec_t dot;
    vec_t ldist, rdist, dist;
    vec_t lscale, rscale, scale;
    vec3_t source_vec;
	sfx_t *snd;

// anything coming from the view entity will allways be full volume
	if (ch->entnum == cl.viewentity)
	{
		ch->leftvol = ch->master_vol;
		ch->rightvol = ch->master_vol;
		return;
	}

// calculate stereo seperation and distance attenuation

	snd = ch->sfx;
	VectorSubtract(ch->origin, listener_origin, source_vec);
	
	dist = VectorNormalize(source_vec) * ch->dist_mult;
	
	dot = DotProduct(listener_axis[1], source_vec);

	if (dma.channels == 1)
	{
		rscale = 1.0;
		lscale = 1.0;
	}
	else
	{
		rscale = 1.0 - dot;
		lscale = 1.0 + dot;
	}

// add in distance effect
	scale = (1.0 - dist) * rscale;
	ch->rightvol = (int) (ch->master_vol * scale);
	if (ch->rightvol < 0)
		ch->rightvol = 0;

	scale = (1.0 - dist) * lscale;
	ch->leftvol = (int) (ch->master_vol * scale);
	if (ch->leftvol < 0)
		ch->leftvol = 0;
}           


// =======================================================================
// Start a sound effect
// =======================================================================

void S_StartSound(int entnum, int entchannel, sfx_t *sfx, vec3_t origin, float fvol, float attenuation)
{
	channel_t *target_chan, *check;
	int		vol;
	int		ch_idx;
	int		skip;
	qboolean skip_dist_check;

	if (!s_soundStarted)
		return;

	if (!sfx)
		return;

	if (nosound->value)
		return;

	vol = fvol*255;

// pick a channel to play on
	target_chan = SND_PickChannel(entnum, entchannel);
	if (!target_chan)
		return;
		
// spatialize
	if(attenuation==4)
	{
		skip_dist_check=true;
		attenuation=1;
	}
	Com_Memset(target_chan, 0, sizeof(*target_chan));
	VectorCopy(origin, target_chan->origin);
	target_chan->dist_mult = attenuation / sound_nominal_clip_dist;
	target_chan->master_vol = vol;
	target_chan->entnum = entnum;
	target_chan->entchannel = entchannel;
	SND_Spatialize(target_chan);

	if(!skip_dist_check)//always play looping sounds
		if (!target_chan->leftvol && !target_chan->rightvol)
			return;		// not audible at all
	
// new channel
	if (!S_LoadSound (sfx))
	{
		target_chan->sfx = NULL;
		return;		// couldn't load the sound's data
	}

	target_chan->sfx = sfx;
    target_chan->startSample = s_paintedtime;

// if an identical sound has also been started this frame, offset the pos
// a bit to keep it from just making the first one louder
	check = s_channels;
    for (ch_idx=0; ch_idx < MAX_CHANNELS ; ch_idx++, check++)
    {
		if (check == target_chan)
			continue;
		if (check->sfx == sfx && check->startSample == s_paintedtime)
		{
			skip = rand () % (int)(0.1*dma.speed);
			if (skip >= target_chan->startSample + target_chan->sfx->Length)
				skip = target_chan->startSample + target_chan->sfx->Length - 1;
			target_chan->startSample -= skip;
			break;
		}
		
	}
}

void S_StopSound(int entnum, int entchannel)
{
	int i;

	for (i=0; i<MAX_CHANNELS ; i++)
	{
		if (s_channels[i].entnum == entnum
			&& ((!entchannel)||s_channels[i].entchannel == entchannel))	// 0 matches any
		{
			s_channels[i].startSample = 0;
			s_channels[i].sfx = NULL;
			if (entchannel)
				return;	//got a match, not looking for more.
		}
	}
}

void S_UpdateSoundPos (int entnum, int entchannel, vec3_t origin)
{
	int i;

	for (i=0; i<MAX_CHANNELS ; i++)
	{
		if (s_channels[i].entnum == entnum
			&& s_channels[i].entchannel == entchannel)
		{
			VectorCopy(origin, s_channels[i].origin);
			return;
		}
	}
}

void S_StopAllSounds(qboolean clear)
{
	int		i;

	if (!s_soundStarted)
		return;

	s_pendingplays.next = s_pendingplays.prev = &s_pendingplays;

	numLoopChannels = BSP29_NUM_AMBIENTS;	// no statics

	Com_Memset(s_channels, 0, MAX_CHANNELS * sizeof(channel_t));
	Com_Memset(loop_channels, 0, MAX_CHANNELS * sizeof(channel_t));

	if (clear)
		S_ClearSoundBuffer ();
}

void S_StopAllSoundsC (void)
{
	S_StopAllSounds (true);
}

void S_ClearSoundBuffer (void)
{
	int		clear;
		
	if (!s_soundStarted)
		return;

	if (dma.samplebits == 8)
		clear = 0x80;
	else
		clear = 0;

	SNDDMA_BeginPainting();
	if (dma.buffer)
		Com_Memset(dma.buffer, clear, dma.samples * dma.samplebits/8);
	SNDDMA_Submit();
}


/*
=================
S_StaticSound
=================
*/
void S_StaticSound (sfx_t *sfx, vec3_t origin, float vol, float attenuation)
{
	channel_t	*ss;

	if (!sfx)
		return;

	if (numLoopChannels == MAX_CHANNELS)
	{
		Con_Printf ("StaticSound: MAX_CHANNELS reached\n");
		Con_Printf (" failed at (%.2f, %.2f, %.2f)\n",origin[0],origin[1],origin[2]);
		return;
	}

	ss = &loop_channels[numLoopChannels];
	numLoopChannels++;

	if (!S_LoadSound (sfx))
		return;

	if (sfx->LoopStart == -1)
	{
		Con_Printf ("Sound %s not looped\n", sfx->Name);
		return;
	}
	
	ss->sfx = sfx;
	VectorCopy (origin, ss->origin);
	ss->master_vol = vol;
	ss->dist_mult = (attenuation/64) / sound_nominal_clip_dist;
    ss->startSample = s_paintedtime;
	
	SND_Spatialize (ss);
}


//=============================================================================

/*
===================
S_UpdateAmbientSounds
===================
*/
void S_UpdateAmbientSounds (void)
{
	float		vol;
	int			ambient_channel;
	channel_t	*chan;

	if (!snd_ambient)
		return;

	// calc ambient sound levels
	byte* ambient_sound_level = CM_LeafAmbientSoundLevel(CM_PointLeafnum(listener_origin));
	if (!ambient_sound_level || !ambient_level->value)
	{
		for (ambient_channel = 0 ; ambient_channel< BSP29_NUM_AMBIENTS ; ambient_channel++)
			loop_channels[ambient_channel].sfx = NULL;
		return;
	}

	for (ambient_channel = 0 ; ambient_channel< BSP29_NUM_AMBIENTS ; ambient_channel++)
	{
		chan = &loop_channels[ambient_channel];	
		chan->sfx = ambient_sfx[ambient_channel];

		vol = ambient_level->value * ambient_sound_level[ambient_channel];
		if (vol < 8)
			vol = 0;

	// don't adjust volume too fast
		if (chan->master_vol < vol)
		{
			chan->master_vol += host_frametime * ambient_fade->value;
			if (chan->master_vol > vol)
				chan->master_vol = vol;
		}
		else if (chan->master_vol > vol)
		{
			chan->master_vol -= host_frametime * ambient_fade->value;
			if (chan->master_vol < vol)
				chan->master_vol = vol;
		}
		
		chan->leftvol = chan->rightvol = chan->master_vol;
	}
}


/*
============
S_Update

Called once each time through the main loop
============
*/
void S_Update(vec3_t origin, vec3_t forward, vec3_t right, vec3_t up)
{
	int			i, j;
	int			total;
	channel_t	*ch;
	channel_t	*combine;

	if (!s_soundStarted || s_soundMuted)
		return;

	VectorCopy(origin, listener_origin);
	VectorCopy(forward, listener_axis[0]);
	VectorSubtract(vec3_origin, right, listener_axis[1]);
	VectorCopy(up, listener_axis[2]);
	
// update general area ambient sound sources
	S_UpdateAmbientSounds ();

	combine = NULL;

// update spatialization for static and dynamic sounds	
	ch = s_channels;
	for (i=0; i<MAX_CHANNELS; i++, ch++)
	{
		if (!ch->sfx)
			continue;
		SND_Spatialize(ch);         // respatialize channel
	}

// update spatialization for static and dynamic sounds	
	ch = loop_channels + BSP29_NUM_AMBIENTS;
	for (i=BSP29_NUM_AMBIENTS; i<numLoopChannels; i++, ch++)
	{
		if (!ch->sfx)
			continue;
		SND_Spatialize(ch);         // respatialize channel
		if (!ch->leftvol && !ch->rightvol)
			continue;

	// try to combine static sounds with a previous channel of the same
	// sound effect so we don't mix five torches every frame
	
	// see if it can just use the last one
		if (combine && combine->sfx == ch->sfx)
		{
			combine->leftvol += ch->leftvol;
			combine->rightvol += ch->rightvol;
			ch->leftvol = ch->rightvol = 0;
			continue;
		}
	// search for one
		combine = loop_channels;
		for (j=BSP29_NUM_AMBIENTS; j<i; j++, combine++)
			if (combine->sfx == ch->sfx)
				break;
				
		if (j == numLoopChannels)
		{
			combine = NULL;
		}
		else
		{
			if (combine != ch)
			{
				combine->leftvol += ch->leftvol;
				combine->rightvol += ch->rightvol;
				ch->leftvol = ch->rightvol = 0;
			}
			continue;
		}
	}

//
// debugging output
//
	if (s_show->value)
	{
		total = 0;
		ch = s_channels;
		for (i=0 ; i<MAX_CHANNELS; i++, ch++)
			if (ch->sfx && (ch->leftvol || ch->rightvol) )
			{
				Con_Printf ("%s %3i %3i\n", ch->sfx->Name, ch->leftvol, ch->rightvol);
				total++;
			}
		ch = loop_channels;
		for (i=0 ; i<numLoopChannels; i++, ch++)
			if (ch->sfx && (ch->leftvol || ch->rightvol) )
			{
				Con_Printf ("%s %3i %3i\n", ch->sfx->Name, ch->leftvol, ch->rightvol);
				total++;
			}
		
		Con_Printf ("----(%i)----\n", total);
	}

// mix some sound
	S_Update_();
}

void GetSoundtime(void)
{
	int		samplepos;
	static	int		buffers;
	static	int		oldsamplepos;
	int		fullsamples;
	
	fullsamples = dma.samples / dma.channels;

// it is possible to miscount buffers if it has wrapped twice between
// calls to S_Update.  Oh well.
	samplepos = SNDDMA_GetDMAPos();

	if (samplepos < oldsamplepos)
	{
		buffers++;					// buffer wrapped
		
		if (s_paintedtime > 0x40000000)
		{	// time to chop things off to avoid 32 bit limits
			buffers = 0;
			s_paintedtime = fullsamples;
			S_StopAllSounds (true);
		}
	}
	oldsamplepos = samplepos;

	s_soundtime = buffers*fullsamples + samplepos/dma.channels;
}

void S_ExtraUpdate (void)
{
	if (snd_noextraupdate->value)
		return;		// don't pollute timings
	S_Update_();
}



void S_Update_(void)
{
	unsigned        endtime;
	int				samps;
	
	if (!s_soundStarted || s_soundMuted)
		return;

// Updates DMA time
	GetSoundtime();

// check to make sure that we haven't overshot
	if (s_paintedtime < s_soundtime)
	{
		//Con_Printf ("S_Update_ : overflow\n");
		s_paintedtime = s_soundtime;
	}

// mix ahead of current position
	endtime = s_soundtime + s_mixahead->value * dma.speed;
	samps = dma.samples >> (dma.channels-1);
	if (endtime - s_soundtime > samps)
		endtime = s_soundtime + samps;

	SNDDMA_BeginPainting();

	S_PaintChannels (endtime);

	SNDDMA_Submit ();
}

/*
===============================================================================

console functions

===============================================================================
*/

void S_Play(void)
{
	static int hash=345;
	int 	i;
	char name[256];
	sfx_t	*sfx;
	
	i = 1;
	while (i<Cmd_Argc())
	{
		if (!QStr::RChr(Cmd_Argv(i), '.'))
		{
			QStr::Cpy(name, Cmd_Argv(i));
			QStr::Cat(name, sizeof(name), ".wav");
		}
		else
			QStr::Cpy(name, Cmd_Argv(i));
		sfx = S_PrecacheSound(name);
		S_StartSound(hash++, 0, sfx, listener_origin, 1.0, 1.0);
		i++;
	}
}

void S_PlayVol(void)
{
	static int hash=543;
	int i;
	float vol;
	char name[256];
	sfx_t	*sfx;
	
	i = 1;
	while (i<Cmd_Argc())
	{
		if (!QStr::RChr(Cmd_Argv(i), '.'))
		{
			QStr::Cpy(name, Cmd_Argv(i));
			QStr::Cat(name, sizeof(name), ".wav");
		}
		else
			QStr::Cpy(name, Cmd_Argv(i));
		sfx = S_PrecacheSound(name);
		vol = QStr::Atof(Cmd_Argv(i+1));
		S_StartSound(hash++, 0, sfx, listener_origin, vol, 1.0);
		i+=2;
	}
}

void S_SoundList(void)
{
	int		i;
	sfx_t	*sfx;
	int		size, total;

	total = 0;
	for (sfx=s_knownSfx, i=0 ; i<s_numSfx; i++, sfx++)
	{
		if (!sfx->Data)
			continue;
		size = sfx->Length * 2;
		total += size;
		if (sfx->LoopStart >= 0)
			Con_Printf ("L");
		else
			Con_Printf (" ");
		Con_Printf(" %6i : %s\n", size, sfx->Name);
	}
	Con_Printf ("Total resident: %i\n", total);
}


void S_LocalSound (char *sound)
{
	sfx_t	*sfx;

	if (nosound->value)
		return;
	if (!s_soundStarted)
		return;
		
	sfx = S_PrecacheSound (sound);
	if (!sfx)
	{
		Con_Printf ("S_LocalSound: can't cache %s\n", sound);
		return;
	}
	S_StartSound (cl.viewentity, -1, sfx, vec3_origin, 1, 1);
}


void S_ClearPrecache (void)
{
}


void S_BeginPrecaching (void)
{
}


void S_EndPrecaching (void)
{
}

void S_IssuePlaysound(playsound_t *ps)
{
}
