//**************************************************************************
//**
//**	$Id$
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

#ifndef _WIN_SHARED_H
#define _WIN_SHARED_H

#include <windows.h>

void SNDDMA_Activate();

LONG CDAudio_MessageHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

LONG WINAPI MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
bool IN_HandleInputMessage(UINT uMsg, WPARAM  wParam, LPARAM  lParam);
void IN_StartupJoystick();
void IN_JoyMove();
void IN_StartupMIDI();
void IN_ShutdownMIDI();
void MidiInfo_f();
void IN_ShutdownDIMouse();
void IN_StartupMouse();
void IN_ActivateMouse();
void IN_DeactivateMouse();
void IN_MouseMove();

#define WINDOW_CLASS_NAME	"vQuake"

struct WinMouseVars_t
{
	int			oldButtonState;

	bool		mouseActive;
	bool		mouseInitialized;
	bool		mouseStartupDelayed; // delay mouse init to try DI again when we have a window
};

extern HINSTANCE	global_hInstance;
extern HWND			GMainWindow;
extern HDC			maindc;
extern HGLRC		baseRC;
extern bool			pixelFormatSet;
extern bool			cdsFullscreen;
extern unsigned		sysMsgTime;
extern QCvar*		in_joystick;
extern QCvar*		in_debugJoystick;
extern QCvar	*joy_threshold;
extern QCvar	*in_joyBallScale;
extern QCvar	*in_midi;
extern QCvar	*in_midiport;
extern QCvar	*in_midichannel;
extern QCvar	*in_mididevice;
extern WinMouseVars_t s_wmv;
extern QCvar	*in_mouse;

#endif
