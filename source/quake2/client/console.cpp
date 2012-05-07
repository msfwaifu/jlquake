/*
Copyright (C) 1997-2001 Id Software, Inc.

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

#include "client.h"

void Key_ClearTyping(void)
{
	g_consoleField.buffer[0] = 0;	// clear any typing
	g_consoleField.cursor = 0;
}

/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f(void)
{
	con.acLength = 0;

	SCR_EndLoadingPlaque();		// get rid of loading plaque

	if (cl.q2_attractloop)
	{
		Cbuf_AddText("killserver\n");
		return;
	}

	if (cls.state == CA_DISCONNECTED)
	{	// start the demo loop again
		Cbuf_AddText("d1\n");
		return;
	}

	Key_ClearTyping();
	Con_ClearNotify();

	if (in_keyCatchers & KEYCATCH_CONSOLE)
	{
		in_keyCatchers &= ~KEYCATCH_CONSOLE;
		M_ForceMenuOff();
		Cvar_SetLatched("paused", "0");
	}
	else
	{
		M_ForceMenuOff();
		in_keyCatchers |= KEYCATCH_CONSOLE;

		if (Cvar_VariableValue("maxclients") == 1 &&
			Com_ServerState())
		{
			Cvar_SetLatched("paused", "1");
		}
	}
}

/*
================
Con_ToggleChat_f
================
*/
void Con_ToggleChat_f(void)
{
	Key_ClearTyping();

	if (in_keyCatchers & KEYCATCH_CONSOLE)
	{
		if (cls.state == CA_ACTIVE)
		{
			M_ForceMenuOff();
			in_keyCatchers &= ~KEYCATCH_CONSOLE;
		}
	}
	else
	{
		in_keyCatchers |= KEYCATCH_CONSOLE;
	}

	Con_ClearNotify();
}

/*
================
Con_Dump_f

Save the console contents out to a file
================
*/
void Con_Dump_f(void)
{
	int l, x;
	short* line;
	fileHandle_t f;
	char buffer[1024];
	char name[MAX_OSPATH];

	if (Cmd_Argc() != 2)
	{
		Com_Printf("usage: condump <filename>\n");
		return;
	}

	String::Sprintf(name, sizeof(name), "%s.txt", Cmd_Argv(1));

	Com_Printf("Dumped console text to %s.\n", name);
	f = FS_FOpenFileWrite(name);
	if (!f)
	{
		Com_Printf("ERROR: couldn't open.\n");
		return;
	}

	// skip empty lines
	for (l = con.current - con.totallines + 1; l <= con.current; l++)
	{
		line = con.text + (l % con.totallines) * con.linewidth;
		for (x = 0; x < con.linewidth; x++)
			if (line[x] != ' ')
			{
				break;
			}
		if (x != con.linewidth)
		{
			break;
		}
	}

	// write the remaining lines
	buffer[con.linewidth] = 0;
	for (; l <= con.current; l++)
	{
		line = con.text + (l % con.totallines) * con.linewidth;
		for (int i = 0; i < con.linewidth; i++)
			buffer[i] = line[i] & 0xff;
		for (x = con.linewidth - 1; x >= 0; x--)
		{
			if (buffer[x] == ' ')
			{
				buffer[x] = 0;
			}
			else
			{
				break;
			}
		}
		for (x = 0; buffer[x]; x++)
			buffer[x] &= 0x7f;

		FS_Printf(f, "%s\n", buffer);
	}

	FS_FCloseFile(f);
}

/*
================
Con_MessageMode_f
================
*/
void Con_MessageMode_f(void)
{
	chat_team = false;
	in_keyCatchers |= KEYCATCH_MESSAGE;
	chatField.widthInChars = (viddef.width >> 3) - 6;
}

/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f(void)
{
	chat_team = true;
	in_keyCatchers |= KEYCATCH_MESSAGE;
	chatField.widthInChars = (viddef.width >> 3) - 12;
}

/*
================
Con_Init
================
*/
void Con_Init(void)
{
	Com_Printf("Console initialized.\n");

	Con_InitCommon();

//
// register our commands
//
	con_notifytime = Cvar_Get("con_notifytime", "3", 0);

	Cmd_AddCommand("toggleconsole", Con_ToggleConsole_f);
	Cmd_AddCommand("togglechat", Con_ToggleChat_f);
	Cmd_AddCommand("messagemode", Con_MessageMode_f);
	Cmd_AddCommand("messagemode2", Con_MessageMode2_f);
	Cmd_AddCommand("clear", Con_Clear_f);
	Cmd_AddCommand("condump", Con_Dump_f);
}

/*
================
Con_Print

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the text will appear at the top of the game window
================
*/
void Con_Print(const char* txt)
{
	int mask;

	if (txt[0] == 1 || txt[0] == 2)
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
==============
Con_CenteredPrint
==============
*/
void Con_CenteredPrint(char* text)
{
	int l;
	char buffer[1024];

	l = String::Length(text);
	l = (con.linewidth - l) / 2;
	if (l < 0)
	{
		l = 0;
	}
	Com_Memset(buffer, ' ', l);
	String::Cpy(buffer + l, text);
	String::Cat(buffer, sizeof(buffer), "\n");
	Con_Print(buffer);
}
