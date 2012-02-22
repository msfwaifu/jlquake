/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// cvar.c -- dynamic variable tracking

#include "../game/q_shared.h"
#include "qcommon.h"

const char* Cvar_TranslateString(const char* string)
{
	return string;
}

void Cvar_Changed(Cvar* var)
{
}

/*
============
Cvar_WriteVariables

Appends lines containing "set variable value" for all variables
with the archive flag set to qtrue.
============
*/
void Cvar_WriteVariables( fileHandle_t f ) {
	Cvar  *var;
	char buffer[1024];

	for ( var = cvar_vars ; var ; var = var->next ) {
		if ( String::ICmp( var->name, "cl_cdkey" ) == 0 ) {
			continue;
		}
		if ( var->flags & CVAR_ARCHIVE ) {
			// write the latched value, even if it hasn't taken effect yet
			if ( var->latchedString ) {
				String::Sprintf( buffer, sizeof( buffer ), "seta %s \"%s\"\n", var->name, var->latchedString );
			} else {
				String::Sprintf( buffer, sizeof( buffer ), "seta %s \"%s\"\n", var->name, var->string );
			}
			FS_Printf( f, "%s", buffer );
		}
	}
}
