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

//
// console
//
extern qboolean con_forcedup;	// because no entities to refresh
extern byte* con_chars;

void Con_DrawCharacter(int cx, int line, int num);

void Con_Init(void);
void Con_DrawConsole(int lines, qboolean drawinput);
void Con_Print(const char* txt);
void Con_Printf(const char* fmt, ...);
void Con_DPrintf(const char* fmt, ...);
void Con_SafePrintf(const char* fmt, ...);
void Con_Clear_f(void);
void Con_DrawNotify(void);
void Con_ToggleConsole_f(void);

void Con_NotifyBox(const char* text);	// during startup for sound / cd warnings
