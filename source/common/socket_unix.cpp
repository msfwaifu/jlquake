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

// HEADER FILES ------------------------------------------------------------

#include "socket_local.h"
#include "Common.h"
#include "strings.h"
#include "console_variable.h"
#include "system_unix.h"
#include <errno.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/if.h>
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

static bool usingSocks = false;
static int socks_socket;
static sockaddr socksRelayAddr;

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	NetadrToSockadr
//
//==========================================================================

static void NetadrToSockadr( const netadr_t* a, sockaddr_in* s ) {
	Com_Memset( s, 0, sizeof ( *s ) );

	if ( a->type == NA_BROADCAST ) {
		s->sin_family = AF_INET;

		s->sin_port = a->port;
		*( int* )& s->sin_addr = -1;
	} else if ( a->type == NA_IP ) {
		s->sin_family = AF_INET;

		*( int* )& s->sin_addr = *( int* )&a->ip;
		s->sin_port = a->port;
	} else {
		common->FatalError( "Invalid address type" );
	}
}

//==========================================================================
//
//	SockadrToNetadr
//
//==========================================================================

static void SockadrToNetadr( sockaddr_in* s, netadr_t* a ) {
	*( int* )& a->ip = *( int* )&s->sin_addr;
	a->port = s->sin_port;
	a->type = NA_IP;
}

//==========================================================================
//
//	SOCK_StringToSockaddr
//
//==========================================================================

static bool SOCK_StringToSockaddr( const char* s, sockaddr_in* sadr ) {
	Com_Memset( sadr, 0, sizeof ( *sadr ) );
	sadr->sin_family = AF_INET;

	sadr->sin_port = 0;

	if ( s[ 0 ] >= '0' && s[ 0 ] <= '9' ) {
		*( int* )& sadr->sin_addr = inet_addr( s );
	} else {
		hostent* h = gethostbyname( s );
		if ( !h ) {
			return false;
		}
		*( int* )& sadr->sin_addr = *( int* )h->h_addr_list[ 0 ];
	}
	return true;
}

//==========================================================================
//
//	SOCK_GetAddressByName
//
//==========================================================================

bool SOCK_GetAddressByName( const char* s, netadr_t* a ) {
	sockaddr_in sadr;
	if ( !SOCK_StringToSockaddr( s, &sadr ) ) {
		return false;
	}

	SockadrToNetadr( &sadr, a );

	return true;
}

//==========================================================================
//
//	SOCK_ErrorString
//
//==========================================================================

static const char* SOCK_ErrorString() {
	int code = errno;
	return strerror( code );
}

//==========================================================================
//
//	SOCK_Init
//
//==========================================================================

bool SOCK_Init() {
	return true;
}

//==========================================================================
//
//	SOCK_Shutdown
//
//==========================================================================

void SOCK_Shutdown() {
}

//==========================================================================
//
//	SOCK_GetLocalAddress
//
//==========================================================================

void SOCK_GetLocalAddress() {
	numIP = 0;

	ifaddrs* IfAddrs = NULL;
	if ( getifaddrs( &IfAddrs ) == -1 ) {
		common->Printf( "SOCK_GetLocalAddress: getifaddrs: %s", SOCK_ErrorString() );
		return;
	}

	for ( ifaddrs* Addr = IfAddrs; Addr && numIP < MAX_IPS; Addr = Addr->ifa_next ) {
		if ( Addr->ifa_addr->sa_family != AF_INET ) {
			continue;
		}

		localIP[ numIP ][ 0 ] = Addr->ifa_addr->sa_data[ 2 ];
		localIP[ numIP ][ 1 ] = Addr->ifa_addr->sa_data[ 3 ];
		localIP[ numIP ][ 2 ] = Addr->ifa_addr->sa_data[ 4 ];
		localIP[ numIP ][ 3 ] = Addr->ifa_addr->sa_data[ 5 ];
		common->Printf( "IP: %i.%i.%i.%i\n", localIP[ numIP ][ 0 ], localIP[ numIP ][ 1 ], localIP[ numIP ][ 2 ], localIP[ numIP ][ 3 ] );
		numIP++;
	}

	if ( IfAddrs ) {
		freeifaddrs( IfAddrs );
	}
}

//==========================================================================
//
//	SOCK_OpenSocks
//
//==========================================================================

void SOCK_OpenSocks( int port ) {
	usingSocks = false;

	common->Printf( "Opening connection to SOCKS server.\n" );

	socks_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if ( socks_socket == -1 ) {
		common->Printf( "WARNING: SOCK_OpenSocks: socket: %s\n", SOCK_ErrorString() );
		return;
	}

	hostent* h = gethostbyname( net_socksServer->string );
	if ( h == NULL ) {
		common->Printf( "WARNING: SOCK_OpenSocks: gethostbyname: %s\n", SOCK_ErrorString() );
		return;
	}
	if ( h->h_addrtype != AF_INET ) {
		common->Printf( "WARNING: SOCK_OpenSocks: gethostbyname: address type was not AF_INET\n" );
		return;
	}
	sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = *( int* )h->h_addr_list[ 0 ];
	address.sin_port = htons( ( short )net_socksPort->integer );

	if ( connect( socks_socket, ( sockaddr* )&address, sizeof ( address ) ) == -1 ) {
		common->Printf( "SOCK_OpenSocks: connect: %s\n", SOCK_ErrorString() );
		return;
	}

	// send socks authentication handshake
	bool rfc1929 = *net_socksUsername->string || *net_socksPassword->string;

	unsigned char buf[ 64 ];
	int len;
	buf[ 0 ] = 5;		// SOCKS version
	// method count
	if ( rfc1929 ) {
		buf[ 1 ] = 2;
		len = 4;
	} else {
		buf[ 1 ] = 1;
		len = 3;
	}
	buf[ 2 ] = 0;		// method #1 - method id #00: no authentication
	if ( rfc1929 ) {
		buf[ 2 ] = 2;		// method #2 - method id #02: username/password
	}
	if ( send( socks_socket, buf, len, 0 ) == -1 ) {
		common->Printf( "SOCK_OpenSocks: send: %s\n", SOCK_ErrorString() );
		return;
	}

	// get the response
	len = recv( socks_socket, buf, 64, 0 );
	if ( len == -1 ) {
		common->Printf( "SOCK_OpenSocks: recv: %s\n", SOCK_ErrorString() );
		return;
	}
	if ( len != 2 || buf[ 0 ] != 5 ) {
		common->Printf( "SOCK_OpenSocks: bad response\n" );
		return;
	}
	switch ( buf[ 1 ] ) {
	case 0:	// no authentication
		break;
	case 2:	// username/password authentication
		break;
	default:
		common->Printf( "SOCK_OpenSocks: request denied\n" );
		return;
	}

	// do username/password authentication if needed
	if ( buf[ 1 ] == 2 ) {
		// build the request
		int ulen = String::Length( net_socksUsername->string );
		int plen = String::Length( net_socksPassword->string );

		buf[ 0 ] = 1;		// username/password authentication version
		buf[ 1 ] = ulen;
		if ( ulen ) {
			Com_Memcpy( &buf[ 2 ], net_socksUsername->string, ulen );
		}
		buf[ 2 + ulen ] = plen;
		if ( plen ) {
			Com_Memcpy( &buf[ 3 + ulen ], net_socksPassword->string, plen );
		}

		// send it
		if ( send( socks_socket, buf, 3 + ulen + plen, 0 ) == -1 ) {
			common->Printf( "SOCK_OpenSocks: send: %s\n", SOCK_ErrorString() );
			return;
		}

		// get the response
		len = recv( socks_socket, buf, 64, 0 );
		if ( len == -1 ) {
			common->Printf( "SOCK_OpenSocks: recv: %s\n", SOCK_ErrorString() );
			return;
		}
		if ( len != 2 || buf[ 0 ] != 1 ) {
			common->Printf( "SOCK_OpenSocks: bad response\n" );
			return;
		}
		if ( buf[ 1 ] != 0 ) {
			common->Printf( "SOCK_OpenSocks: authentication failed\n" );
			return;
		}
	}

	// send the UDP associate request
	buf[ 0 ] = 5;		// SOCKS version
	buf[ 1 ] = 3;		// command: UDP associate
	buf[ 2 ] = 0;		// reserved
	buf[ 3 ] = 1;		// address type: IPV4
	*( int* )&buf[ 4 ] = INADDR_ANY;
	*( short* )&buf[ 8 ] = htons( ( short )port );		// port
	if ( send( socks_socket, buf, 10, 0 ) == -1 ) {
		common->Printf( "SOCK_OpenSocks: send: %s\n", SOCK_ErrorString() );
		return;
	}

	// get the response
	len = recv( socks_socket, buf, 64, 0 );
	if ( len == -1 ) {
		common->Printf( "SOCK_OpenSocks: recv: %s\n", SOCK_ErrorString() );
		return;
	}
	if ( len < 2 || buf[ 0 ] != 5 ) {
		common->Printf( "SOCK_OpenSocks: bad response\n" );
		return;
	}
	// check completion code
	if ( buf[ 1 ] != 0 ) {
		common->Printf( "SOCK_OpenSocks: request denied: %i\n", buf[ 1 ] );
		return;
	}
	if ( buf[ 3 ] != 1 ) {
		common->Printf( "SOCK_OpenSocks: relay address is not IPV4: %i\n", buf[ 3 ] );
		return;
	}
	( ( sockaddr_in* )&socksRelayAddr )->sin_family = AF_INET;
	( ( sockaddr_in* )&socksRelayAddr )->sin_addr.s_addr = *( int* )&buf[ 4 ];
	( ( sockaddr_in* )&socksRelayAddr )->sin_port = *( short* )&buf[ 8 ];
	Com_Memset( ( ( sockaddr_in* )&socksRelayAddr )->sin_zero, 0, sizeof ( ( ( sockaddr_in* )&socksRelayAddr )->sin_zero ) );

	usingSocks = true;
}

//==========================================================================
//
//	SOCK_CloseSocks
//
//==========================================================================

void SOCK_CloseSocks() {
	if ( socks_socket && socks_socket != -1 ) {
		close( socks_socket );
		socks_socket = 0;
	}
	usingSocks = false;
}

//==========================================================================
//
//	SOCK_Open
//
//==========================================================================

int SOCK_Open( const char* net_interface, int port ) {
	if ( net_interface ) {
		common->Printf( "Opening IP socket: %s:%i\n", net_interface, port );
	} else {
		common->Printf( "Opening IP socket: localhost:%i\n", port );
	}

	int newsocket = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP );
	if ( newsocket == -1 ) {
		common->Printf( "ERROR: SOCK_Open: socket: %s\n", SOCK_ErrorString() );
		return 0;
	}

	// make it non-blocking
	int _true = 1;
	if ( ioctl( newsocket, FIONBIO, &_true ) == -1 ) {
		common->Printf( "ERROR: SOCK_Open: ioctl FIONBIO:%s\n", SOCK_ErrorString() );
		close( newsocket );
		return 0;
	}

	// make it broadcast capable
	int i = 1;
	if ( setsockopt( newsocket, SOL_SOCKET, SO_BROADCAST, ( char* )&i, sizeof ( i ) ) == -1 ) {
		common->Printf( "ERROR: SOCK_Open: setsockopt SO_BROADCAST:%s\n", SOCK_ErrorString() );
		close( newsocket );
		return 0;
	}

	sockaddr_in address;
	if ( !net_interface || !net_interface[ 0 ] || !String::ICmp( net_interface, "localhost" ) ) {
		address.sin_addr.s_addr = INADDR_ANY;
	} else {
		SOCK_StringToSockaddr( net_interface, &address );
	}

	if ( port == PORT_ANY ) {
		address.sin_port = 0;
	} else {
		address.sin_port = htons( ( short )port );
	}

	address.sin_family = AF_INET;

	if ( bind( newsocket, ( sockaddr* )&address, sizeof ( address ) ) == -1 ) {
		common->Printf( "ERROR: SOCK_Open: bind: %s\n", SOCK_ErrorString() );
		close( newsocket );
		return 0;
	}

	return newsocket;
}

//==========================================================================
//
//	SOCK_Close
//
//==========================================================================

void SOCK_Close( int Socket ) {
	close( Socket );
}

//==========================================================================
//
//	SOCK_Recv
//
//==========================================================================

int SOCK_Recv( int socket, void* buf, int len, netadr_t* From ) {
	sockaddr_in addr;
	socklen_t addrlen = sizeof ( addr );
	int ret;
	idList<quint8> SocksBuf;
	if ( usingSocks ) {
		SocksBuf.SetNum( len + 10 );
		ret = recvfrom( socket, SocksBuf.Ptr(), len + 10, 0, ( sockaddr* )&addr, &addrlen );
	} else {
		ret = recvfrom( socket, buf, len, 0, ( sockaddr* )&addr, &addrlen );
	}

	if ( ret == -1 ) {
		int err = errno;
		if ( err == EWOULDBLOCK || err == ECONNREFUSED ) {
			return SOCKRECV_NO_DATA;
		}

		common->Printf( "NET_GetPacket: %s", SOCK_ErrorString() );
		//common->Printf("NET_GetPacket: %s from %s\n", SOCK_ErrorString(), SOCK_AdrToString(*net_from));
		return SOCKRECV_ERROR;
	}


	Com_Memset( addr.sin_zero, 0, sizeof ( addr.sin_zero ) );

	if ( usingSocks ) {
		if ( memcmp( &addr, &socksRelayAddr, sizeof ( addr ) ) == 0 ) {
			if ( ret < 10 || SocksBuf[ 0 ] != 0 || SocksBuf[ 1 ] != 0 || SocksBuf[ 2 ] != 0 || SocksBuf[ 3 ] != 1 ) {
				return SOCKRECV_ERROR;
			}
			From->type = NA_IP;
			From->ip[ 0 ] = SocksBuf[ 4 ];
			From->ip[ 1 ] = SocksBuf[ 5 ];
			From->ip[ 2 ] = SocksBuf[ 6 ];
			From->ip[ 3 ] = SocksBuf[ 7 ];
			From->port = *( short* )&SocksBuf[ 8 ];
			ret -= 10;
			if ( ret > 0 ) {
				Com_Memcpy( buf, &SocksBuf[ 10 ], ret );
			}
		} else {
			SockadrToNetadr( &addr, From );
			if ( ret > len ) {
				ret = len;
			}
			if ( ret > 0 ) {
				Com_Memcpy( buf, SocksBuf.Ptr(), ret );
			}
		}
	} else {
		SockadrToNetadr( &addr, From );
	}
	return ret;
}

//==========================================================================
//
//	SOCK_Send
//
//==========================================================================

int SOCK_Send( int socket, const void* data, int length, const netadr_t& to ) {
	sockaddr_in addr;
	NetadrToSockadr( &to, &addr );

	int ret;
	if ( usingSocks && to.type == NA_IP ) {
		idList<char> socksBuf;
		socksBuf.SetNum( length + 10 );
		socksBuf[ 0 ] = 0;		// reserved
		socksBuf[ 1 ] = 0;
		socksBuf[ 2 ] = 0;		// fragment (not fragmented)
		socksBuf[ 3 ] = 1;		// address type: IPV4
		*( int* )&socksBuf[ 4 ] = addr.sin_addr.s_addr;
		*( short* )&socksBuf[ 8 ] = addr.sin_port;
		Com_Memcpy( &socksBuf[ 10 ], data, length );
		ret = sendto( socket, socksBuf.Ptr(), length + 10, 0, &socksRelayAddr, sizeof ( socksRelayAddr ) );
	} else {
		ret = sendto( socket, data, length, 0, ( struct sockaddr* )&addr, sizeof ( addr ) );
	}

	if ( ret == -1 ) {
		int err = errno;

		// wouldblock is silent
		if ( err == EWOULDBLOCK ) {
			return SOCKSEND_WOULDBLOCK;
		}

		// some PPP links do not allow broadcasts and return an error
		if ( ( err == EADDRNOTAVAIL ) && ( to.type == NA_BROADCAST ) ) {
			return SOCKSEND_WOULDBLOCK;
		}

		common->Printf( "NET_SendPacket ERROR: %s\n", SOCK_ErrorString() );
		//common->Printf("NET_SendPacket ERROR: %s to %s\n", SOCK_ErrorString(), SOCK_AdrToString(to));
		return SOCKSEND_ERROR;
	}

	return ret;
}

//==========================================================================
//
//	SOCK_Sleep
//
//	Sleeps msec or until net socket is ready
//
//==========================================================================

bool SOCK_Sleep( int socket, int msec ) {
	if ( !socket ) {
		return false;
	}

	fd_set fdset;
	FD_ZERO( &fdset );
	if ( stdin_active ) {
		FD_SET( 0, &fdset );	// stdin is processed too
	}
	FD_SET( socket, &fdset );	// network socket

	timeval timeout;
	timeout.tv_sec = msec / 1000;
	timeout.tv_usec = ( msec % 1000 ) * 1000;

	return select( socket + 1, &fdset, NULL, NULL, &timeout ) != -1;
}

//==========================================================================
//
//	SOCK_GetAddr
//
//==========================================================================

bool SOCK_GetAddr( int socket, netadr_t* addr ) {
	sockaddr_in sadr;
	socklen_t addrlen = sizeof ( sadr );

	Com_Memset( &sadr, 0, sizeof ( sadr ) );
	if ( getsockname( socket, ( sockaddr* )&sadr, &addrlen ) == -1 ) {
		common->Printf( "getsockname: %s", SOCK_ErrorString() );
		return false;
	}

	SockadrToNetadr( &sadr, addr );
	return true;
}
