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

#include "ui.h"
#include "../game/quake_hexen2/view.h"
#include "../game/quake2/local.h"
#include "../../common/Common.h"
#include "../../common/common_defs.h"
#include "../client_main.h"

static Cvar* cl_polyblend;

void V_Init() {
	crosshair = Cvar_Get( "crosshair", "0", CVAR_ARCHIVE );
	cl_polyblend = Cvar_Get( "cl_polyblend", "1", 0 );

	if ( GGameType & GAME_QuakeHexen ) {
		VQH_Init();
	}
	if ( GGameType & GAME_Quake2 ) {
		VQ2_Init();
	}
}

float CalcFov( float fovX, float width, float height ) {
	if ( fovX < 1 || fovX > 179 ) {
		common->Error( "Bad fov: %f", fovX );
	}

	float x = width / tan( DEG2RAD( fovX * 0.5f ) );

	float a = atan( height / x );

	a = RAD2DEG( a * 2 );

	return a;
}

void R_PolyBlend( refdef_t* fd, float* blendColour ) {
	if ( !cl_polyblend->value ) {
		return;
	}
	if ( !blendColour[ 3 ] ) {
		return;
	}

	R_SetColor( blendColour );
	R_StretchPic( fd->x, fd->y, fd->width, fd->height, 0, 0, 0, 0, cls.whiteShader );
	R_SetColor( NULL );
}
