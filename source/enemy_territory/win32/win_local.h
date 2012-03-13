/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).  

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// win_local.h: Win32-specific Quake3 header file

#if defined ( _MSC_VER ) && ( _MSC_VER >= 1200 )
#pragma warning(disable : 4201)
#pragma warning( push )
#endif
#include "../../wolfclient/windows_shared.h"
#if defined ( _MSC_VER ) && ( _MSC_VER >= 1200 )
#pragma warning( pop )
#endif

#ifndef __GNUC__
#include <dinput.h>
#include <dsound.h>
#else
#include <directx.h>
#endif

#include <winsock.h>
#include <wsipx.h>

void    IN_MouseEvent( int mstate );

// Input subsystem

void    IN_Init( void );
void    IN_Shutdown( void );
void    IN_JoystickCommands( void );

void    IN_Move( etusercmd_t *cmd );
// add additional non keyboard / non mouse movement on top of the keyboard move cmd

void    IN_DeactivateWin32Mouse( void );

void    IN_Activate( qboolean active );
void    IN_Frame( void );

// window procedure
LRESULT WINAPI MainWndProc(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam );

// ydnar: mousewheel stuff
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL       ( WM_MOUSELAST + 1 )  // message that will be supported by the OS
#endif

#ifndef WM_XBUTTONDOWN
#define WM_XBUTTONDOWN      ( WM_MOUSELAST + 2 )
#define WM_XBUTTONUP        ( WM_MOUSELAST + 3 )
#define MK_XBUTTON1         0x0020
#define MK_XBUTTON2         0x0040
#endif

// Gordon: exception handling
void WinSetExceptionWnd( HWND wnd );
void WinSetExceptionVersion( const char* version );
void Com_FrameExt( void );
