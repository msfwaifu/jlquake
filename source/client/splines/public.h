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

#ifndef __SPLINES_PUBLIC__H__
#define __SPLINES_PUBLIC__H__

#include "../../common/qcommon.h"

bool loadCamera( int camNum, const char* name );
void startCamera( int camNum, int time );
bool getCameraInfo( int camNum, int time, float* origin, float* angles, float* fov );

#endif
