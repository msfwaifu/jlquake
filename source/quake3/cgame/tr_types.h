/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
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
//
#ifndef __TR_TYPES_H
#define __TR_TYPES_H

// FIXME: VM should be OS agnostic .. in theory

#if defined(Q3_VM) || defined(_WIN32)

#define OPENGL_DRIVER_NAME	"opengl32"

#else

// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=524
#define OPENGL_DRIVER_NAME	"libGL.so.1"

#endif	// !defined _WIN32

#endif	// __TR_TYPES_H
