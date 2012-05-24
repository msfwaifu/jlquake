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

#include "../server.h"
#include "local.h"

/*

  area routing cache:
  stores the distances within one cluster to a specific goal area
  this goal area is in this same cluster and could be a cluster portal
  for every cluster there's a list with routing cache for every area
  in that cluster (including the portals of that cluster)
  area cache stores aasworld->clusters[?].numreachabilityareas travel times

  portal routing cache:
  stores the distances of all portals to a specific goal area
  this goal area could be in any cluster and could also be a cluster portal
  for every area (aasworld->numareas) the portal cache stores
  aasworld->numportals travel times

*/

#ifdef ROUTING_DEBUG
int numareacacheupdates;
int numportalcacheupdates;
#endif	//ROUTING_DEBUG

int routingcachesize;
int max_routingcachesize;
int max_frameroutingupdates;

void AAS_RoutingInfo()
{
#ifdef ROUTING_DEBUG
	BotImport_Print(PRT_MESSAGE, "%d area cache updates\n", numareacacheupdates);
	BotImport_Print(PRT_MESSAGE, "%d portal cache updates\n", numportalcacheupdates);
	BotImport_Print(PRT_MESSAGE, "%d bytes routing cache\n", routingcachesize);
#endif
}

// returns the number of the area in the cluster
// assumes the given area is in the given cluster or a portal of the cluster
/*inline*/ int AAS_ClusterAreaNum(int cluster, int areanum)
{
	int areacluster = aasworld->areasettings[areanum].cluster;
	if (areacluster > 0)
	{
		return aasworld->areasettings[areanum].clusterareanum;
	}
	else
	{
		int side = aasworld->portals[-areacluster].frontcluster != cluster;
		return aasworld->portals[-areacluster].clusterareanum[side];
	}
}

void AAS_InitTravelFlagFromType()
{
	for (int i = 0; i < MAX_TRAVELTYPES; i++)
	{
		aasworld->travelflagfortype[i] = TFL_INVALID;
	}
	aasworld->travelflagfortype[TRAVEL_INVALID] = TFL_INVALID;
	aasworld->travelflagfortype[TRAVEL_WALK] = TFL_WALK;
	aasworld->travelflagfortype[TRAVEL_CROUCH] = TFL_CROUCH;
	aasworld->travelflagfortype[TRAVEL_BARRIERJUMP] = TFL_BARRIERJUMP;
	aasworld->travelflagfortype[TRAVEL_JUMP] = TFL_JUMP;
	aasworld->travelflagfortype[TRAVEL_LADDER] = TFL_LADDER;
	aasworld->travelflagfortype[TRAVEL_WALKOFFLEDGE] = TFL_WALKOFFLEDGE;
	aasworld->travelflagfortype[TRAVEL_SWIM] = TFL_SWIM;
	aasworld->travelflagfortype[TRAVEL_WATERJUMP] = TFL_WATERJUMP;
	aasworld->travelflagfortype[TRAVEL_TELEPORT] = TFL_TELEPORT;
	aasworld->travelflagfortype[TRAVEL_ELEVATOR] = TFL_ELEVATOR;
	aasworld->travelflagfortype[TRAVEL_ROCKETJUMP] = TFL_ROCKETJUMP;
	aasworld->travelflagfortype[TRAVEL_BFGJUMP] = TFL_BFGJUMP;
	aasworld->travelflagfortype[TRAVEL_GRAPPLEHOOK] = TFL_GRAPPLEHOOK;
	aasworld->travelflagfortype[TRAVEL_DOUBLEJUMP] = TFL_DOUBLEJUMP;
	aasworld->travelflagfortype[TRAVEL_RAMPJUMP] = TFL_RAMPJUMP;
	aasworld->travelflagfortype[TRAVEL_STRAFEJUMP] = TFL_STRAFEJUMP;
	aasworld->travelflagfortype[TRAVEL_JUMPPAD] = TFL_JUMPPAD;
	aasworld->travelflagfortype[TRAVEL_FUNCBOB] = TFL_FUNCBOB;
}

int AAS_TravelFlagForType(int traveltype)
{
	traveltype &= TRAVELTYPE_MASK;
	if (traveltype < 0 || traveltype >= MAX_TRAVELTYPES)
	{
		return TFL_INVALID;
	}
	return aasworld->travelflagfortype[traveltype];
}

/*__inline*/ float AAS_RoutingTime()
{
	return AAS_Time();
}

void AAS_LinkCache(aas_routingcache_t* cache)
{
	if (aasworld->newestcache)
	{
		aasworld->newestcache->time_next = cache;
		cache->time_prev = aasworld->newestcache;
	}
	else
	{
		aasworld->oldestcache = cache;
		cache->time_prev = NULL;
	}
	cache->time_next = NULL;
	aasworld->newestcache = cache;
}

void AAS_UnlinkCache(aas_routingcache_t* cache)
{
	if (cache->time_next)
	{
		cache->time_next->time_prev = cache->time_prev;
	}
	else
	{
		aasworld->newestcache = cache->time_prev;
	}
	if (cache->time_prev)
	{
		cache->time_prev->time_next = cache->time_next;
	}
	else
	{
		aasworld->oldestcache = cache->time_next;
	}
	cache->time_next = NULL;
	cache->time_prev = NULL;
}

aas_routingcache_t* AAS_AllocRoutingCache(int numtraveltimes)
{
	int size = sizeof(aas_routingcache_t) +
		numtraveltimes * sizeof(unsigned short int) +
		numtraveltimes * sizeof(unsigned char);

	routingcachesize += size;

	aas_routingcache_t* cache = (aas_routingcache_t*)Mem_ClearedAlloc(size);
	cache->reachabilities = (unsigned char*)cache + sizeof(aas_routingcache_t) +
		numtraveltimes * sizeof(unsigned short int);
	cache->size = size;
	return cache;
}

void AAS_FreeRoutingCache(aas_routingcache_t* cache)
{
	AAS_UnlinkCache(cache);
	routingcachesize -= cache->size;
	Mem_Free(cache);
}

static void AAS_RemoveRoutingCacheInCluster(int clusternum)
{
	int i;
	aas_routingcache_t* cache, * nextcache;
	aas_cluster_t* cluster;

	if (!aasworld->clusterareacache)
	{
		return;
	}
	cluster = &aasworld->clusters[clusternum];
	for (i = 0; i < cluster->numareas; i++)
	{
		for (cache = aasworld->clusterareacache[clusternum][i]; cache; cache = nextcache)
		{
			nextcache = cache->next;
			AAS_FreeRoutingCache(cache);
		}
		aasworld->clusterareacache[clusternum][i] = NULL;
	}
}

void AAS_RemoveRoutingCacheUsingArea(int areanum)
{
	int clusternum = aasworld->areasettings[areanum].cluster;
	if (clusternum > 0)
	{
		//remove all the cache in the cluster the area is in
		AAS_RemoveRoutingCacheInCluster(clusternum);
	}
	else
	{
		// if this is a portal remove all cache in both the front and back cluster
		AAS_RemoveRoutingCacheInCluster(aasworld->portals[-clusternum].frontcluster);
		AAS_RemoveRoutingCacheInCluster(aasworld->portals[-clusternum].backcluster);
	}
	// remove all portal cache
	for (int i = 0; i < aasworld->numareas; i++)
	{
		//refresh portal cache
		aas_routingcache_t* nextcache;
		for (aas_routingcache_t* cache = aasworld->portalcache[i]; cache; cache = nextcache)
		{
			nextcache = cache->next;
			AAS_FreeRoutingCache(cache);
		}
		aasworld->portalcache[i] = NULL;
	}
}

void AAS_ClearClusterTeamFlags(int areanum)
{
	int clusternum = aasworld->areasettings[areanum].cluster;
	if (clusternum > 0)
	{
		aasworld->clusterTeamTravelFlags[clusternum] = -1;	// recalculate
	}
}

int AAS_EnableRoutingArea(int areanum, int enable)
{
	if (areanum <= 0 || areanum >= aasworld->numareas)
	{
		if (bot_developer)
		{
			BotImport_Print(PRT_ERROR, "AAS_EnableRoutingArea: areanum %d out of range\n", areanum);
		}
		return 0;
	}

	int bitflag;	// flag to set or clear
	if (GGameType & GAME_ET)
	{
		if ((enable & 1) || (enable < 0))
		{
			// clear all flags
			bitflag = ETAREA_AVOID | AREA_DISABLED | ETAREA_TEAM_AXIS | ETAREA_TEAM_ALLIES | ETAREA_TEAM_AXIS_DISGUISED | ETAREA_TEAM_ALLIES_DISGUISED;
		}
		else if (enable & 0x10)
		{
			bitflag = ETAREA_AVOID;
		}
		else if (enable & 0x20)
		{
			bitflag = ETAREA_TEAM_AXIS;
		}
		else if (enable & 0x40)
		{
			bitflag = ETAREA_TEAM_ALLIES;
		}
		else if (enable & 0x80)
		{
			bitflag = ETAREA_TEAM_AXIS_DISGUISED;
		}
		else if (enable & 0x100)
		{
			bitflag = ETAREA_TEAM_ALLIES_DISGUISED;
		}
		else
		{
			bitflag = AREA_DISABLED;
		}

		// remove avoidance flag
		enable &= 1;
	}
	else
	{
		bitflag = AREA_DISABLED;
	}

	int flags = aasworld->areasettings[areanum].areaflags & bitflag;
	if (enable < 0)
	{
		return !flags;
	}

	if (enable)
	{
		aasworld->areasettings[areanum].areaflags &= ~bitflag;
	}
	else
	{
		aasworld->areasettings[areanum].areaflags |= bitflag;
	}

	// if the status of the area changed
	if ((flags & bitflag) != (aasworld->areasettings[areanum].areaflags & bitflag))
	{
		//remove all routing cache involving this area
		AAS_RemoveRoutingCacheUsingArea(areanum);
		if (GGameType & GAME_ET)
		{
			// recalculate the team flags that are used in this cluster
			AAS_ClearClusterTeamFlags(areanum);
		}
	}
	return !flags;
}

void AAS_EnableAllAreas()
{
	for (int i = 0; i < aasworld->numareas; i++)
	{
		if (aasworld->areasettings[i].areaflags & AREA_DISABLED)
		{
			AAS_EnableRoutingArea(i, true);
		}
	}
}

static int AAS_GetAreaContentsTravelFlags(int areanum)
{
	int contents = aasworld->areasettings[areanum].contents;
	int tfl = 0;
	if (contents & AREACONTENTS_WATER)
	{
		tfl |= TFL_WATER;
	}
	else if (contents & AREACONTENTS_SLIME)
	{
		tfl |= TFL_SLIME;
	}
	else if (contents & AREACONTENTS_LAVA)
	{
		tfl |= TFL_LAVA;
	}
	else
	{
		tfl |= TFL_AIR;
	}
	if (contents & AREACONTENTS_DONOTENTER)
	{
		tfl |= TFL_DONOTENTER;
	}
	if (contents & Q3AREACONTENTS_NOTTEAM1)
	{
		tfl |= Q3TFL_NOTTEAM1;
	}
	if (contents & Q3AREACONTENTS_NOTTEAM2)
	{
		tfl |= Q3TFL_NOTTEAM2;
	}
	if (aasworld->areasettings[areanum].areaflags & Q3AREA_BRIDGE)
	{
		tfl |= Q3TFL_BRIDGE;
	}
	return tfl;
}

void AAS_InitAreaContentsTravelFlags()
{
	if (aasworld->areacontentstravelflags)
	{
		Mem_Free(aasworld->areacontentstravelflags);
	}
	aasworld->areacontentstravelflags = (int*)Mem_ClearedAlloc(aasworld->numareas * sizeof(int));

	for (int i = 0; i < aasworld->numareas; i++)
	{
		aasworld->areacontentstravelflags[i] = AAS_GetAreaContentsTravelFlags(i);
	}
}

int AAS_AreaContentsTravelFlags(int areanum)
{
	return aasworld->areacontentstravelflags[areanum];
}