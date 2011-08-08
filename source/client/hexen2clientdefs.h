//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

//	.MDL model effect flags.
enum
{
	H2MDLEF_ROCKET = 1,				// leave a trail
	H2MDLEF_GRENADE = 2,			// leave a trail
	H2MDLEF_GIB = 4,				// leave a trail
	H2MDLEF_ROTATE = 8,				// rotate (bonus items)
	H2MDLEF_TRACER = 16,			// green split trail
	H2MDLEF_ZOMGIB = 32,			// small blood trail
	H2MDLEF_TRACER2 = 64,			// orange split trail + rotate
	H2MDLEF_TRACER3 = 128,			// purple trail
	H2MDLEF_FIREBALL = 256,			// Yellow transparent trail in all directions
	H2MDLEF_ICE = 512,				// Blue-white transparent trail, with gravity
	H2MDLEF_MIP_MAP = 1024,			// This model has mip-maps
	H2MDLEF_SPIT = 2048,			// Black transparent trail with negative light
	H2MDLEF_TRANSPARENT = 4096,		// Transparent sprite
	H2MDLEF_SPELL = 8192,			// Vertical spray of particles
	H2MDLEF_HOLEY = 16384,			// Solid model with color 0
	H2MDLEF_SPECIAL_TRANS = 32768,	// Translucency through the particle table
	H2MDLEF_FACE_VIEW = 65536,		// Poly Model always faces you
	H2MDLEF_VORP_MISSILE = 131072,	// leave a trail at top and bottom of model
	H2MDLEF_SET_STAFF = 262144,		// slowly move up and left/right
	H2MDLEF_MAGICMISSILE = 524288,	// a trickle of blue/white particles with gravity
	H2MDLEF_BONESHARD = 1048576,	// a trickle of brown particles with gravity
	H2MDLEF_SCARAB = 2097152,		// white transparent particles with little gravity
	H2MDLEF_ACIDBALL = 4194304,		// Green drippy acid shit
	H2MDLEF_BLOODSHOT = 8388608,	// Blood rain shot trail
};