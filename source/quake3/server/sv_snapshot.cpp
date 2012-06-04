/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
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

#include "server.h"


/*
=============================================================================

Delta encode a client frame onto the network channel

A normal server packet will look like:

4	sequence number (high bit set if an oversize fragment)
<optional reliable commands>
1	q3svc_snapshot
4	last client reliable command
4	serverTime
1	lastframe for delta compression
1	snapFlags
1	areaBytes
<areabytes>
<playerstate>
<packetentities>

=============================================================================
*/

/*
=============
SV_EmitPacketEntities

Writes a delta update of an q3entityState_t list to the message.
=============
*/
static void SV_EmitPacketEntities(q3clientSnapshot_t* from, q3clientSnapshot_t* to, QMsg* msg)
{
	q3entityState_t* oldent, * newent;
	int oldindex, newindex;
	int oldnum, newnum;
	int from_num_entities;

	// generate the delta update
	if (!from)
	{
		from_num_entities = 0;
	}
	else
	{
		from_num_entities = from->num_entities;
	}

	newent = NULL;
	oldent = NULL;
	newindex = 0;
	oldindex = 0;
	while (newindex < to->num_entities || oldindex < from_num_entities)
	{
		if (newindex >= to->num_entities)
		{
			newnum = 9999;
		}
		else
		{
			newent = &svs.q3_snapshotEntities[(to->first_entity + newindex) % svs.q3_numSnapshotEntities];
			newnum = newent->number;
		}

		if (oldindex >= from_num_entities)
		{
			oldnum = 9999;
		}
		else
		{
			oldent = &svs.q3_snapshotEntities[(from->first_entity + oldindex) % svs.q3_numSnapshotEntities];
			oldnum = oldent->number;
		}

		if (newnum == oldnum)
		{
			// delta update from old position
			// because the force parm is qfalse, this will not result
			// in any bytes being emited if the entity has not changed at all
			MSGQ3_WriteDeltaEntity(msg, oldent, newent, qfalse);
			oldindex++;
			newindex++;
			continue;
		}

		if (newnum < oldnum)
		{
			// this is a new entity, send it from the baseline
			MSGQ3_WriteDeltaEntity(msg, &sv.q3_svEntities[newnum].q3_baseline, newent, qtrue);
			newindex++;
			continue;
		}

		if (newnum > oldnum)
		{
			// the old entity isn't present in the new message
			MSGQ3_WriteDeltaEntity(msg, oldent, NULL, qtrue);
			oldindex++;
			continue;
		}
	}

	msg->WriteBits((MAX_GENTITIES_Q3 - 1), GENTITYNUM_BITS_Q3);	// end of packetentities
}



/*
==================
SV_WriteSnapshotToClient
==================
*/
static void SV_WriteSnapshotToClient(client_t* client, QMsg* msg)
{
	q3clientSnapshot_t* frame, * oldframe;
	int lastframe;
	int i;
	int snapFlags;

	// this is the snapshot we are creating
	frame = &client->q3_frames[client->netchan.outgoingSequence & PACKET_MASK_Q3];

	// try to use a previous frame as the source for delta compressing the snapshot
	if (client->q3_deltaMessage <= 0 || client->state != CS_ACTIVE)
	{
		// client is asking for a retransmit
		oldframe = NULL;
		lastframe = 0;
	}
	else if (client->netchan.outgoingSequence - client->q3_deltaMessage
			 >= (PACKET_BACKUP_Q3 - 3))
	{
		// client hasn't gotten a good message through in a long time
		Com_DPrintf("%s: Delta request from out of date packet.\n", client->name);
		oldframe = NULL;
		lastframe = 0;
	}
	else
	{
		// we have a valid snapshot to delta from
		oldframe = &client->q3_frames[client->q3_deltaMessage & PACKET_MASK_Q3];
		lastframe = client->netchan.outgoingSequence - client->q3_deltaMessage;

		// the snapshot's entities may still have rolled off the buffer, though
		if (oldframe->first_entity <= svs.q3_nextSnapshotEntities - svs.q3_numSnapshotEntities)
		{
			Com_DPrintf("%s: Delta request from out of date entities.\n", client->name);
			oldframe = NULL;
			lastframe = 0;
		}
	}

	msg->WriteByte(q3svc_snapshot);

	// NOTE, MRE: now sent at the start of every message from server to client
	// let the client know which reliable clientCommands we have received
	//msg->WriteLong(client->lastClientCommand );

	// send over the current server time so the client can drift
	// its view of time to try to match
	msg->WriteLong(svs.q3_time);

	// what we are delta'ing from
	msg->WriteByte(lastframe);

	snapFlags = svs.q3_snapFlagServerBit;
	if (client->q3_rateDelayed)
	{
		snapFlags |= SNAPFLAG_RATE_DELAYED;
	}
	if (client->state != CS_ACTIVE)
	{
		snapFlags |= SNAPFLAG_NOT_ACTIVE;
	}

	msg->WriteByte(snapFlags);

	// send over the areabits
	msg->WriteByte(frame->areabytes);
	msg->WriteData(frame->areabits, frame->areabytes);

	// delta encode the playerstate
	if (oldframe)
	{
		MSGQ3_WriteDeltaPlayerstate(msg, &oldframe->q3_ps, &frame->q3_ps);
	}
	else
	{
		MSGQ3_WriteDeltaPlayerstate(msg, NULL, &frame->q3_ps);
	}

	// delta encode the entities
	SV_EmitPacketEntities(oldframe, frame, msg);

	// padding for rate debugging
	if (sv_padPackets->integer)
	{
		for (i = 0; i < sv_padPackets->integer; i++)
		{
			msg->WriteByte(q3svc_nop);
		}
	}
}


/*
==================
SV_UpdateServerCommandsToClient

(re)send all server commands the client hasn't acknowledged yet
==================
*/
void SV_UpdateServerCommandsToClient(client_t* client, QMsg* msg)
{
	int i;

	// write any unacknowledged serverCommands
	for (i = client->q3_reliableAcknowledge + 1; i <= client->q3_reliableSequence; i++)
	{
		msg->WriteByte(q3svc_serverCommand);
		msg->WriteLong(i);
		msg->WriteString(client->q3_reliableCommands[i & (MAX_RELIABLE_COMMANDS_Q3 - 1)]);
	}
	client->q3_reliableSent = client->q3_reliableSequence;
}

/*
=============================================================================

Build a client snapshot structure

=============================================================================
*/

#define MAX_SNAPSHOT_ENTITIES   1024
typedef struct
{
	int numSnapshotEntities;
	int snapshotEntities[MAX_SNAPSHOT_ENTITIES];
} snapshotEntityNumbers_t;

/*
=======================
SV_QsortEntityNumbers
=======================
*/
static int SV_QsortEntityNumbers(const void* a, const void* b)
{
	int* ea, * eb;

	ea = (int*)a;
	eb = (int*)b;

	if (*ea == *eb)
	{
		Com_Error(ERR_DROP, "SV_QsortEntityStates: duplicated entity");
	}

	if (*ea < *eb)
	{
		return -1;
	}

	return 1;
}


/*
===============
SV_AddEntToSnapshot
===============
*/
static void SV_AddEntToSnapshot(q3svEntity_t* svEnt, q3sharedEntity_t* gEnt, snapshotEntityNumbers_t* eNums)
{
	// if we have already added this entity to this snapshot, don't add again
	if (svEnt->snapshotCounter == sv.q3_snapshotCounter)
	{
		return;
	}
	svEnt->snapshotCounter = sv.q3_snapshotCounter;

	// if we are full, silently discard entities
	if (eNums->numSnapshotEntities == MAX_SNAPSHOT_ENTITIES)
	{
		return;
	}

	eNums->snapshotEntities[eNums->numSnapshotEntities] = gEnt->s.number;
	eNums->numSnapshotEntities++;
}

/*
===============
SV_AddEntitiesVisibleFromPoint
===============
*/
static void SV_AddEntitiesVisibleFromPoint(vec3_t origin, q3clientSnapshot_t* frame,
	snapshotEntityNumbers_t* eNums, qboolean portal)
{
	int e, i;
	q3sharedEntity_t* ent;
	q3svEntity_t* svEnt;
	int l;
	int clientarea, clientcluster;
	int leafnum;
	int c_fullsend;
	byte* clientpvs;
	byte* bitvector;

	// during an error shutdown message we may need to transmit
	// the shutdown message after the server has shutdown, so
	// specfically check for it
	if (!sv.state)
	{
		return;
	}

	leafnum = CM_PointLeafnum(origin);
	clientarea = CM_LeafArea(leafnum);
	clientcluster = CM_LeafCluster(leafnum);

	// calculate the visible areas
	frame->areabytes = CM_WriteAreaBits(frame->areabits, clientarea);

	clientpvs = CM_ClusterPVS(clientcluster);

	c_fullsend = 0;

	for (e = 0; e < sv.q3_num_entities; e++)
	{
		ent = SVQ3_GentityNum(e);

		// never send entities that aren't linked in
		if (!ent->r.linked)
		{
			continue;
		}

		if (ent->s.number != e)
		{
			Com_DPrintf("FIXING ENT->S.NUMBER!!!\n");
			ent->s.number = e;
		}

		// entities can be flagged to explicitly not be sent to the client
		if (ent->r.svFlags & Q3SVF_NOCLIENT)
		{
			continue;
		}

		// entities can be flagged to be sent to only one client
		if (ent->r.svFlags & Q3SVF_SINGLECLIENT)
		{
			if (ent->r.singleClient != frame->q3_ps.clientNum)
			{
				continue;
			}
		}
		// entities can be flagged to be sent to everyone but one client
		if (ent->r.svFlags & Q3SVF_NOTSINGLECLIENT)
		{
			if (ent->r.singleClient == frame->q3_ps.clientNum)
			{
				continue;
			}
		}
		// entities can be flagged to be sent to a given mask of clients
		if (ent->r.svFlags & Q3SVF_CLIENTMASK)
		{
			if (frame->q3_ps.clientNum >= 32)
			{
				Com_Error(ERR_DROP, "Q3SVF_CLIENTMASK: cientNum > 32\n");
			}
			if (~ent->r.singleClient & (1 << frame->q3_ps.clientNum))
			{
				continue;
			}
		}

		svEnt = SVQ3_SvEntityForGentity(ent);

		// don't double add an entity through portals
		if (svEnt->snapshotCounter == sv.q3_snapshotCounter)
		{
			continue;
		}

		// broadcast entities are always sent
		if (ent->r.svFlags & Q3SVF_BROADCAST)
		{
			SV_AddEntToSnapshot(svEnt, ent, eNums);
			continue;
		}

		// ignore if not touching a PV leaf
		// check area
		if (!CM_AreasConnected(clientarea, svEnt->areanum))
		{
			// doors can legally straddle two areas, so
			// we may need to check another one
			if (!CM_AreasConnected(clientarea, svEnt->areanum2))
			{
				continue;		// blocked by a door
			}
		}

		bitvector = clientpvs;

		// check individual leafs
		if (!svEnt->numClusters)
		{
			continue;
		}
		l = 0;
		for (i = 0; i < svEnt->numClusters; i++)
		{
			l = svEnt->clusternums[i];
			if (bitvector[l >> 3] & (1 << (l & 7)))
			{
				break;
			}
		}

		// if we haven't found it to be visible,
		// check overflow clusters that coudln't be stored
		if (i == svEnt->numClusters)
		{
			if (svEnt->lastCluster)
			{
				for (; l <= svEnt->lastCluster; l++)
				{
					if (bitvector[l >> 3] & (1 << (l & 7)))
					{
						break;
					}
				}
				if (l == svEnt->lastCluster)
				{
					continue;	// not visible
				}
			}
			else
			{
				continue;
			}
		}

		// add it
		SV_AddEntToSnapshot(svEnt, ent, eNums);

		// if its a portal entity, add everything visible from its camera position
		if (ent->r.svFlags & Q3SVF_PORTAL)
		{
			if (ent->s.generic1)
			{
				vec3_t dir;
				VectorSubtract(ent->s.origin, origin, dir);
				if (VectorLengthSquared(dir) > (float)ent->s.generic1 * ent->s.generic1)
				{
					continue;
				}
			}
			SV_AddEntitiesVisibleFromPoint(ent->s.origin2, frame, eNums, qtrue);
		}

	}
}

/*
=============
SV_BuildClientSnapshot

Decides which entities are going to be visible to the client, and
copies off the playerstate and areabits.

This properly handles multiple recursive portals, but the render
currently doesn't.

For viewing through other player's eyes, clent can be something other than client->gentity
=============
*/
static void SV_BuildClientSnapshot(client_t* client)
{
	vec3_t org;
	q3clientSnapshot_t* frame;
	snapshotEntityNumbers_t entityNumbers;
	int i;
	q3sharedEntity_t* ent;
	q3entityState_t* state;
	q3svEntity_t* svEnt;
	q3sharedEntity_t* clent;
	int clientNum;
	q3playerState_t* ps;

	// bump the counter used to prevent double adding
	sv.q3_snapshotCounter++;

	// this is the frame we are creating
	frame = &client->q3_frames[client->netchan.outgoingSequence & PACKET_MASK_Q3];

	// clear everything in this snapshot
	entityNumbers.numSnapshotEntities = 0;
	Com_Memset(frame->areabits, 0, sizeof(frame->areabits));

	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=62
	frame->num_entities = 0;

	clent = client->q3_gentity;
	if (!clent || client->state == CS_ZOMBIE)
	{
		return;
	}

	// grab the current q3playerState_t
	ps = SVQ3_GameClientNum(client - svs.clients);
	frame->q3_ps = *ps;

	// never send client's own entity, because it can
	// be regenerated from the playerstate
	clientNum = frame->q3_ps.clientNum;
	if (clientNum < 0 || clientNum >= MAX_GENTITIES_Q3)
	{
		Com_Error(ERR_DROP, "SVQ3_SvEntityForGentity: bad gEnt");
	}
	svEnt = &sv.q3_svEntities[clientNum];

	svEnt->snapshotCounter = sv.q3_snapshotCounter;

	// find the client's viewpoint
	VectorCopy(ps->origin, org);
	org[2] += ps->viewheight;

	// add all the entities directly visible to the eye, which
	// may include portal entities that merge other viewpoints
	SV_AddEntitiesVisibleFromPoint(org, frame, &entityNumbers, qfalse);

	// if there were portals visible, there may be out of order entities
	// in the list which will need to be resorted for the delta compression
	// to work correctly.  This also catches the error condition
	// of an entity being included twice.
	qsort(entityNumbers.snapshotEntities, entityNumbers.numSnapshotEntities,
		sizeof(entityNumbers.snapshotEntities[0]), SV_QsortEntityNumbers);

	// now that all viewpoint's areabits have been OR'd together, invert
	// all of them to make it a mask vector, which is what the renderer wants
	for (i = 0; i < MAX_MAP_AREA_BYTES / 4; i++)
	{
		((int*)frame->areabits)[i] = ((int*)frame->areabits)[i] ^ -1;
	}

	// copy the entity states out
	frame->num_entities = 0;
	frame->first_entity = svs.q3_nextSnapshotEntities;
	for (i = 0; i < entityNumbers.numSnapshotEntities; i++)
	{
		ent = SVQ3_GentityNum(entityNumbers.snapshotEntities[i]);
		state = &svs.q3_snapshotEntities[svs.q3_nextSnapshotEntities % svs.q3_numSnapshotEntities];
		*state = ent->s;
		svs.q3_nextSnapshotEntities++;
		// this should never hit, map should always be restarted first in SV_Frame
		if (svs.q3_nextSnapshotEntities >= 0x7FFFFFFE)
		{
			Com_Error(ERR_FATAL, "svs.q3_nextSnapshotEntities wrapped");
		}
		frame->num_entities++;
	}
}


/*
====================
SV_RateMsec

Return the number of msec a given size message is supposed
to take to clear, based on the current rate
====================
*/
#define HEADER_RATE_BYTES   48		// include our header, IP header, and some overhead
static int SV_RateMsec(client_t* client, int messageSize)
{
	int rate;
	int rateMsec;

	// individual messages will never be larger than fragment size
	if (messageSize > 1500)
	{
		messageSize = 1500;
	}
	rate = client->rate;
	if (sv_maxRate->integer)
	{
		if (sv_maxRate->integer < 1000)
		{
			Cvar_Set("sv_MaxRate", "1000");
		}
		if (sv_maxRate->integer < rate)
		{
			rate = sv_maxRate->integer;
		}
	}
	rateMsec = (messageSize + HEADER_RATE_BYTES) * 1000 / rate;

	return rateMsec;
}

/*
=======================
SV_SendMessageToClient

Called by SV_SendClientSnapshot and SV_SendClientGameState
=======================
*/
void SV_SendMessageToClient(QMsg* msg, client_t* client)
{
	int rateMsec;

	// record information about the message
	client->q3_frames[client->netchan.outgoingSequence & PACKET_MASK_Q3].messageSize = msg->cursize;
	client->q3_frames[client->netchan.outgoingSequence & PACKET_MASK_Q3].messageSent = svs.q3_time;
	client->q3_frames[client->netchan.outgoingSequence & PACKET_MASK_Q3].messageAcked = -1;

	// send the datagram
	SV_Netchan_Transmit(client, msg);	//msg->cursize, msg->data );

	// set nextSnapshotTime based on rate and requested number of updates

	// local clients get snapshots every frame
	// TTimo - https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=491
	// added sv_lanForceRate check
	if (client->netchan.remoteAddress.type == NA_LOOPBACK || (sv_lanForceRate->integer && SOCK_IsLANAddress(client->netchan.remoteAddress)))
	{
		client->q3_nextSnapshotTime = svs.q3_time - 1;
		return;
	}

	// normal rate / snapshotMsec calculation
	rateMsec = SV_RateMsec(client, msg->cursize);

	if (rateMsec < client->q3_snapshotMsec)
	{
		// never send more packets than this, no matter what the rate is at
		rateMsec = client->q3_snapshotMsec;
		client->q3_rateDelayed = qfalse;
	}
	else
	{
		client->q3_rateDelayed = qtrue;
	}

	client->q3_nextSnapshotTime = svs.q3_time + rateMsec;

	// don't pile up empty snapshots while connecting
	if (client->state != CS_ACTIVE)
	{
		// a gigantic connection message may have already put the nextSnapshotTime
		// more than a second away, so don't shorten it
		// do shorten if client is downloading
		if (!*client->downloadName && client->q3_nextSnapshotTime < svs.q3_time + 1000)
		{
			client->q3_nextSnapshotTime = svs.q3_time + 1000;
		}
	}
}


/*
=======================
SV_SendClientSnapshot

Also called by SV_FinalMessage

=======================
*/
void SV_SendClientSnapshot(client_t* client)
{
	byte msg_buf[MAX_MSGLEN_Q3];
	QMsg msg;

	// build the snapshot
	SV_BuildClientSnapshot(client);

	// bots need to have their snapshots build, but
	// the query them directly without needing to be sent
	if (client->q3_gentity && client->q3_gentity->r.svFlags & Q3SVF_BOT)
	{
		return;
	}

	msg.Init(msg_buf, sizeof(msg_buf));

	// NOTE, MRE: all server->client messages now acknowledge
	// let the client know which reliable clientCommands we have received
	msg.WriteLong(client->q3_lastClientCommand);

	// (re)send any reliable server commands
	SV_UpdateServerCommandsToClient(client, &msg);

	// send over all the relevant q3entityState_t
	// and the q3playerState_t
	SV_WriteSnapshotToClient(client, &msg);

	// Add any download data if the client is downloading
	SV_WriteDownloadToClient(client, &msg);

	// check for overflow
	if (msg.overflowed)
	{
		Com_Printf("WARNING: msg overflowed for %s\n", client->name);
		msg.Clear();
	}

	SV_SendMessageToClient(&msg, client);
}


/*
=======================
SV_SendClientMessages
=======================
*/
void SV_SendClientMessages(void)
{
	int i;
	client_t* c;

	// send a message to each connected client
	for (i = 0, c = svs.clients; i < sv_maxclients->integer; i++, c++)
	{
		if (!c->state)
		{
			continue;		// not connected
		}

		if (svs.q3_time < c->q3_nextSnapshotTime)
		{
			continue;		// not time yet
		}

		// send additional message fragments if the last message
		// was too large to send at once
		if (c->netchan.unsentFragments)
		{
			c->q3_nextSnapshotTime = svs.q3_time +
								  SV_RateMsec(c, c->netchan.reliableOrUnsentLength - c->netchan.unsentFragmentStart);
			SV_Netchan_TransmitNextFragment(c);
			continue;
		}

		// generate and send a new message
		SV_SendClientSnapshot(c);
	}
}
