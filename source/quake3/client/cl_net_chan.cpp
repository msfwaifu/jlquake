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

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "client.h"

/*
==============
CL_Netchan_Encode

    // first 12 bytes of the data are always:
    long serverId;
    long messageAcknowledge;
    long reliableAcknowledge;

==============
*/
static void CL_Netchan_Encode(QMsg* msg)
{
	int serverId, messageAcknowledge, reliableAcknowledge;
	int i, index, srdc, sbit, soob;
	byte key, * string;

	if (msg->cursize <= Q3CL_ENCODE_START)
	{
		return;
	}

	srdc = msg->readcount;
	sbit = msg->bit;
	soob = msg->oob;

	msg->bit = 0;
	msg->readcount = 0;
	msg->oob = 0;

	serverId = msg->ReadLong();
	messageAcknowledge = msg->ReadLong();
	reliableAcknowledge = msg->ReadLong();

	msg->oob = soob;
	msg->bit = sbit;
	msg->readcount = srdc;

	string = (byte*)clc.q3_serverCommands[reliableAcknowledge & (MAX_RELIABLE_COMMANDS_Q3 - 1)];
	index = 0;
	//
	key = clc.q3_challenge ^ serverId ^ messageAcknowledge;
	for (i = Q3CL_ENCODE_START; i < msg->cursize; i++)
	{
		// modify the key with the last received now acknowledged server command
		if (!string[index])
		{
			index = 0;
		}
		if (string[index] > 127 || string[index] == '%')
		{
			key ^= '.' << (i & 1);
		}
		else
		{
			key ^= string[index] << (i & 1);
		}
		index++;
		// encode the data with this key
		*(msg->_data + i) = (*(msg->_data + i)) ^ key;
	}
}

/*
==============
CL_Netchan_Decode

    // first four bytes of the data are always:
    long reliableAcknowledge;

==============
*/
static void CL_Netchan_Decode(QMsg* msg)
{
	long reliableAcknowledge, i, index;
	byte key, * string;
	int srdc, sbit, soob;

	srdc = msg->readcount;
	sbit = msg->bit;
	soob = msg->oob;

	msg->oob = 0;

	reliableAcknowledge = msg->ReadLong();

	msg->oob = soob;
	msg->bit = sbit;
	msg->readcount = srdc;

	string = (byte*)clc.q3_reliableCommands[reliableAcknowledge & (MAX_RELIABLE_COMMANDS_Q3 - 1)];
	index = 0;
	// xor the client challenge with the netchan sequence number (need something that changes every message)
	key = clc.q3_challenge ^ LittleLong(*(unsigned*)msg->_data);
	for (i = msg->readcount + Q3CL_DECODE_START; i < msg->cursize; i++)
	{
		// modify the key with the last sent and with this message acknowledged client command
		if (!string[index])
		{
			index = 0;
		}
		if (string[index] > 127 || string[index] == '%')
		{
			key ^= '.' << (i & 1);
		}
		else
		{
			key ^= string[index] << (i & 1);
		}
		index++;
		// decode the data with this key
		*(msg->_data + i) = *(msg->_data + i) ^ key;
	}
}

/*
=================
CL_Netchan_TransmitNextFragment
=================
*/
void CL_Netchan_TransmitNextFragment(netchan_t* chan)
{
	Netchan_TransmitNextFragment(chan);
}

/*
===============
CL_Netchan_Transmit
================
*/
void CL_Netchan_Transmit(netchan_t* chan, QMsg* msg)
{
	msg->WriteByte(q3clc_EOF);

	CL_Netchan_Encode(msg);
	Netchan_Transmit(chan, msg->cursize, msg->_data);
}

int newsize = 0;

/*
=================
CL_Netchan_Process
=================
*/
qboolean CL_Netchan_Process(netchan_t* chan, QMsg* msg)
{
	int ret;

	ret = Netchan_Process(chan, msg);
	if (!ret)
	{
		return false;
	}
	CL_Netchan_Decode(msg);
	newsize += msg->cursize;
	return true;
}
