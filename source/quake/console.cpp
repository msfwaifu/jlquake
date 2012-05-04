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
// console.c

#include "quakedef.h"

qboolean con_forcedup;			// because no entities to refresh

Cvar* con_notifytime;

qboolean con_debuglog;

extern void M_Menu_Main_f(void);

/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f(void)
{
	if (in_keyCatchers & KEYCATCH_CONSOLE)
	{
		in_keyCatchers &= ~KEYCATCH_CONSOLE;
		if (cls.state == CA_CONNECTED)
		{
			g_consoleField.buffer[0] = 0;	// clear any typing
			g_consoleField.cursor = 0;
		}
		else
		{
			M_Menu_Main_f();
		}
	}
	else
	{
		in_keyCatchers |= KEYCATCH_CONSOLE;
	}

	SCR_EndLoadingPlaque();
	Con_ClearNotify();
}

/*
================
Con_MessageMode_f
================
*/
extern qboolean chat_team;

void Con_MessageMode_f(void)
{
	in_keyCatchers |= KEYCATCH_MESSAGE;
	chat_team = false;
}


/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f(void)
{
	in_keyCatchers |= KEYCATCH_MESSAGE;
	chat_team = true;
}

/*
================
Con_Init
================
*/
void Con_Init(void)
{
	const char* t2 = "qconsole.log";

	con_debuglog = COM_CheckParm("-condebug");

	if (con_debuglog)
	{
		FS_FCloseFile(FS_FOpenFileWrite(t2));
	}

	con.cursorspeed = 4;

	Con_Printf("Console initialized.\n");

//
// register our commands
//
	con_notifytime = Cvar_Get("con_notifytime", "3", 0);		//seconds

	Cmd_AddCommand("toggleconsole", Con_ToggleConsole_f);
	Cmd_AddCommand("messagemode", Con_MessageMode_f);
	Cmd_AddCommand("messagemode2", Con_MessageMode2_f);
	Cmd_AddCommand("clear", Con_Clear_f);
}

/*
================
Con_Print

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the notify window will pop up.
================
*/
void Con_Print(const char* txt)
{
	int mask;

	if (txt[0] == 1)
	{
		mask = 128;		// go to colored text
		S_StartLocalSound("misc/talk.wav");
		// play talk wav
		txt++;
	}
	else if (txt[0] == 2)
	{
		mask = 128;		// go to colored text
		txt++;
	}
	else
	{
		mask = 0;
	}

	CL_ConsolePrintCommon(txt, mask);
}


/*
================
Con_DebugLog
================
*/
void Con_DebugLog(const char* file, const char* fmt, ...)
{
	va_list argptr;
	static char data[8 * 1024];
	fileHandle_t fd;

	va_start(argptr, fmt);
	Q_vsnprintf(data, sizeof(data), fmt, argptr);
	va_end(argptr);
	FS_FOpenFileByMode(file, &fd, FS_APPEND);
	FS_Write(data, String::Length(data), fd);
	FS_FCloseFile(fd);
}


/*
================
Con_Printf

Handles cursor positioning, line wrapping, etc
================
*/
void Con_Printf(const char* fmt, ...)
{
	va_list argptr;
	char msg[MAXPRINTMSG];
	static qboolean inupdate;

	va_start(argptr,fmt);
	Q_vsnprintf(msg, MAXPRINTMSG, fmt, argptr);
	va_end(argptr);

// also echo to debugging console
	Sys_Print(msg);	// also echo to debugging console

// log all messages to file
	if (con_debuglog)
	{
		Con_DebugLog("qconsole.log", "%s", msg);
	}

	if (cls.state == CA_DEDICATED)
	{
		return;		// no graphics mode

	}
// write it to the scrollable buffer
	Con_Print(msg);

// update the screen if the console is displayed
	if (clc.qh_signon != SIGNONS && !cls.disable_screen)
	{
		// protect against infinite loop if something in SCR_UpdateScreen calls
		// Con_Printd
		if (!inupdate)
		{
			inupdate = true;
			SCR_UpdateScreen();
			inupdate = false;
		}
	}
}

/*
================
Con_DPrintf

A Con_Printf that only shows up if the "developer" cvar is set
================
*/
void Con_DPrintf(const char* fmt, ...)
{
	va_list argptr;
	char msg[MAXPRINTMSG];

	if (!com_developer || !com_developer->value)
	{
		return;			// don't confuse non-developers with techie stuff...

	}
	va_start(argptr,fmt);
	Q_vsnprintf(msg, MAXPRINTMSG, fmt, argptr);
	va_end(argptr);

	Con_Printf("%s", msg);
}


/*
==================
Con_SafePrintf

Okay to call even when the screen can't be updated
==================
*/
void Con_SafePrintf(const char* fmt, ...)
{
	va_list argptr;
	char msg[MAXPRINTMSG];
	int temp;

	va_start(argptr, fmt);
	Q_vsnprintf(msg, MAXPRINTMSG, fmt, argptr);
	va_end(argptr);

	temp = cls.disable_screen;
	cls.disable_screen = true;
	Con_Printf("%s", msg);
	cls.disable_screen = temp;
}


/*
==============================================================================

DRAWING

==============================================================================
*/


/*
================
Con_DrawInput

The input line scrolls horizontally if typing goes beyond the right edge
================
*/
void Con_DrawInput(void)
{
	int y;
	char buffer[MAX_EDIT_LINE + 1];
	buffer[0] = ']';
	String::Cpy(buffer + 1, g_consoleField.buffer);
	int key_linepos = String::Length(buffer);
	char* text;

	if (!(in_keyCatchers & KEYCATCH_CONSOLE) && !con_forcedup)
	{
		return;		// don't draw anything

	}
	text = buffer;

// add the cursor frame
	text[key_linepos] = 10 + ((int)(realtime * con.cursorspeed) & 1);
	text[key_linepos + 1] = 0;

//	prestep if horizontally scrolling
	if (key_linepos >= con.linewidth)
	{
		text += 1 + key_linepos - con.linewidth;
	}

// draw it
	y = con.vislines - 16;

	UI_DrawString(8, y, text);
}


/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify(void)
{
	int x, v;
	short* text;
	int i;

	v = 0;
	for (i = con.current - NUM_CON_TIMES + 1; i <= con.current; i++)
	{
		if (i < 0)
		{
			continue;
		}
		int time = con.times[i % NUM_CON_TIMES];
		if (time == 0)
		{
			continue;
		}
		time = realtime * 1000 - time;
		if (time > con_notifytime->value * 1000)
		{
			continue;
		}
		text = con.text + (i % con.totallines) * con.linewidth;

		for (x = 0; x < con.linewidth; x++)
			UI_DrawChar((x + 1) << 3, v, text[x] & 0xff);

		v += 8;
	}


	if (in_keyCatchers & KEYCATCH_MESSAGE)
	{
		UI_DrawString(8, v, "say:");
		UI_DrawString(40, v, chatField.buffer);
		UI_DrawChar(40 + chatField.cursor * 8, v, 10 + ((int)(realtime * con.cursorspeed) & 1));
		v += 8;
	}
}

/*
================
Con_DrawConsole

Draws the console with the solid background
The typing input line at the bottom should only be drawn if typing is allowed
================
*/
void Con_DrawConsole(int lines, qboolean drawinput)
{
	int i, x, y;
	int rows;
	short* text;
	int j;

	if (lines <= 0)
	{
		return;
	}

// draw the background
	Draw_ConsoleBackground(lines);

// draw the text
	con.vislines = lines;

	rows = (lines - 16) >> 3;		// rows of text to draw
	y = lines - 16 - (rows << 3);	// may start slightly negative

	for (i = con.current - rows + 1; i <= con.current; i++, y += 8)
	{
		j = i - con.current + con.display;
		if (j < 0)
		{
			j = 0;
		}
		text = con.text + (j % con.totallines) * con.linewidth;

		for (x = 0; x < con.linewidth; x++)
			UI_DrawChar((x + 1) << 3, y, text[x] & 0xff);
	}

// draw the input prompt, user text, and cursor if desired
	if (drawinput)
	{
		Con_DrawInput();
	}
}


/*
==================
Con_NotifyBox
==================
*/
void Con_NotifyBox(const char* text)
{
	double t1, t2;

// during startup for sound / cd warnings
	Con_Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n");

	Con_Printf(text);

	Con_Printf("Press a key.\n");
	Con_Printf("\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n");

	key_count = -2;		// wait for a key down and up
	in_keyCatchers |= KEYCATCH_CONSOLE;

	do
	{
		t1 = Sys_DoubleTime();
		SCR_UpdateScreen();
		Sys_SendKeyEvents();
		IN_ProcessEvents();
		t2 = Sys_DoubleTime();
		realtime += t2 - t1;		// make the cursor blink
	}
	while (key_count < 0);

	Con_Printf("\n");
	in_keyCatchers &= ~KEYCATCH_CONSOLE;
	realtime = 0;				// put the cursor back to invisible
}
