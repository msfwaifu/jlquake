/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).  

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// snd_local.h -- private sound definations


#include "../../wolfclient/client.h"
#include "../../client/sound/local.h"
#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "snd_public.h"

struct loopSound_t
{
	vec3_t origin;
	vec3_t velocity;
	sfx_t       *sfx;
	int mergeFrame;
	qboolean active;
	qboolean kill;
	qboolean doppler;
	float dopplerScale;
	float oldDopplerScale;
	int framenum;
	float range;            //----(SA)	added
	int vol;
	qboolean loudUnderWater;    // (SA) set if this sound should be played at full vol even when under water (under water loop sound for ex.)
	int startTime, startSample;         // ydnar: so looping sounds can be out of phase
};

//====================================================================

extern vec3_t listener_forward;
extern vec3_t listener_right;
extern vec3_t listener_up;

extern Cvar   *s_nosound;
extern Cvar   *s_show;
extern Cvar   *s_mixahead;

extern Cvar   *s_separation;

void S_memoryLoad( sfx_t *sfx );
portable_samplepair_t *S_GetRawSamplePointer();

// spatializes a channel
void S_Spatialize( channel_t *ch );
