//**************************************************************************
//**
//**	$Id: render_local.h 745 2011-04-29 15:35:25Z dj_jl $
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

// the renderer front end should never modify glstate_t
struct glstate_t
{
	int				currenttextures[2];
	int				currenttmu;
	bool			finishCalled;
	int				texEnv[2];
	int				faceCulling;
	unsigned long	glStateBits;
};

void GL_Bind(image_t* Image);

extern glstate_t	glState;		// outside of TR since it shouldn't be cleared during ref re-init
