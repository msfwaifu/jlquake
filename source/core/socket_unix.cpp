//**************************************************************************
//**
//**	$Id$
//**
//**	Copyright (C) 1996-2005 Id Software, Inc.
//**	Copyright (C) 2010 Jānis Legzdiņš
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "core.h"
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/ioctl.h>

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	NetadrToSockadr
//
//==========================================================================

void NetadrToSockadr(netadr_t* a, sockaddr_in* s)
{
	Com_Memset(s, 0, sizeof(*s));

	if (a->type == NA_BROADCAST)
	{
		s->sin_family = AF_INET;

		s->sin_port = a->port;
		*(int*)&s->sin_addr = -1;
	}
	else if (a->type == NA_IP)
	{
		s->sin_family = AF_INET;

		*(int*)&s->sin_addr = *(int*)&a->ip;
		s->sin_port = a->port;
	}
	else
	{
		throw QException("Invalid address type");
	}
}

//==========================================================================
//
//	SockadrToNetadr
//
//==========================================================================

void SockadrToNetadr(sockaddr_in* s, netadr_t* a)
{
	*(int*)&a->ip = *(int*)&s->sin_addr;
	a->port = s->sin_port;
	a->type = NA_IP;
}

//==========================================================================
//
//	SOCK_ErrorString
//
//==========================================================================

const char* SOCK_ErrorString()
{
	int code = errno;
	return strerror(code);
}

//==========================================================================
//
//	SOCK_Open
//
//==========================================================================

int SOCK_Open(const char* net_interface, int port)
{
	int newsocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (newsocket == -1)
	{
		GLog.Write("ERROR: UDP_OpenSocket: socket: %s\n", SOCK_ErrorString());
		return 0;
	}

	// make it non-blocking
	int _true = 1;
	if (ioctl(newsocket, FIONBIO, &_true) == -1)
	{
		GLog.Write("ERROR: UDP_OpenSocket: ioctl FIONBIO:%s\n", SOCK_ErrorString());
		close(newsocket);
		return 0;
	}

	// make it broadcast capable
	int i = 1;
	if (setsockopt(newsocket, SOL_SOCKET, SO_BROADCAST, (char*)&i, sizeof(i)) == -1)
	{
		GLog.Write("ERROR: UDP_OpenSocket: setsockopt SO_BROADCAST:%s\n", SOCK_ErrorString());
		return 0;
	}
	return newsocket;
}
