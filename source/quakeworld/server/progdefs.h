/*
Copyright (C) 1996-1997 Id Software, Inc.

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

/* file generated by qcc, do not modify */

typedef struct
{   int pad[28];
	int self;
	int other;
	int world;
	float time;
	float frametime;
	int newmis;
	float force_retouch;
	string_t mapname;
	float serverflags;
	float total_secrets;
	float total_monsters;
	float found_secrets;
	float killed_monsters;
	float parm1;
	float parm2;
	float parm3;
	float parm4;
	float parm5;
	float parm6;
	float parm7;
	float parm8;
	float parm9;
	float parm10;
	float parm11;
	float parm12;
	float parm13;
	float parm14;
	float parm15;
	float parm16;
	vec3_t v_forward;
	vec3_t v_up;
	vec3_t v_right;
	float trace_allsolid;
	float trace_startsolid;
	float trace_fraction;
	vec3_t trace_endpos;
	vec3_t trace_plane_normal;
	float trace_plane_dist;
	int trace_ent;
	float trace_inopen;
	float trace_inwater;
	int msg_entity;
	func_t main;
	func_t StartFrame;
	func_t PlayerPreThink;
	func_t PlayerPostThink;
	func_t ClientKill;
	func_t ClientConnect;
	func_t PutClientInServer;
	func_t ClientDisconnect;
	func_t SetNewParms;
	func_t SetChangeParms; } globalvars_t;

#define PROGHEADER_CRC 54730
