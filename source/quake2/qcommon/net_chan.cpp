/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "qcommon.h"

/*

packet header
-------------
31	sequence
1	does this message contain a reliable payload
31	acknowledge sequence
1	acknowledge receipt of even/odd message
16	qport

The remote connection never knows if it missed a reliable message, the
local side detects that it has been dropped by seeing a sequence acknowledge
higher thatn the last reliable sequence, but without the correct evon/odd
bit for the reliable set.

If the sender notices that a reliable message has been dropped, it will be
retransmitted.  It will not be retransmitted again until a message after
the retransmit has been acknowledged and the reliable still failed to get there.

if the sequence number is -1, the packet should be handled without a netcon

The reliable message can be added to at any time by doing
MSG_Write* (&netchan->message, <data>).

If the message buffer is overflowed, either by a single message, or by
multiple frames worth piling up while the last reliable transmit goes
unacknowledged, the netchan signals a fatal error.

Reliable messages are always placed first in a packet, then the unreliable
message is included if there is sufficient room.

To the receiver, there is no distinction between the reliable and unreliable
parts of the message, they are just processed out as a single larger message.

Illogical packet sequence numbers cause the packet to be dropped, but do
not kill the connection.  This, combined with the tight window of valid
reliable acknowledgement numbers provides protection against malicious
address spoofing.


The qport field is a workaround for bad address translating routers that
sometimes remap the client's source port on a packet during gameplay.

If the base part of the net address matches and the qport matches, then the
channel matches even if the IP port differs.  The IP port should be updated
to the new value before sending out any replies.


If there is no information that needs to be transfered on a given frame,
such as during the connection stage while waiting for the client to load,
then a packet only needs to be delivered if there is something in the
unacknowledged reliable
*/

Cvar* showpackets;
Cvar* showdrop;
Cvar* qport;

netadr_t net_from;
QMsg net_message;
byte net_message_buffer[MAX_MSGLEN_Q2];

/*
===============
Netchan_Init

===============
*/
void Netchan_Init(void)
{
	int port;

	// pick a port value that should be nice and random
	port = Sys_Milliseconds_() & 0xffff;

	showpackets = Cvar_Get("showpackets", "0", 0);
	showdrop = Cvar_Get("showdrop", "0", 0);
	qport = Cvar_Get("qport", va("%i", port), CVAR_INIT);
}

/*
===============
Netchan_OutOfBand

Sends an out-of-band datagram
================
*/
void Netchan_OutOfBand(int net_socket, netadr_t adr, int length, byte* data)
{
	QMsg send;
	byte send_buf[MAX_MSGLEN_Q2];

// write the packet header
	send.InitOOB(send_buf, sizeof(send_buf));

	send.WriteLong(-1);	// -1 sequence means out of band
	send.WriteData(data, length);

// send the datagram
	NET_SendPacket((netsrc_t)net_socket, send.cursize, send._data, adr);
}

/*
===============
Netchan_OutOfBandPrint

Sends a text message in an out-of-band datagram
================
*/
void Netchan_OutOfBandPrint(int net_socket, netadr_t adr, const char* format, ...)
{
	va_list argptr;
	static char string[MAX_MSGLEN_Q2 - 4];

	va_start(argptr, format);
	Q_vsnprintf(string, sizeof(string), format, argptr);
	va_end(argptr);

	Netchan_OutOfBand(net_socket, adr, String::Length(string), (byte*)string);
}


/*
==============
Netchan_Setup

called to open a channel to a remote system
==============
*/
void Netchan_Setup(netsrc_t sock, netchan_t* chan, netadr_t adr, int qport)
{
	Com_Memset(chan, 0, sizeof(*chan));

	chan->sock = sock;
	chan->remoteAddress = adr;
	chan->qport = qport;
	chan->lastReceived = curtime;
	chan->incomingSequence = 0;
	chan->outgoingSequence = 1;

	chan->message.InitOOB(chan->messageBuffer, MAX_MSGLEN_Q2 - 16);	// leave space for header
	chan->message.allowoverflow = true;
}


/*
===============
Netchan_CanReliable

Returns true if the last reliable message has acked
================
*/
qboolean Netchan_CanReliable(netchan_t* chan)
{
	if (chan->reliableOrUnsentLength)
	{
		return false;			// waiting for ack
	}
	return true;
}


qboolean Netchan_NeedReliable(netchan_t* chan)
{
	qboolean send_reliable;

// if the remote side dropped the last reliable message, resend it
	send_reliable = false;

	if (chan->incomingAcknowledged > chan->lastReliableSequence &&
		chan->incomingReliableAcknowledged != chan->outgoingReliableSequence)
	{
		send_reliable = true;
	}

// if the reliable transmit buffer is empty, copy the current message out
	if (!chan->reliableOrUnsentLength && chan->message.cursize)
	{
		send_reliable = true;
	}

	return send_reliable;
}

/*
===============
Netchan_Transmit

tries to send an unreliable message to a connection, and handles the
transmition / retransmition of the reliable messages.

A 0 length will still generate a packet and deal with the reliable messages.
================
*/
void Netchan_Transmit(netchan_t* chan, int length, byte* data)
{
	QMsg send;
	byte send_buf[MAX_MSGLEN_Q2];
	qboolean send_reliable;
	unsigned w1, w2;

// check for message overflow
	if (chan->message.overflowed)
	{
		Com_Printf("%s:Outgoing message overflow\n",
			SOCK_AdrToString(chan->remoteAddress));
		return;
	}

	send_reliable = Netchan_NeedReliable(chan);

	if (!chan->reliableOrUnsentLength && chan->message.cursize)
	{
		Com_Memcpy(chan->reliableOrUnsentBuffer, chan->messageBuffer, chan->message.cursize);
		chan->reliableOrUnsentLength = chan->message.cursize;
		chan->message.cursize = 0;
		chan->outgoingReliableSequence ^= 1;
	}


// write the packet header
	send.InitOOB(send_buf, sizeof(send_buf));

	w1 = (chan->outgoingSequence & ~(1 << 31)) | (send_reliable << 31);
	w2 = (chan->incomingSequence & ~(1 << 31)) | (chan->incomingReliableSequence << 31);

	chan->outgoingSequence++;
	chan->lastSent = curtime;

	send.WriteLong(w1);
	send.WriteLong(w2);

	// send the qport if we are a client
	if (chan->sock == NS_CLIENT)
	{
		send.WriteShort(qport->value);
	}

// copy the reliable message to the packet first
	if (send_reliable)
	{
		send.WriteData(chan->reliableOrUnsentBuffer, chan->reliableOrUnsentLength);
		chan->lastReliableSequence = chan->outgoingSequence;
	}

// add the unreliable part if space is available
	if (send.maxsize - send.cursize >= length)
	{
		send.WriteData(data, length);
	}
	else
	{
		Com_Printf("Netchan_Transmit: dumped unreliable\n");
	}

// send the datagram
	NET_SendPacket(chan->sock, send.cursize, send._data, chan->remoteAddress);

	if (showpackets->value)
	{
		if (send_reliable)
		{
			Com_Printf("send %4i : s=%i reliable=%i ack=%i rack=%i\n",
				send.cursize,
				chan->outgoingSequence - 1,
				chan->outgoingReliableSequence,
				chan->incomingSequence,
				chan->incomingReliableSequence);
		}
		else
		{
			Com_Printf("send %4i : s=%i ack=%i rack=%i\n",
				send.cursize,
				chan->outgoingSequence - 1,
				chan->incomingSequence,
				chan->incomingReliableSequence);
		}
	}
}

/*
=================
Netchan_Process

called when the current net_message is from remote_address
modifies net_message so that it points to the packet payload
=================
*/
qboolean Netchan_Process(netchan_t* chan, QMsg* msg)
{
	int sequence, sequence_ack;
	int reliable_ack, reliable_message;
	int qport;

// get sequence numbers
	msg->BeginReadingOOB();
	sequence = msg->ReadLong();
	sequence_ack = msg->ReadLong();

	// read the qport if we are a server
	if (chan->sock == NS_SERVER)
	{
		qport = msg->ReadShort();
	}

	reliable_message = (unsigned)sequence >> 31;
	reliable_ack = (unsigned)sequence_ack >> 31;

	sequence &= ~(1 << 31);
	sequence_ack &= ~(1 << 31);

	if (showpackets->value)
	{
		if (reliable_message)
		{
			Com_Printf("recv %4i : s=%i reliable=%i ack=%i rack=%i\n",
				msg->cursize,
				sequence,
				chan->incomingReliableSequence ^ 1,
				sequence_ack,
				reliable_ack);
		}
		else
		{
			Com_Printf("recv %4i : s=%i ack=%i rack=%i\n",
				msg->cursize,
				sequence,
				sequence_ack,
				reliable_ack);
		}
	}

//
// discard stale or duplicated packets
//
	if (sequence <= chan->incomingSequence)
	{
		if (showdrop->value)
		{
			Com_Printf("%s:Out of order packet %i at %i\n",
				SOCK_AdrToString(chan->remoteAddress),
				sequence,
				chan->incomingSequence);
		}
		return false;
	}

//
// dropped packets don't keep the message from being used
//
	chan->dropped = sequence - (chan->incomingSequence + 1);
	if (chan->dropped > 0)
	{
		if (showdrop->value)
		{
			Com_Printf("%s:Dropped %i packets at %i\n",
				SOCK_AdrToString(chan->remoteAddress),
				chan->dropped,
				sequence);
		}
	}

//
// if the current outgoing reliable message has been acknowledged
// clear the buffer to make way for the next
//
	if (reliable_ack == chan->outgoingReliableSequence)
	{
		chan->reliableOrUnsentLength = 0;	// it has been received

	}
//
// if this message contains a reliable message, bump incoming_reliable_sequence
//
	chan->incomingSequence = sequence;
	chan->incomingAcknowledged = sequence_ack;
	chan->incomingReliableAcknowledged = reliable_ack;
	if (reliable_message)
	{
		chan->incomingReliableSequence ^= 1;
	}

//
// the message can now be read from the current message pointer
//
	chan->lastReceived = curtime;

	return true;
}

/*
=============================================================================

LOOPBACK BUFFERS FOR LOCAL PLAYER

=============================================================================
*/

#define MAX_LOOPBACK    4

struct loopmsg_t
{
	byte data[MAX_PACKETLEN];
	int datalen;
};

struct loopback_t
{
	loopmsg_t msgs[MAX_LOOPBACK];
	int get, send;
};

static loopback_t loopbacks[2];

static bool NET_GetLoopPacket(netsrc_t sock, netadr_t* net_from, QMsg* net_message)
{
	int i;
	loopback_t* loop;

	loop = &loopbacks[sock];

	if (loop->send - loop->get > MAX_LOOPBACK)
	{
		loop->get = loop->send - MAX_LOOPBACK;
	}

	if (loop->get >= loop->send)
	{
		return false;
	}

	i = loop->get & (MAX_LOOPBACK - 1);
	loop->get++;

	Com_Memcpy(net_message->_data, loop->msgs[i].data, loop->msgs[i].datalen);
	net_message->cursize = loop->msgs[i].datalen;
	Com_Memset(net_from, 0, sizeof(*net_from));
	net_from->type = NA_LOOPBACK;
	return true;

}

static void NET_SendLoopPacket(netsrc_t sock, int length, void* data, netadr_t to)
{
	int i;
	loopback_t* loop;

	loop = &loopbacks[sock ^ 1];

	i = loop->send & (MAX_LOOPBACK - 1);
	loop->send++;

	Com_Memcpy(loop->msgs[i].data, data, length);
	loop->msgs[i].datalen = length;
}

//=============================================================================

int ip_sockets[2];

//=============================================================================

qboolean NET_GetPacket(netsrc_t sock, netadr_t* net_from, QMsg* net_message)
{
	int ret;
	int net_socket;

	if (NET_GetLoopPacket(sock, net_from, net_message))
	{
		return true;
	}

	net_socket = ip_sockets[sock];

	if (!net_socket)
	{
		return false;
	}

	ret = SOCK_Recv(net_socket, net_message->_data, net_message->maxsize, net_from);
	if (ret == SOCKRECV_NO_DATA)
	{
		return false;
	}
	if (ret == SOCKRECV_ERROR)
	{
		if (!com_dedicated->value)	// let dedicated servers continue after errors
		{
			Com_Error(ERR_DROP, "NET_GetPacket failed");
		}
		return false;
	}

	if (ret == net_message->maxsize)
	{
		Com_Printf("Oversize packet from %s\n", SOCK_AdrToString(*net_from));
		return false;
	}

	net_message->cursize = ret;
	return true;
}

//=============================================================================

void NET_SendPacket(netsrc_t sock, int length, void* data, netadr_t to)
{
	int net_socket;

	if (to.type == NA_LOOPBACK)
	{
		NET_SendLoopPacket(sock, length, data, to);
		return;
	}

	if (to.type == NA_BROADCAST)
	{
		net_socket = ip_sockets[sock];
		if (!net_socket)
		{
			return;
		}
	}
	else if (to.type == NA_IP)
	{
		net_socket = ip_sockets[sock];
		if (!net_socket)
		{
			return;
		}
	}
	else
	{
		Com_Error(ERR_FATAL, "NET_SendPacket: bad address type");
	}

	int ret = SOCK_Send(net_socket, data, length, &to);
	if (ret == SOCKSEND_ERROR)
	{
		if (!com_dedicated->value)	// let dedicated servers continue after errors
		{
			Com_Error(ERR_DROP, "NET_SendPacket ERROR");
		}
	}
}

//=============================================================================

static Cvar* noudp;

/*
====================
NET_OpenIP
====================
*/
static void NET_OpenIP()
{
	Cvar* ip = Cvar_Get("ip", "localhost", CVAR_INIT);

	int dedicated = Cvar_VariableValue("dedicated");

	if (!ip_sockets[NS_SERVER])
	{
		int port = Cvar_Get("ip_hostport", "0", CVAR_INIT)->integer;
		if (!port)
		{
			port = Cvar_Get("hostport", "0", CVAR_INIT)->integer;
			if (!port)
			{
				port = Cvar_Get("port", va("%i", PORT_SERVER), CVAR_INIT)->integer;
			}
		}
		ip_sockets[NS_SERVER] = SOCK_Open(ip->string, port);
		if (!ip_sockets[NS_SERVER] && dedicated)
		{
			Com_Error(ERR_FATAL, "Couldn't allocate dedicated server IP port");
		}
	}

	// dedicated servers don't need client ports
	if (dedicated)
	{
		return;
	}

	if (!ip_sockets[NS_CLIENT])
	{
		int port = Cvar_Get("ip_clientport", "0", CVAR_INIT)->integer;
		if (!port)
		{
			port = Cvar_Get("clientport", va("%i", PORT_CLIENT), CVAR_INIT)->integer;
			if (!port)
			{
				port = PORT_ANY;
			}
		}
		ip_sockets[NS_CLIENT] = SOCK_Open(ip->string, port);
		if (!ip_sockets[NS_CLIENT])
		{
			ip_sockets[NS_CLIENT] = SOCK_Open(ip->string, PORT_ANY);
		}
	}
}

/*
====================
NET_Config

A single player game will only use the loopback code
====================
*/
void    NET_Config(qboolean multiplayer)
{
	int i;
	static qboolean old_config;

	if (old_config == multiplayer)
	{
		return;
	}

	old_config = multiplayer;

	if (!multiplayer)
	{	// shut down any existing sockets
		for (i = 0; i < 2; i++)
		{
			if (ip_sockets[i])
			{
				SOCK_Close(ip_sockets[i]);
				ip_sockets[i] = 0;
			}
		}
	}
	else
	{	// open sockets
		if (!noudp->value)
		{
			NET_OpenIP();
		}
	}
}

//===================================================================

/*
====================
NET_Init
====================
*/
void NET_Init()
{
	if (!SOCK_Init())
	{
		Com_Error(ERR_FATAL,"Sockets initialization failed.");
	}

	noudp = Cvar_Get("noudp", "0", CVAR_INIT);
}

/*
====================
NET_Shutdown
====================
*/
void NET_Shutdown()
{
	NET_Config(false);	// close sockets

	SOCK_Shutdown();
}

//=============================================================================

// sleeps msec or until net socket is ready
void NET_Sleep(int msec)
{
	if (!com_dedicated || !com_dedicated->value)
	{
		return;	// we're not a server, just run full speed
	}

	SOCK_Sleep(ip_sockets[NS_SERVER], msec);
}
