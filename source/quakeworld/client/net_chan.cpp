/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "quakedef.h"

#ifdef _WIN32
#include "../../client/windows_shared.h"
#else
#include <unistd.h>
#endif

#define	PACKET_HEADER	8

/*

packet header
-------------
31	sequence
1	does this message contain a reliable payload
31	acknowledge sequence
1	acknowledge receipt of even/odd message
16  qport

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

Reliable messages are allways placed first in a packet, then the unreliable
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


*/

Cvar*	showpackets;
Cvar*	showdrop;
Cvar*	qport;

/*
===============
Netchan_Init

===============
*/
void Netchan_Init (void)
{
	int		port;

	// pick a port value that should be nice and random
#ifdef _WIN32
	port = ((int)(timeGetTime()*1000) * time(NULL)) & 0xffff;
#else
	port = ((int)(getpid()+getuid()*1000) * time(NULL)) & 0xffff;
#endif

	showpackets = Cvar_Get("showpackets", "0", 0);
	showdrop = Cvar_Get("showdrop", "0", 0);
	qport = Cvar_Get("qport", "0", 0);
	Cvar_SetValue("qport", port);
}

/*
===============
Netchan_OutOfBand

Sends an out-of-band datagram
================
*/
void Netchan_OutOfBand (netadr_t adr, int length, byte *data)
{
	QMsg		send;
	byte		send_buf[MAX_MSGLEN + PACKET_HEADER];

// write the packet header
	send.InitOOB(send_buf, sizeof(send_buf));
	
	send.WriteLong(-1);	// -1 sequence means out of band
	send.WriteData(data, length);

// send the datagram
	//zoid, no input in demo playback mode
#ifndef SERVERONLY
	if (!cls.demoplayback)
#endif
		NET_SendPacket (send.cursize, send._data, adr);
}

/*
===============
Netchan_OutOfBandPrint

Sends a text message in an out-of-band datagram
================
*/
void Netchan_OutOfBandPrint (netadr_t adr, const char *format, ...)
{
	va_list		argptr;
	static char		string[8192];		// ??? why static?
	
	va_start (argptr, format);
	Q_vsnprintf(string, 8192, format, argptr);
	va_end (argptr);


	Netchan_OutOfBand (adr, String::Length(string), (byte *)string);
}


/*
==============
Netchan_Setup

called to open a channel to a remote system
==============
*/
void Netchan_Setup (netsrc_t sock, netchan_t *chan, netadr_t adr, int qport)
{
	Com_Memset(chan, 0, sizeof(*chan));
	
	chan->remoteAddress = adr;
	chan->last_received = realtime;
	
	chan->message.InitOOB(chan->message_buf, sizeof(chan->message_buf));
	chan->message.allowoverflow = true;

	chan->qport = qport;
	
	chan->rate = 1.0/2500;
}


/*
===============
Netchan_CanPacket

Returns true if the bandwidth choke isn't active
================
*/
#define	MAX_BACKUP	200
qboolean Netchan_CanPacket (netchan_t *chan)
{
	if (chan->cleartime < realtime + MAX_BACKUP*chan->rate)
		return true;
	return false;
}


/*
===============
Netchan_CanReliable

Returns true if the bandwidth choke isn't 
================
*/
qboolean Netchan_CanReliable (netchan_t *chan)
{
	if (chan->reliable_length)
		return false;			// waiting for ack
	return Netchan_CanPacket (chan);
}

#ifdef SERVERONLY
qboolean ServerPaused(void);
#endif

/*
===============
Netchan_Transmit

tries to send an unreliable message to a connection, and handles the
transmition / retransmition of the reliable messages.

A 0 length will still generate a packet and deal with the reliable messages.
================
*/
void Netchan_Transmit (netchan_t *chan, int length, byte *data)
{
	QMsg		send;
	byte		send_buf[MAX_MSGLEN + PACKET_HEADER];
	qboolean	send_reliable;
	unsigned	w1, w2;
	int			i;

// check for message overflow
	if (chan->message.overflowed)
	{
		chan->fatal_error = true;
		Con_Printf ("%s:Outgoing message overflow\n"
			, SOCK_AdrToString (chan->remoteAddress));
		return;
	}

// if the remote side dropped the last reliable message, resend it
	send_reliable = false;

	if (chan->incoming_acknowledged > chan->last_reliable_sequence
	&& chan->incoming_reliable_acknowledged != chan->reliable_sequence)
		send_reliable = true;

// if the reliable transmit buffer is empty, copy the current message out
	if (!chan->reliable_length && chan->message.cursize)
	{
		Com_Memcpy(chan->reliable_buf, chan->message_buf, chan->message.cursize);
		chan->reliable_length = chan->message.cursize;
		chan->message.cursize = 0;
		chan->reliable_sequence ^= 1;
		send_reliable = true;
	}

// write the packet header
	send.InitOOB(send_buf, sizeof(send_buf));

	w1 = chan->outgoing_sequence | (send_reliable<<31);
	w2 = chan->incoming_sequence | (chan->incoming_reliable_sequence<<31);

	chan->outgoing_sequence++;

	send.WriteLong(w1);
	send.WriteLong(w2);

	// send the qport if we are a client
#ifndef SERVERONLY
	send.WriteShort(cls.qport);
#endif

// copy the reliable message to the packet first
	if (send_reliable)
	{
		send.WriteData(chan->reliable_buf, chan->reliable_length);
		chan->last_reliable_sequence = chan->outgoing_sequence;
	}
	
// add the unreliable part if space is available
	if (send.maxsize - send.cursize >= length)
		send.WriteData(data, length);

// send the datagram
	i = chan->outgoing_sequence & (MAX_LATENT-1);
	chan->outgoing_size[i] = send.cursize;
	chan->outgoing_time[i] = realtime;

	//zoid, no input in demo playback mode
#ifndef SERVERONLY
	if (!cls.demoplayback)
#endif
		NET_SendPacket (send.cursize, send._data, chan->remoteAddress);

	if (chan->cleartime < realtime)
		chan->cleartime = realtime + send.cursize*chan->rate;
	else
		chan->cleartime += send.cursize*chan->rate;
#ifdef SERVERONLY
	if (ServerPaused())
		chan->cleartime = realtime;
#endif

	if (showpackets->value)
		Con_Printf ("--> s=%i(%i) a=%i(%i) %i\n"
			, chan->outgoing_sequence
			, send_reliable
			, chan->incoming_sequence
			, chan->incoming_reliable_sequence
			, send.cursize);

}

/*
=================
Netchan_Process

called when the current net_message is from remote_address
modifies net_message so that it points to the packet payload
=================
*/
qboolean Netchan_Process (netchan_t *chan)
{
	unsigned		sequence, sequence_ack;
	unsigned		reliable_ack, reliable_message;
#ifdef SERVERONLY
	int			qport;
#endif

	if (
#ifndef SERVERONLY
			!cls.demoplayback && 
#endif
			!SOCK_CompareAdr (net_from, chan->remoteAddress))
		return false;
	
// get sequence numbers		
	net_message.BeginReadingOOB();
	sequence = net_message.ReadLong();
	sequence_ack = net_message.ReadLong();

	// read the qport if we are a server
#ifdef SERVERONLY
	qport = net_message.ReadShort();
#endif

	reliable_message = sequence >> 31;
	reliable_ack = sequence_ack >> 31;

	sequence &= ~(1<<31);	
	sequence_ack &= ~(1<<31);	

	if (showpackets->value)
		Con_Printf ("<-- s=%i(%i) a=%i(%i) %i\n"
			, sequence
			, reliable_message
			, sequence_ack
			, reliable_ack
			, net_message.cursize);

// get a rate estimation
#if 0
	if (chan->outgoing_sequence - sequence_ack < MAX_LATENT)
	{
		int				i;
		double			time, rate;
	
		i = sequence_ack & (MAX_LATENT - 1);
		time = realtime - chan->outgoing_time[i];
		time -= 0.1;	// subtract 100 ms
		if (time <= 0)
		{	// gotta be a digital link for <100 ms ping
			if (chan->rate > 1.0/5000)
				chan->rate = 1.0/5000;
		}
		else
		{
			if (chan->outgoing_size[i] < 512)
			{	// only deal with small messages
				rate = chan->outgoing_size[i]/time;
				if (rate > 5000)
					rate = 5000;
				rate = 1.0/rate;
				if (chan->rate > rate)
					chan->rate = rate;
			}
		}
	}
#endif

//
// discard stale or duplicated packets
//
	if (sequence <= (unsigned)chan->incoming_sequence)
	{
		if (showdrop->value)
			Con_Printf ("%s:Out of order packet %i at %i\n"
				, SOCK_AdrToString (chan->remoteAddress)
				,  sequence
				, chan->incoming_sequence);
		return false;
	}

//
// dropped packets don't keep the message from being used
//
	chan->dropped = sequence - (chan->incoming_sequence+1);
	if (chan->dropped > 0)
	{
		chan->drop_count += 1;

		if (showdrop->value)
			Con_Printf ("%s:Dropped %i packets at %i\n"
			, SOCK_AdrToString (chan->remoteAddress)
			, sequence-(chan->incoming_sequence+1)
			, sequence);
	}

//
// if the current outgoing reliable message has been acknowledged
// clear the buffer to make way for the next
//
	if (reliable_ack == (unsigned)chan->reliable_sequence)
		chan->reliable_length = 0;	// it has been received
	
//
// if this message contains a reliable message, bump incoming_reliable_sequence 
//
	chan->incoming_sequence = sequence;
	chan->incoming_acknowledged = sequence_ack;
	chan->incoming_reliable_acknowledged = reliable_ack;
	if (reliable_message)
		chan->incoming_reliable_sequence ^= 1;

//
// the message can now be read from the current message pointer
// update statistics counters
//
	chan->frame_latency = chan->frame_latency*OLD_AVG
		+ (chan->outgoing_sequence-sequence_ack)*(1.0-OLD_AVG);
	chan->frame_rate = chan->frame_rate*OLD_AVG
		+ (realtime-chan->last_received)*(1.0-OLD_AVG);		
	chan->good_count += 1;

	chan->last_received = realtime;

	return true;
}


//=============================================================================

netadr_t	net_from;
QMsg		net_message;
int			net_socket;

#define	MAX_UDP_PACKET	(MAX_MSGLEN*2)	// one more than msg + header
byte		net_message_buffer[MAX_UDP_PACKET];

//=============================================================================

static int UDP_OpenSocket (int port)
{
	int newsocket;
	int i;

	const char* net_interface = NULL;
	//ZOID -- check for interface binding option
	if ((i = COM_CheckParm("-ip")) != 0 && i < COM_Argc())
	{
		net_interface = COM_Argv(i+1);
	}
	newsocket = SOCK_Open(net_interface, port);
	if (newsocket == 0)
		Sys_Error ("UDP_OpenSocket: socket failed");
	return newsocket;
}

//=============================================================================

qboolean NET_GetPacket (void)
{
	int ret = SOCK_Recv(net_socket, net_message_buffer, sizeof(net_message_buffer), &net_from);
	if (ret == SOCKRECV_NO_DATA)
	{
		return false;
	}
	if (ret == SOCKRECV_ERROR)
	{
		Sys_Error("NET_GetPacket failed");
	}

	net_message.cursize = ret;
	if (ret == sizeof(net_message_buffer) )
	{
		Con_Printf ("Oversize packet from %s\n", SOCK_AdrToString (net_from));
		return false;
	}

	return ret;
}

//=============================================================================

void NET_SendPacket (int length, void *data, netadr_t to)
{
	SOCK_Send(net_socket, data, length, &to);
}

/*
====================
NET_Init
====================
*/
void NET_Init (int port)
{
	if (!SOCK_Init())
		Sys_Error("Sockets initialization failed.");

	//
	// open the single socket to be used for all communications
	//
	net_socket = UDP_OpenSocket (port);

	//
	// init the message buffer
	//
	net_message.InitOOB(net_message_buffer, sizeof(net_message_buffer));

	//
	// determine my addresses
	//
	SOCK_GetLocalAddress();
}

/*
====================
NET_Shutdown
====================
*/
void	NET_Shutdown (void)
{
	SOCK_Close(net_socket);
	SOCK_Shutdown();
}
