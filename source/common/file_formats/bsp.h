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

#ifndef _BSPFILE_H
#define _BSPFILE_H

struct bsp_lump_t {
	qint32 fileofs;
	qint32 filelen;
};

struct bsp_vertex_t {
	float point[ 3 ];
};

// note that edge 0 is never used, because negative edge nums are used for
// counterclockwise use of the edge in a face
struct bsp_edge_t {
	quint16 v[ 2 ];				// vertex numbers
};

#endif
